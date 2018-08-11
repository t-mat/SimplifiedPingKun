#pragma once

class TaskTrayNotifyIcon {
public:
	TaskTrayNotifyIcon(HWND hWnd, UINT uCallbackMessage, UINT notifyIconUid = 1)
		: hWnd { hWnd }
		, notifyIconUid { notifyIconUid }
	{
		auto nid = NotifyIconData(NIF_MESSAGE);
		nid.uCallbackMessage = uCallbackMessage;
		Shell_NotifyIconW(NIM_ADD, &nid);
	}

	~TaskTrayNotifyIcon() {
		auto nid = NotifyIconData();
		Shell_NotifyIconW(NIM_DELETE, &nid);
	}

	using MenuFunc = std::function<void(HMENU)>;

	LRESULT messageHandler(const MenuFunc& menuFunc) const {
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
		auto nid = NotifyIconData(NIF_ICON);
		nid.hIcon = hIcon;
		Shell_NotifyIcon(NIM_MODIFY, &nid);
	}

	void setTipText(const wchar_t* tipText) const {
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

protected:
	const HWND hWnd { nullptr };
	const UINT notifyIconUid = 1;
};
