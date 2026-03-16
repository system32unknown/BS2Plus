#include "win.h"
#include <iostream>
#include <fstream>

char* path;

#ifdef COMPILER_WINDOWS

#include "SDL/SDL_syswm.h"
#include "config.h"

void osinit(char* filename) {
	path = new char[strlen(filename) + 1];
	strcpy(path, filename);
	char* tmp = 0;
	char* tmp2 = path;
	while ((tmp = strchr(tmp2, '/')) || (tmp = strchr(tmp2, '\\')))
		tmp2 = tmp + 1;
	*tmp2 = 0;
	SetCurrentDirectoryA(path);
	char* command = new char[strlen(filename) + 20];
	sprintf(command, "\"%s\" \"%s\"", filename, "%1");
	if (strcmp(filename + strlen(filename) - 4, ".exe")) return;

	HKEY hKey;
	RegCreateKeyA(HKEY_CLASSES_ROOT, "bs2mod\\shell\\open\\command", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)command, strlen(command));
	RegCloseKey(hKey);
	RegCreateKeyA(HKEY_CLASSES_ROOT, "bs2mod\\DefaultIcon", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)filename, strlen(filename));
	RegCloseKey(hKey);
	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2mod\\shell\\open\\command", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)command, strlen(command));
	RegCloseKey(hKey);
	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2mod", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)"URL: bs2mod Protocol", strlen("URL: bs2mod Protocol"));
	RegSetValueExA(hKey, "URL Protocol", 0, REG_SZ, (BYTE*)"", strlen(""));
	RegCloseKey(hKey);
	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2mod\\DefaultIcon", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)filename, strlen(filename));
	RegCloseKey(hKey);

	RegCreateKeyA(HKEY_CLASSES_ROOT, "bs2addon\\shell\\open\\command", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)command, strlen(command));
	RegCloseKey(hKey);
	RegCreateKeyA(HKEY_CLASSES_ROOT, "bs2addon\\DefaultIcon", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)filename, strlen(filename));
	RegCloseKey(hKey);
	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2addon\\shell\\open\\command", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)command, strlen(command));
	RegCloseKey(hKey);
	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2addon", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)"URL: bs2mod Protocol", strlen("URL: bs2addon Protocol"));
	RegSetValueExA(hKey, "URL Protocol", 0, REG_SZ, (BYTE*)"", strlen(""));
	RegCloseKey(hKey);
	RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Classes\\bs2addon\\DefaultIcon", &hKey);
	RegSetValueExA(hKey, "", 0, REG_SZ, (BYTE*)filename, strlen(filename));
	RegCloseKey(hKey);

	DragAcceptFiles(GetActiveWindow(), true);
}

void sysmessage(SDL_SysWMmsg* msg) {
	SDL_SysWMmsg* m = msg;
	if (m->msg == WM_DROPFILES) {
		char tmp[256];
		int count = DragQueryFile((HDROP__*)m->wParam, 0xFFFFFFFF, 0, 0);
		for (int i = 0; i < count; i++) {
			DragQueryFileA((HDROP__*)m->wParam, i, tmp, 255);
			parsefile(tmp, 0);
		}
	}
}

void ossystem(char* cmd, char* parameters, bool wait, bool hidden) {
	if (parameters) {
		SHELLEXECUTEINFOA t;
		ZeroMemory(&t, sizeof(SHELLEXECUTEINFO));
		t.cbSize = sizeof(SHELLEXECUTEINFO);
		t.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT;
		t.hwnd = GetActiveWindow();
		t.lpVerb = "open";
		t.lpFile = cmd;
		t.lpParameters = parameters;
		if (hidden) t.nShow = SW_HIDE;
		else t.nShow = SW_SHOWNORMAL;

		if (ShellExecuteExA(&t) && wait)
			while (WaitForSingleObject(t.hProcess, INFINITE) != WAIT_OBJECT_0);
	} else {
		std::ofstream batchfile;
		batchfile.open("tmp.bat", std::ios::out);
		batchfile.write(cmd, strlen(cmd));
		batchfile.close();
		ossystem("tmp.bat", "", true, hidden);
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
			SendMessage(hwnd, WM_KEYUP, i, 0);
	}
	hwnd = GetActiveWindow();
}

int copyStringToClipboard(char* source) {
	int ok = OpenClipboard(NULL);
	if (!ok) return 0;
	HGLOBAL clipbuffer;
	char* buffer;
	EmptyClipboard();
	clipbuffer = GlobalAlloc(GMEM_DDESHARE, strlen(source) + 1);
	buffer = (char*)GlobalLock(clipbuffer);
	strcpy(buffer, source);
	GlobalUnlock(clipbuffer);
	SetClipboardData(CF_TEXT, clipbuffer);
	CloseClipboard();
	return 0;
}

char* getStringFromClipboard() {
	int ok = OpenClipboard(NULL);
	char* buffer = NULL;
	if (!ok) return NULL;
	HANDLE hData = GetClipboardData(CF_TEXT);
	buffer = (char*)GlobalLock(hData);
	GlobalUnlock(hData);
	CloseClipboard();
	return buffer;
}

char* opendialog(char* filter, char* defaultname) {
	OPENFILENAMEA ofn;
	char* szFile = new char[260];
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ZeroMemory(szFile, 260);
	if (defaultname)
		strcpy(szFile, defaultname);
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetActiveWindow();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = 260;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileNameA(&ofn) == TRUE) {
		mousebuttonbug(true);
		return szFile;
	}
	mousebuttonbug(true);
	return 0;
}

char* savedialog(char* filter, char* defaultname) {
	OPENFILENAMEA ofn;
	char* szFile = new char[260];
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ZeroMemory(szFile, 260);
	if (defaultname)
		strcpy(szFile, defaultname);
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetActiveWindow();
	ofn.hInstance = (HINSTANCE)GetCurrentProcess();
	ofn.Flags = OFN_HIDEREADONLY | OFN_NONETWORKBUTTON | OFN_NODEREFERENCELINKS | OFN_NOCHANGEDIR;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = 258;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = "";
	if (GetSaveFileNameA(&ofn)) {
		mousebuttonbug(true);
		if (strlen(szFile)) return szFile;
		else return 0;
	}
	mousebuttonbug(true);
	return 0;
}

bool yesnobox(char* text, char* title) {
	int i = MessageBoxA(GetActiveWindow(), text, title, MB_YESNO);
	mousebuttonbug(true);
	if (i == IDYES) return true;
	return false;
}

void messagebox(char* text, char* title) {
	MessageBoxA(GetActiveWindow(), text, title, MB_OK);
	mousebuttonbug(true);
}

char* inputbox() {
	return 0;
}

#else

void osinit(char* filename) {
	path = new char[strlen(filename) + 1];
	strcpy(path, filename);
	char* tmp = 0;
	char* tmp2 = path;
	while ((tmp = strchr(tmp2, '/')) || (tmp = strchr(tmp2, '\\')))
		tmp2 = tmp + 1;
	*tmp2 = 0;
}

void sysmessage(SDL_SysWMmsg* msg) {
}

void ossystem(char* cmd, char* parameters, bool wait, bool hidden) {
#ifdef COMPILER_SYSTEM
	system(cmd);
#endif
}

void mousebuttonbug(bool mouseup) {
}

int copyStringToClipboard(char* source) {
	return 0;
}

char* getStringFromClipboard() {
	char* buffer = new char[1];
	buffer[0] = 0;
	return buffer;
}

char* opendialog(char* filter, char* defaultname) {
	char* buffer = new char[1];
	buffer[0] = 0;
	return buffer;
}

char* savedialog(char* filter, char* defaultname) {
	char* buffer = new char[1];
	buffer[0] = 0;
	return buffer;
}

bool yesnobox(char* text, char* title) {
	return false;
}

void messagebox(char* text, char* title) {
}

char* inputbox() {
	return 0;
}

#endif

char* checkfilename(char* filename) {
	return filename;
};