#pragma once

class TaskTrayNotifyIcon {
public:
	TaskTrayNotifyIcon(UINT notifyIconUid = 1)
		: hWnd { nullptr }
		, notifyIconUid { notifyIconUid }
	{}

	~TaskTrayNotifyIcon() {
		cleanup();
	}

	void cleanup() {
		if(!good()) { return; }
		auto nid = NotifyIconData();
		Shell_NotifyIconW(NIM_DELETE, &nid);
		invalidate();
	}

	void setCallback(HWND hWnd, UINT uCallbackMessage) {
		this->hWnd = hWnd;
		auto nid = NotifyIconData(NIF_MESSAGE);
		nid.uCallbackMessage = uCallbackMessage;
		Shell_NotifyIconW(NIM_ADD, &nid);
	}

	using MenuFunc = std::function<void(HMENU)>;

	LRESULT messageHandler(const MenuFunc& menuFunc) const {
		if(!good()) { return 0; }
		SetForegroundWindow(hWnd);
		SetFocus(hWnd);
		POINT p;
		GetCursorPos(&p);

		HMENU hMenu = CreatePopupMenu();
		menuFunc(hMenu);
		const auto result = TrackPopupMenu(
			hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, p.x, p.y, 0, hWnd, 0);
		DestroyMenu(hMenu);
		return result;
	}

	void setIcon(HICON hIcon) const {
		if(!good()) { return; }
		auto nid = NotifyIconData(NIF_ICON);
		nid.hIcon = hIcon;
		Shell_NotifyIcon(NIM_MODIFY, &nid);
	}

	void setTipText(const wchar_t* tipText) const {
		if(!good()) { return; }
		auto nid = NotifyIconData(NIF_TIP);
		StringCchCopy(nid.szTip, sizeof(nid.szTip)/sizeof(nid.szTip[0]), tipText);
		Shell_NotifyIcon(NIM_MODIFY, &nid);
	}

	NOTIFYICONDATAW NotifyIconData(UINT uFlags = 0) const {
		NOTIFYICONDATAW nid = { sizeof(nid) };
		nid.uID		= notifyIconUid;
		nid.hWnd	= hWnd;
		nid.uFlags	= uFlags;
		return nid;
	}

	bool good() const {
		return hWnd != nullptr;
	}

protected:
	void invalidate() {
		hWnd = nullptr;
	}

	HWND hWnd { nullptr };
	UINT notifyIconUid = 1;
};
