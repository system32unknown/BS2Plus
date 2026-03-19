#include "win.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>

char* path = nullptr;

#ifdef COMPILER_WINDOWS

#include "SDL/SDL_syswm.h"
#include "config.h"

void osinit(char* filename) {
	path = new char[strlen(filename) + 1];
	strcpy(path, filename);

	// Find the last path separator to isolate the directory portion
	char* tmp = nullptr;
	char* tmp2 = path;
	while ((tmp = strchr(tmp2, '/')) || (tmp = strchr(tmp2, '\\')))
		tmp2 = tmp + 1;
	*tmp2 = '\0';
	SetCurrentDirectoryA(path);

	if (strcmp(filename + strlen(filename) - 4, ".exe") != 0) return;

	const size_t cmdlen = strlen(filename) + 20;
	char* command = new char[cmdlen];
	snprintf(command, cmdlen, "\"%s\" \"%s\"", filename, "%1");

	HKEY hKey;

	RegCreateKeyA(HKEY_CLASSES_ROOT, "bs2mod\\shell\\open\\command", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(command), static_cast<DWORD>(strlen(command) + 1));
	RegCloseKey(hKey);

	RegCreateKeyA(HKEY_CLASSES_ROOT, "bs2mod\\DefaultIcon", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(filename), static_cast<DWORD>(strlen(filename) + 1));
	RegCloseKey(hKey);

	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2mod\\shell\\open\\command", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(command), static_cast<DWORD>(strlen(command) + 1));
	RegCloseKey(hKey);

	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2mod", &hKey);

	static const char bs2mod_url[] = "URL: bs2mod Protocol";
	RegSetValueExA(hKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(bs2mod_url), static_cast<DWORD>(sizeof(bs2mod_url)));
	RegSetValueExA(hKey, "URL Protocol", 0, REG_SZ, reinterpret_cast<const BYTE*>(""), 1);
	RegCloseKey(hKey);

	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2mod\\DefaultIcon", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(filename), static_cast<DWORD>(strlen(filename) + 1));
	RegCloseKey(hKey);

	RegCreateKeyA(HKEY_CLASSES_ROOT, "bs2addon\\shell\\open\\command", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(command), static_cast<DWORD>(strlen(command) + 1));
	RegCloseKey(hKey);

	RegCreateKeyA(HKEY_CLASSES_ROOT, "bs2addon\\DefaultIcon", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(filename), static_cast<DWORD>(strlen(filename) + 1));
	RegCloseKey(hKey);

	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2addon\\shell\\open\\command", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(command), static_cast<DWORD>(strlen(command) + 1));
	RegCloseKey(hKey);

	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2addon", &hKey);

	static const char bs2addon_url[] = "URL: bs2addon Protocol";
	RegSetValueExA(hKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(bs2addon_url), static_cast<DWORD>(sizeof(bs2addon_url)));
	RegSetValueExA(hKey, "URL Protocol", 0, REG_SZ, reinterpret_cast<const BYTE*>(""), 1);
	RegCloseKey(hKey);

	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2addon\\DefaultIcon", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, reinterpret_cast<const BYTE*>(filename), static_cast<DWORD>(strlen(filename) + 1));
	RegCloseKey(hKey);

	DragAcceptFiles(GetActiveWindow(), TRUE);

	delete[] command;
}

void sysmessage(SDL_SysWMmsg* msg) {
	if (msg->msg == WM_DROPFILES) {
		char tmp[256];
		const int count = static_cast<int>(DragQueryFile(reinterpret_cast<HDROP>(msg->wParam), 0xFFFFFFFF, nullptr, 0));
		for (int i = 0; i < count; i++) {
			DragQueryFileA(reinterpret_cast<HDROP>(msg->wParam), i, tmp, sizeof(tmp) - 1);
			parsefile(tmp, 0);
		}
	}
}

void ossystem(char* cmd, char* parameters, bool wait, bool hidden) {
	if (parameters) {
		SHELLEXECUTEINFOA t = {};
		t.cbSize = sizeof(SHELLEXECUTEINFOA);
		t.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT;
		t.hwnd = GetActiveWindow();
		t.lpVerb = "open";
		t.lpFile = cmd;
		t.lpParameters = parameters;
		t.nShow = hidden ? SW_HIDE : SW_SHOWNORMAL;

		if (ShellExecuteExA(&t) && wait)
			while (WaitForSingleObject(t.hProcess, INFINITE) != WAIT_OBJECT_0);
	} else {
		{
			std::ofstream batchfile("tmp.bat", std::ios::out);
			batchfile.write(cmd, static_cast<std::streamsize>(strlen(cmd)));
		}
		ossystem("tmp.bat", const_cast<char*>(""), wait, hidden);
		remove("tmp.bat");
	}
}

void mousebuttonbug(bool mouseup) {
	static HWND hwnd = GetActiveWindow();
	if (hwnd != GetActiveWindow() || mouseup) {
		SendMessage(hwnd, WM_LBUTTONUP, 0, 0);
		SendMessage(hwnd, WM_MBUTTONUP, 0, 0);
		SendMessage(hwnd, WM_RBUTTONUP, 0, 0);
		for (int i = 0; i < 100; i++)
			SendMessage(hwnd, WM_KEYUP, static_cast<WPARAM>(i), 0);
	}
	hwnd = GetActiveWindow();
}

int copyStringToClipboard(char* source) {
	if (!OpenClipboard(nullptr)) return 0;
	EmptyClipboard();
	const size_t len = strlen(source) + 1;
	HGLOBAL clipbuffer = GlobalAlloc(GMEM_DDESHARE, len);
	if (!clipbuffer) { CloseClipboard(); return 0; }
	char* buffer = static_cast<char*>(GlobalLock(clipbuffer));
	strcpy(buffer, source);
	GlobalUnlock(clipbuffer);
	SetClipboardData(CF_TEXT, clipbuffer);
	CloseClipboard();
	return 0;
}

char* getStringFromClipboard() {
	if (!OpenClipboard(nullptr)) return nullptr;
	HANDLE hData = GetClipboardData(CF_TEXT);
	char* buffer = static_cast<char*>(GlobalLock(hData));
	GlobalUnlock(hData);
	CloseClipboard();
	return buffer;
}

char* opendialog(char* filter, char* defaultname) {
	char* szFile = new char[260]();
	if (defaultname) strncpy(szFile, defaultname, 259);

	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = GetActiveWindow();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = 260;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	mousebuttonbug(true);
	if (GetOpenFileNameA(&ofn) == TRUE) return szFile;

	delete[] szFile;
	return nullptr;
}

char* savedialog(char* filter, char* defaultname) {
	char* szFile = new char[260]();
	if (defaultname) strncpy(szFile, defaultname, 259);

	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = GetActiveWindow();
	ofn.hInstance = reinterpret_cast<HINSTANCE>(GetCurrentProcess());
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = 258;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON | OFN_NODEREFERENCELINKS | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = "";

	mousebuttonbug(true);
	if (GetSaveFileNameA(&ofn) && szFile[0]) return szFile;

	delete[] szFile;
	return nullptr;
}

bool yesnobox(char* text, char* title) {
	const int i = MessageBoxA(GetActiveWindow(), text, title, MB_YESNO);
	mousebuttonbug(true);
	return i == IDYES;
}

void messagebox(char* text, char* title) {
	MessageBoxA(GetActiveWindow(), text, title, MB_OK);
	mousebuttonbug(true);
}

char* inputbox() {
	return nullptr;
}

#else

void osinit(char* filename) {
	path = new char[strlen(filename) + 1];
	strcpy(path, filename);
	char* tmp = nullptr;
	char* tmp2 = path;
	while ((tmp = strchr(tmp2, '/')) || (tmp = strchr(tmp2, '\\')))
		tmp2 = tmp + 1;
	*tmp2 = '\0';
}

void sysmessage(SDL_SysWMmsg* msg) {}

void ossystem(char* cmd, char* parameters, bool wait, bool hidden) {
#ifdef COMPILER_SYSTEM
	system(cmd);
#endif
}

void mousebuttonbug(bool mouseup) {}

int copyStringToClipboard(char* source) {
	return 0;
}

char* getStringFromClipboard() {
	char* buffer = new char[1];
	buffer[0] = '\0';
	return buffer;
}

char* opendialog(char* filter, char* defaultname) {
	char* buffer = new char[1];
	buffer[0] = '\0';
	return buffer;
}

char* savedialog(char* filter, char* defaultname) {
	char* buffer = new char[1];
	buffer[0] = '\0';
	return buffer;
}

bool yesnobox(char* text, char* title) {
	return false;
}

void messagebox(char* text, char* title) {}

char* inputbox() {
	return nullptr;
}

#endif

char* checkfilename(char* filename) {
	return filename;
}