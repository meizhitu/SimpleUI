#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include "stdio.h"
#pragma comment(lib,"kernel32")
#pragma comment(lib,"user32")
#pragma comment(lib,"Advapi32.lib")


//提升进程访问权限
bool enableDebugPriv()
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
			return false;
	}
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue)) {
		CloseHandle(hToken);
		return false;
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL)) {
		CloseHandle(hToken);
		return false;
	}
	return true;
}

//根据进程名称得到进程ID,如果有多个运行实例的话，返回第一个枚举到的进程的ID
DWORD processNameToId(LPCTSTR lpszProcessName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe)) {
		MessageBox(NULL, 
			"The frist entry of the process list has not been copyied to the buffer", 
			"Notice", MB_ICONINFORMATION | MB_OK);
		return 0;
	}
	while (Process32Next(hSnapshot, &pe)) {
		if (!strcmp(lpszProcessName, pe.szExeFile)) {
			return pe.th32ProcessID;
		}
	}

	return 0;
}
int main(int argc, char* argv[])
{
	//定义线程体的大小
	const DWORD dwThreadSize = 4096;
	DWORD dwWriteBytes;
	//提升进程访问权限
	enableDebugPriv();
	//等待输入进程名称，注意大小写匹配
	char szExeName[MAX_PATH] = { 0 };
	//    cout<< "Please input the name of target process !" <<endl;
	//    
	//    cin >> szExeName;
	// cout<<szExeName<<endl;
	//strcpy(szExeName,"notepad.exe");
	//scanf("%s",szExeName);
	strcpy(szExeName,"explorer.exe");  

	DWORD dwProcessId = processNameToId(szExeName);
	if (dwProcessId == 0) {
		MessageBox(NULL, "The target process have not been found !",
			"Notice", MB_ICONINFORMATION | MB_OK);
		return -1;
	}
	//根据进程ID得到进程句柄
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);

	if (!hProcess) {
		MessageBox(NULL, "Open target process failed !", 
			"Notice", MB_ICONINFORMATION | MB_OK);
		return 0;
	}

	HANDLE hThread;
	char   szLibPath [_MAX_PATH];
	void*  pLibRemote = 0;	// the address (in the remote process) where
							// szLibPath will be copied to;
	DWORD  hLibModule = 0;	// base adress of loaded module (==HMODULE);

	HMODULE hKernel32 = ::GetModuleHandle("Kernel32");
	strcpy( szLibPath,"F:\\VSWorkSpace\\SimpleUI\\Remote.dll\0" );


	// 1. Allocate memory in the remote process for szLibPath
	// 2. Write szLibPath to the allocated memory
	pLibRemote = ::VirtualAllocEx( hProcess, NULL, sizeof(szLibPath), MEM_COMMIT, PAGE_READWRITE );
	if( pLibRemote == NULL )
		return false;
	::WriteProcessMemory(hProcess, pLibRemote, (void*)szLibPath,sizeof(szLibPath),NULL);


	hThread = ::CreateRemoteThread( hProcess, NULL, 0,	
					(LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,"LoadLibraryA"), 
					pLibRemote, 0, NULL );
	if( hThread == NULL )
		goto JUMP;

	::WaitForSingleObject( hThread, INFINITE );

	// Get handle of loaded module
	::GetExitCodeThread( hThread, &hLibModule );
	::CloseHandle( hThread );

JUMP:	
	//::VirtualFreeEx( hProcess, pLibRemote, sizeof(szLibPath), MEM_RELEASE );
	//if( hLibModule == NULL )
	//	return false;
	

	// Unload "LibSpy.dll" from the remote process 
	// (via CreateRemoteThread & FreeLibrary)
	//hThread = ::CreateRemoteThread( hProcess,
  //              NULL, 0,
  //              (LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,"FreeLibrary"),
  //              (void*)hLibModule,
  //               0, NULL );
	//if( hThread == NULL )	// failed to unload
	//	return false;

	//::WaitForSingleObject( hThread, INFINITE );
	//::GetExitCodeThread( hThread, &hLibModule );
	//::CloseHandle( hThread );

	// return value of remote FreeLibrary (=nonzero on success)
	return hLibModule;
}
