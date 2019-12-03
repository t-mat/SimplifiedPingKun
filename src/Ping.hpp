// https://www.winsocketdotnetworkprogramming.com/winsock2programming/winsock2advancedrawsocket11a.html
#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#pragma comment(lib, "ws2_32.lib")

//
//  usage:
//      void myTest() {
//          WSADATA wsaData;
//          WSAStartup(MAKEWORD(2,2), &wsaData);
//          const auto target = "8.8.8.8";
//          const auto result = ping_internal(target);
//          printf("Ping to %s -> %d ms\n", target, result);
//          WSACleanup();
//      }
//
inline int ping_internal(const char* ipAddrString, uint32_t timeout = 6000) {
    const auto getTick = []() -> uint64_t {
        return GetTickCount64();
    };

    const auto getId = []() -> uint16_t {
        return static_cast<uint16_t>(GetCurrentProcessId());
    };

    const auto calcChecksum = [](const void* buf, int bufLengthInBytes) -> uint16_t {
        uint32_t sum32 = 0;
        const auto* p = reinterpret_cast<const uint16_t*>(buf);
        for(int i = 0; i < bufLengthInBytes/2; ++i) {
            sum32 += p[i];
        }
        if((bufLengthInBytes & 1) != 0) {
            sum32 += reinterpret_cast<const uint8_t*>(buf)[bufLengthInBytes-1];
        }

        const uint32_t s1 = (sum32 >> 16) + (sum32 & 0xffff);
        const uint32_t s2 = s1 + (s1 >> 16);
        return static_cast<uint16_t>(~s2);
    };

    const auto internalPing = [&getTick, &getId, &calcChecksum](
          ADDRINFOA* destAddr
        , ADDRINFOA* localAddr
        , SOCKET sock
        , HANDLE event
        , uint32_t timeout
    ) -> int {
        // Bind the socket -- need to do this since we post a receive first
        if(SOCKET_ERROR == bind(sock, localAddr->ai_addr, (int) localAddr->ai_addrlen)) {
            return -1;
        }

        // Setup the receive operation
        WSAOVERLAPPED wsaOverlapped;
        memset(&wsaOverlapped, 0, sizeof(wsaOverlapped));
        wsaOverlapped.hEvent = event;

        // Post the first overlapped receive
        {
            SOCKADDR_STORAGE from;
            char recvbuf[0xFFFF];
            int recvbuflen = 0xFFFF;

            WSABUF wsabuf;
            wsabuf.buf = recvbuf;
            wsabuf.len = recvbuflen;

            DWORD flags = 0;
            DWORD bytes;
            int fromlen = sizeof(from);
            const auto err = WSARecvFrom(
                  sock
                , &wsabuf
                , 1
                , &bytes
                , &flags
                , (SOCKADDR*) &from
                , &fromlen
                , &wsaOverlapped
                , nullptr
            );
            if(err == SOCKET_ERROR) {
                const auto lastError = WSAGetLastError();
                if(lastError != WSA_IO_PENDING) {
                    return -1;
                }
            }
        }

        #include <pshpack1.h>
        struct ICMP_HDR {               // ICMP header
            uint8_t     icmp_type;      // +0
            uint8_t     icmp_code;      // +1
            uint16_t    icmp_checksum;  // +2
            uint16_t    icmp_id;        // +4
            uint16_t    icmp_sequence;  // +6
        };
        const uint8_t ICMP_ECHO = 8;    // Echo request
        #include <poppack.h>

        char icmpbuf[sizeof(ICMP_HDR) + 32] = { 0 };
        const int icmpbufLen = sizeof(icmpbuf);
        auto& icmpHdr = *reinterpret_cast<ICMP_HDR*>(&icmpbuf[0]);

        // Initialize the ICMP headers
        icmpHdr.icmp_type       = ICMP_ECHO;
        icmpHdr.icmp_id         = getId();
        icmpHdr.icmp_checksum   = calcChecksum(icmpbuf, icmpbufLen);

        const auto time0 = getTick();
        if(SOCKET_ERROR == sendto(sock, icmpbuf, icmpbufLen, 0, destAddr->ai_addr, (int) destAddr->ai_addrlen)) {
            return -1;
        }

        const auto waitResult = WSAWaitForMultipleEvents(1, &wsaOverlapped.hEvent, TRUE, timeout, FALSE);
        switch(waitResult) {
        case WAIT_FAILED:   return -1;
        case WAIT_TIMEOUT:  return -1;
        default:            break;
        }

        DWORD cbTransfer = 0;
        DWORD dwFlags = 0;
        if(! WSAGetOverlappedResult(sock, &wsaOverlapped, &cbTransfer, FALSE, &dwFlags)) {
            return -1;
        }
        const auto time1 = getTick();
        return static_cast<int>(time1 - time0);
    };

    int result = -1;
    {
        HANDLE event = WSACreateEvent();
        SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        ADDRINFOA* destAddr = nullptr;
        {
            struct addrinfo h;
            memset(&h, 0, sizeof(h));
            h.ai_flags  = 0;
            h.ai_family = AF_INET;
            getaddrinfo(ipAddrString, "0", &h, &destAddr);
        }

        ADDRINFOA* localAddr = nullptr;
        {
            struct addrinfo h;
            memset(&h, 0, sizeof(h));
            h.ai_flags  = AI_PASSIVE;
            h.ai_family = destAddr->ai_family;
            getaddrinfo(nullptr, "0", &h, &localAddr);
        }

        result = internalPing(destAddr, localAddr, sock, event, timeout);

        if(localAddr) { freeaddrinfo(localAddr); }
        if(destAddr) { freeaddrinfo(destAddr); }
        if(sock != INVALID_SOCKET) { closesocket(sock); }
        if(event != INVALID_HANDLE_VALUE) { WSACloseEvent(event); }
    }

    return result;
}


struct Ping {
    int ping(const char* target) {
        return ping_internal(target);
    }
};
