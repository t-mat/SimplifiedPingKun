// https://docs.microsoft.com/en-us/windows/desktop/api/icmpapi/nf-icmpapi-icmpsendecho
#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

class Ping {
public:
    const int Error_InvalidHandleValue = -1;
    const int Error_IcmpSendEchoFailed = -2;

    Ping() {
        open();
    }

    ~Ping() {
        close();
    }

    int ping(const char* ipAddrString) const {
        if(!good()) {
            return Error_InvalidHandleValue;
        }

        IN_ADDR inAddr {};
        inet_pton(AF_INET, ipAddrString, &inAddr);

        static char sendData[] = "Ping";
        const auto replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
        char replyBuffer[replySize];
        const auto dwResult = IcmpSendEcho(
              hIcmpFile
            , inAddr.S_un.S_addr
            , sendData
            , sizeof(sendData)
            , nullptr
            , replyBuffer
            , sizeof(replyBuffer)
            , 1000
        );

        if(dwResult == 0) {
            return Error_IcmpSendEchoFailed;
        }

        auto* pEchoReply = reinterpret_cast<const ICMP_ECHO_REPLY*>(replyBuffer);
        return pEchoReply->RoundTripTime;
    }

protected:
    void open() {
        hIcmpFile = IcmpCreateFile();
    }

    void close() {
        if(good()) {
            IcmpCloseHandle(hIcmpFile);
        }
    }

    bool good() const {
        return INVALID_HANDLE_VALUE != hIcmpFile;
    }

    void* hIcmpFile = INVALID_HANDLE_VALUE;
};
