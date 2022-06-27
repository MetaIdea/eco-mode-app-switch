#pragma once
#include <windows.h>
#include <iostream>
#include <WinBase.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <cstring>
#include <string>
#include <vector>
using namespace std;

#define APP_LIST_START_ID WM_APP + 4
#define MAX_MENU_ITEMS 300

struct ProcessInfoData {
	size_t MenuItemID;
	DWORD ProcessId;
	wstring ExeName;
	wstring WindowTitle;
	bool ThreadSuspended;
	wstring MenuString;
};

std::vector<ProcessInfoData> ProcessInfo;

DWORD FindProcessId(const std::wstring& processName)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	Process32First(processesSnapshot, &processInfo);
	if (!processName.compare(processInfo.szExeFile)) {
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processesSnapshot, &processInfo)) {
		if (!processName.compare(processInfo.szExeFile)) {
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot);
	return 0;
}

typedef LONG(NTAPI* NtSuspendProcess)(IN HANDLE ProcessHandle);
NtSuspendProcess NtSuspendProcessAction = (NtSuspendProcess)GetProcAddress(GetModuleHandle((LPCWSTR)L"ntdll"), "NtSuspendProcess");

typedef LONG(NTAPI* NtResumeProcess)(IN HANDLE ProcessHandle);
NtResumeProcess NtResumeProcessAction = (NtResumeProcess)GetProcAddress(GetModuleHandle((LPCWSTR)L"ntdll"), "NtResumeProcess");

bool SuspendResumeProcess(size_t MenuItemIDTrigger) {
	for (int i = 0; i < ProcessInfo.size(); i++) {
		if (ProcessInfo[i].MenuItemID == MenuItemIDTrigger) {
			DWORD ProcessId_ = ProcessInfo[i].ProcessId; // FindProcessId(ProcessInfo[i].ExeName); //ProcessInfo[i].ProcessId
			if (ProcessId_) {
				HANDLE ProcessHandle = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, ProcessId_);
				if (ProcessHandle) {
					if (ProcessInfo[i].ThreadSuspended) {
						ProcessInfo[i].ThreadSuspended = false;
						NtResumeProcessAction(ProcessHandle);
						//OpenIcon(FindWindow(NULL, ProcessInfo[i].WindowTitle.c_str()));
					}
					else {
						CloseWindow(FindWindow(NULL, ProcessInfo[i].WindowTitle.c_str()));
						NtSuspendProcessAction(ProcessHandle);
						ProcessInfo[i].ThreadSuspended = true;
					}
					CloseHandle(ProcessHandle);
					return true;
				}
			}
			break;
		}
	}
	return false;
}

BOOL CALLBACK StoreWindowNames(HWND hwnd, LPARAM lParam) {
	const DWORD TITLE_SIZE = 1024;
	WCHAR windowTitle[TITLE_SIZE];

	GetWindowTextW(hwnd, windowTitle, TITLE_SIZE);

	int length = ::GetWindowTextLength(hwnd);
	wstring title(&windowTitle[0]);
	if (!IsWindowVisible(hwnd) || length == 0 || title == L"Program Manager") {
		return TRUE;
	}

	std::vector<std::wstring>& titles = *reinterpret_cast<std::vector<std::wstring>*>(lParam);
	titles.push_back(title);

	return TRUE;
}

int RefreshAppList(HMENU hMenu) {
	std::vector<ProcessInfoData> NewProcessInfo;
	//ProcessInfo.clear();
	std::vector<std::wstring> titles;
	EnumWindows(StoreWindowNames, reinterpret_cast<LPARAM>(&titles));
	size_t MenuItems = 0;
	for (const auto& title : titles) {
		HWND hWnd = FindWindow(NULL, title.c_str());
		DWORD ProcessIdFound = 0;
		DWORD Thread = GetWindowThreadProcessId(hWnd, &ProcessIdFound);

		bool IsDuplicate = false;
		for (int i = 0; i < NewProcessInfo.size(); i++) {
			if (NewProcessInfo[i].ProcessId == ProcessIdFound) {
				IsDuplicate = true;
			}
		}

		if (Thread && !IsDuplicate) {
			HANDLE ProcessHandle = OpenProcess(PROCESS_SUSPEND_RESUME | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, ProcessIdFound);
			//DWORD StaticProcessID = GetProcessId(ProcessHandle); //FindProcessId(const std::wstring& processName)

			if (ProcessHandle) {
				WCHAR lpExePath[MAX_PATH];
				DWORD lpExeNameSize = MAX_PATH;
				QueryFullProcessImageName(ProcessHandle, 0, lpExePath, &lpExeNameSize);

				wstring TitleCleaned = wstring(title);
				auto lastpart = TitleCleaned.find_last_of(L"-");
				if (lastpart != std::wstring::npos) {
					TitleCleaned = wstring(title);
					TitleCleaned = TitleCleaned.substr(lastpart + 1);

				}
				auto start = TitleCleaned.find_first_not_of(L" \r\n\t\v\f");
				auto end = TitleCleaned.find_last_not_of(L" \r\n\t\v\f");
				if (start != std::wstring::npos && end != std::wstring::npos) {
					TitleCleaned = TitleCleaned.substr(start, end - start + 1);
				}
				if (TitleCleaned.find_first_not_of(' ') == std::string::npos) {
					TitleCleaned = L"";
				}

				if (wcslen(lpExePath) > 0) {
					ProcessInfoData ProcessInfoItem {
						APP_LIST_START_ID + MenuItems,
						ProcessIdFound,
						wstring(lpExePath).substr(wstring(lpExePath).find_last_of(L"/\\") + 1),
						title,
						false,
						L"",
					};
					if (TitleCleaned.size() > 0) {
						ProcessInfoItem.MenuString = TitleCleaned + L" | " + ProcessInfoItem.ExeName;
						//SHOW PID
						//ProcessInfoItem.MenuString = TitleCleaned + L" | " + ProcessInfoItem.ExeName + L" | PID:" + to_wstring(ProcessIdFound);
					}
					else {
						ProcessInfoItem.MenuString = ProcessInfoItem.ExeName;
					}

					if (!(ProcessInfoItem.ExeName == L"explorer.exe")) {
						MenuItems++;
						NewProcessInfo.push_back(ProcessInfoItem);
					}
				}

				CloseHandle(ProcessHandle);
			}
		}
	}
	bool SuspendedAppsDetected = false;

	//All valid normal items
	for (int i = 0; i < NewProcessInfo.size(); i++) {
		for (int j = 0; j < ProcessInfo.size(); j++) {
			if (NewProcessInfo[i].ProcessId == ProcessInfo[j].ProcessId) {
				if (ProcessInfo[j].ThreadSuspended) {
					SuspendedAppsDetected = true;
					NewProcessInfo[i].ThreadSuspended = true;
				}
				break;
			}
		}
		if (!NewProcessInfo[i].ThreadSuspended) {
			InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_UNCHECKED, NewProcessInfo[i].MenuItemID, NewProcessInfo[i].MenuString.c_str());
		}
	}

	//Seperator
	if (SuspendedAppsDetected) {
		InsertMenu(hMenu, APP_LIST_START_ID + MenuItems, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
		MenuItems++;

		//Add all suspended items from NewProcessInfo
		for (int i = 0; i < NewProcessInfo.size(); i++) {
			if (NewProcessInfo[i].ThreadSuspended) {
				InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_CHECKED, NewProcessInfo[i].MenuItemID, NewProcessInfo[i].MenuString.c_str());
			}
		}

		//Add all suspended and only in ProcessInfo not available from NewProcessInfo
		for (int i = 0; i < ProcessInfo.size(); i++) {
			if (ProcessInfo[i].ThreadSuspended) {
				bool IsInProcessInfo = false;
				for (int j = 0; j < NewProcessInfo.size(); j++) {
					if (ProcessInfo[i].ProcessId == NewProcessInfo[j].ProcessId) {
						IsInProcessInfo = true;
						break;
					}
				}
				if (!IsInProcessInfo) {
					ProcessInfo[i].MenuItemID = APP_LIST_START_ID + MenuItems;
					MenuItems++;
					InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_CHECKED, ProcessInfo[i].MenuItemID, ProcessInfo[i].MenuString.c_str());
					NewProcessInfo.push_back(ProcessInfo[i]);
				}
			}
		}
	}

	ProcessInfo.clear();
	ProcessInfo = NewProcessInfo;
}












//UNUSED

BOOL SuspendResumeProcessToggle(DWORD ProcessId, bool Suspend) {
	//DWORD ProcessId = FindProcessId(processName);

	HANDLE snHandle = NULL;
	BOOL rvBool = FALSE;
	THREADENTRY32 te32 = { 0 };

	snHandle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snHandle == INVALID_HANDLE_VALUE) return (FALSE);

	te32.dwSize = sizeof(THREADENTRY32);
	if (Thread32First(snHandle, &te32)) {
		do {
			if (te32.th32OwnerProcessID == ProcessId) {
				HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
				if (hThread) {
					if (!Suspend) {
						ResumeThread(hThread);
					}
					else {
						SuspendThread(hThread);
					}

					CloseHandle(hThread);
				}
			}
		} while (Thread32Next(snHandle, &te32));
		rvBool = TRUE;
	}
	else {
		rvBool = FALSE;
	}
	CloseHandle(snHandle);
	return (rvBool);
}


bool SuspendResumeProcessAlt(size_t MenuItemIDTrigger) {
	for (int i = 0; i < ProcessInfo.size(); i++) {
		if (ProcessInfo[i].MenuItemID == MenuItemIDTrigger) {
			DWORD ProcessId_ = FindProcessId(ProcessInfo[i].ExeName); //ProcessInfo[i].ProcessId
			if (ProcessId_) {
				if (ProcessInfo[i].ThreadSuspended) {
					SuspendResumeProcessToggle(ProcessId_, false);
					//ResumeThread(ProcessHandle);
					ProcessInfo[i].ThreadSuspended = false;
					OpenIcon(FindWindow(NULL, ProcessInfo[i].WindowTitle.c_str()));
				}
				else {
					CloseWindow(FindWindow(NULL, ProcessInfo[i].WindowTitle.c_str()));
					SuspendResumeProcessToggle(ProcessId_, true);
					//SuspendThread(ProcessHandle);
					ProcessInfo[i].ThreadSuspended = true;
				}
				return true;
			}
			break;
		}
	}
	return false;
}

//BOOLEAN PhGetProcessIsSuspended(_In_ PSYSTEM_PROCESS_INFORMATION Process) {
//	ULONG i;
//
//	for (i = 0; i < Process->NumberOfThreads; i++){
//		if (Process->Threads[i].ThreadState != Waiting || Process->Threads[i].WaitReason != Suspended) {
//			return FALSE;
//		}
//	}
//
//	return Process->NumberOfThreads != 0;
//}
//
//BOOLEAN PhIsProcessSuspended(_In_ HANDLE ProcessId) {
//	BOOLEAN suspended = FALSE;
//	PVOID processes;
//	PSYSTEM_PROCESS_INFORMATION process;
//
//	if (NT_SUCCESS(PhEnumProcesses(&processes)))
//	{
//		if (process = PhFindProcessInformation(processes, ProcessId))
//		{
//			suspended = PhGetProcessIsSuspended(process);
//		}
//
//		PhFree(processes);
//	}
//
//	return suspended;
//}

int RefreshAppListAlt(HMENU hMenu) {
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	Process32First(processesSnapshot, &processInfo);
	InsertMenuW(hMenu, -1, MF_BYPOSITION, 999, processInfo.szExeFile);

	while (Process32Next(processesSnapshot, &processInfo)) {
		InsertMenuW(hMenu, -1, MF_BYPOSITION, 999, processInfo.szExeFile);
	}

	CloseHandle(processesSnapshot);
	return 1;
}

