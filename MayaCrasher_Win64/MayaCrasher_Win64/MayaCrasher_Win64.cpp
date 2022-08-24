#include <iostream>
#include <vector>
#include <cassert>
#include <windows.h>
#include <tlhelp32.h>

void crash() {
    *(int*) 0 = 0;
}

void CrashPid(DWORD processId) {
    printf("Opening process with PID %u\n", processId);
    
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (process == NULL) {
        printf("Error: Cannot find a process with PID %u", processId);
        return;
    }

    LPVOID arg = (LPVOID)VirtualAllocEx(process, NULL, 256, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (arg == NULL) {
        printf("Error: Cannot allocate memory inside Maya process.\n");
        return;
    }

    int n = WriteProcessMemory(process, arg, crash, 256, NULL);
    if (n == 0) {
        printf("Error: Cannot write memory inside Maya process.\n");
        return;
    }

    HANDLE threadID = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)arg, NULL, NULL, NULL);
    if (threadID == NULL) {
        printf("Error %d: cannot start crash function in Maya.\n", GetLastError());
        return;
    }
        
    printf("Crashed Maya successfully :)\n");
}

bool FindMayaPid(std::vector<DWORD>& pids)
{
  HANDLE hProcessSnap;
  PROCESSENTRY32 pe32;

  pids.clear();
  // Take a snapshot of all processes in the system.
  hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
    printf("CreateToolhelp32Snapshot (of processes)\n");
    return false;
  }

  // Set the size of the structure before using it.
  pe32.dwSize = sizeof( PROCESSENTRY32 );

  if( !Process32First( hProcessSnap, &pe32 ) )
  {
    CloseHandle( hProcessSnap );
    return false;
  }

  do
  {
      if (!strcmp("maya.exe", pe32.szExeFile)) {
          pids.push_back(pe32.th32ProcessID);
      }

  } while( Process32Next( hProcessSnap, &pe32 ) );

  CloseHandle( hProcessSnap );
  return true;
}

void FindByWindow() {
    while (true) {
        BOOL clicked = (GetAsyncKeyState(VK_LBUTTON) & 0x8000u);
        if (!clicked)
            break;
        Sleep(10);
    }
    std::vector<DWORD> valid_pids;

    bool ret = FindMayaPid(valid_pids);
    if (!ret || valid_pids.empty()) {
        printf("maya.exe is not running.\n");
		return;
    }
    
    printf("Please CTRL+Click on the Maya window that is stuck...\n");

    while (true) {
        BOOL clicked = (GetAsyncKeyState(VK_LBUTTON) & 0x8000u);
        BOOL leftCtrl = (GetAsyncKeyState(VK_LCONTROL) & 0x8000u);
        BOOL rightCtrl = (GetAsyncKeyState(VK_RCONTROL) & 0x8000u);
        if (clicked && (leftCtrl || rightCtrl)) {
            break;
        }
        Sleep(10);
    }

    POINT p;
    {
        BOOL ret = GetCursorPos(&p);
        if (!ret) {
            printf("Error: Cannot get cursor position. Exiting.\n");
            return;
        }
    }

    HWND h1 = WindowFromPoint(p);

    if (h1 == NULL) {
        printf("Error: Invalid window. Please try again.\n");
        return FindByWindow();
    }

    DWORD pid = NULL;
    DWORD tid = GetWindowThreadProcessId(h1, &pid);
    if (tid == NULL || pid == NULL) {
        printf("Error: Invalid window. Please try again.\n");
        return FindByWindow();
    }

    for(const auto& i : valid_pids)
        if (i == pid) {
			printf("Detected CTRL+Click on maya.exe with PID=%d\n", pid);
            CrashPid(pid);
            return;
        }

    printf("You clicked on a window that is not Maya. :(\n");
    FindByWindow();
}

int main(int argc, char *argv[])
{
    printf("Welcome to MayaCrasher v1.0\n");
	FindByWindow();

    printf("\nPress any key to exit. ");
    (void) getchar();
    return 0;
}
