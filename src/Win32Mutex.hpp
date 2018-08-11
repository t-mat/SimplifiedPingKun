#pragma once

class Win32Mutex {
public:
	Win32Mutex() {}
	~Win32Mutex() {
		release();
	}

	bool create(const TCHAR* mutexName, BOOL fInitialOwner = FALSE) {
		if(good()) {
			return false;
		} else {
			handle = CreateMutex(0, fInitialOwner, mutexName);
			return ERROR_ALREADY_EXISTS != GetLastError();
		}
	}

	void release() {
		if(good()) {
			ReleaseMutex(handle);
			handle = nullptr;
		}
	}

	bool good() const {
		return nullptr != handle;
	}

protected:
	HANDLE handle { nullptr };
};
