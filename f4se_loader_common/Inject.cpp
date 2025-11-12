#include "LoaderError.h"
#include "IdentifyEXE.h"
#include "common/IFileStream.h"

// remote thread creation

static bool DoInjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync, bool noTimeout);

bool InjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync, bool noTimeout)
{
    bool    result = false;

    // wrap DLL injection in SEH, if it crashes print a message
    
    // Mobi's Note:
    // https://learn.microsoft.com/en-us/cpp/cpp/structured-exception-handling-c-cpp?view=msvc-170
    // Structured Exception Handling can handle more than just C++ exceptions.
    // It can catch things that would normally make your process crash, like if
    // an antivirus tries to kill the loader, or something else prevents us
    // from injecting the DLL. This way, the user still gets a friendlier error
    // message, instead of a dead program.
    __try {
        result = DoInjectDLLThread(info, dllPath, sync, noTimeout);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        PrintLoaderError("DLL injection failed. In most cases, this is caused by an overly paranoid software firewall or antivirus package. Disabling either of these may solve the problem.");
        result = false;
    }

    return result;
}



// Mobi's Notes:
//
// In order to get the game to load our DLL, we inject some assembly code. I'll walk you through it.
//
// # Good information to know:
//   ## The x64 Windows Calling Convention
//   - First 4 parameters of a function are stored in: RCX, RDX, R8, R9
//   - Return value will be stored in: RAX
//   - Shadow space: Caller must reserve 32 bytes on stack for callee
//   - Non-volatile registers: RBX, RBP, RDI, RSI, R12-R15 (must be preserved)
//   - Volatile registers: RAX, RCX, RDX, R8-R11 (can be destroyed by callee)
//
//   ## Registers when we start execution:
//     RCX register = pointer to InjectDLLThreadData struct (the thread parameter)
//     RIP register = pointing at our code
//     RSP register = stack pointer
//
//   ## Win32 API Functions Used
//   LoadLibraryA - Takes a path string to a .dll file, loads it and returns a handle (A sort of ID) to that loaded library
//   getProcAddress - Takes a handle and (in this case) a literal number 1. It fetches the address of the first exported function in the DLL. In this case, it's the xSE DLL's initialization function.
//
//   ## Memory layout at the address in RCX:
//      Offset 0x00:  loadLibraryA      (function pointer)
//      Offset 0x08:  getProcAddress    (function pointer)
//      Offset 0x10:  dllPath           (string: "F:\...\f4se_[VERSION].dll")
//      Offset 0x810: code              (the assembly code itself)
//
//   Phew! Now here's the actual assembly:

/*

  | Bytes                    | Assembly                                | Notes (IanPatt)                                  | Notes (Mobi)
  |--------------------------|-----------------------------------------|--------------------------------------------------|-------------------------------------------------------------------------------------------------------|
  | 48:83EC 28               | sub rsp, 28                             |                                                  | Make room on the stack 0x28 bytes. 0x20 is required, 0x8 to make stack 16 byte aligned                |
  | 48:8BD9                  | mov rbx,rcx                             | save to non-volatile register                    | Copy rcx (volatile register) into rbx, so that it's safe for later                                    |
  | 48:8D4B 10               | lea rcx,qword ptr ds:[rbx+10]           | offsetof(InjectDLLThreadData, dllPath)           | Load the Effective Address (LEA) of the path to our dll into RCX                                      |
  | FF13                     | call qword ptr ds:[rbx]                 | offsetof(InjectDLLThreadData, loadLibraryA)      | Call the function at the pointer in RBX. (LoadLibraryA:      ^^^  the path is our parameter)          |
  | 48:8BC8                  | mov rcx,rax                             |                                                  | Copy the returned handle from the previous call into RCX (we're going to use it as 1st parameter now) |
  | BA 01000000              | mov edx,1                               |                                                  | Move the literal number 1 into EDX (same register as RDX, but just the first 32 bits, instead of 64)  |
  | FF53 08                  | call qword ptr ds:[rbx+8]               | offsetof(InjectDLLThreadData, getProcAddress)    | Call the function getProcAddress (see description above), returns function pointer into RAX.          |
  | FFD0                     | call rax                                |                                                  | Call the function we just found (xSE's initialization function)                                       |
  | 48:83C4 28               | add rsp, 28                             |                                                  | Remove the shadow space we set up at the beginning                                                    |
  | C3                       | ret                                     |                                                  | Return (Cave Johnson! We're done here!)                                                               |
*/

// Too much? Don't worry. It roughly translates to the following C code:
//
// void __stdcall BootstrapCode(InjectDLLThreadData* data)
// {
//     // Call LoadLibraryA to load the DLL
//     HMODULE dll = data->loadLibraryA(data->dllPath);
//
//     // Call GetProcAddress to find F4SE's initialization function
//     void* initFunc = data->getProcAddress(dll, 1);  // Ordinal 1
//
//     // Call F4SE's initialization function
//     initFunc();
// }

struct InjectDLLThreadData
{
    InjectDLLThreadData(uintptr_t _loadLibraryA, uintptr_t _getProcAddress, const char * _dllPath)
    {
        memset(this, 0, sizeof(*this));
        loadLibraryA = _loadLibraryA;
        getProcAddress = _getProcAddress;
        strcpy_s(dllPath, sizeof(dllPath), _dllPath);
        
        static const UInt8 kCode[] =
        {
            0x48, 0x83, 0xEC, 0x28,
            0x48, 0x8B, 0xD9,
            0x48, 0x8D, 0x4B, 0x10,
            0xFF, 0x13,
            0x48, 0x8B, 0xC8,
            0xBA, 0x01, 0x00, 0x00, 0x00,
            0xFF, 0x53, 0x08,
            0xFF, 0xD0,
            0x48, 0x83, 0xC4, 0x28,
            0xC3,
        };
        
        memcpy(code, kCode, sizeof(kCode));
    }
    
    uintptr_t   loadLibraryA;       // 00
    uintptr_t   getProcAddress;     // 08
    char        dllPath[2048];      // 10
    UInt8       code[128];          // 810
};


// Inject the xSE DLL into the game. Returns whether injection was successful.
static bool DoInjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync, bool noTimeout)
{
    bool    result = false;

    // make sure the dll exists
    {
        IFileStream fileCheck;
        if(!fileCheck.Open(dllPath))
        {
            PrintLoaderError("Couldn't find %s.", dllPath);
            return false;
        }
    }

    // Mobi's notes
    // Given the process information stored in info, try to open the process for manipulation
    HANDLE  process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, info->dwProcessId);
    if(process)
    {
        // If we get it, allocate 8kb of space for the struct with the assembly code explained above, and mark it as readable, writeable and executable.
        uintptr_t   hookBase = (uintptr_t)VirtualAllocEx(process, NULL, 8192, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if(hookBase)
        {
            // safe because kernel32 is loaded at the same address in all processes
            // (can change across restarts)
            //
            //
            // Mobi's notes:
            // We do have to pass our code the location of LoadLibraryA and
            // getProcAddress, but as explained in the comment above, these are
            // part of kernel32.dll, which is at the same location for every
            // process. That means we can just fetch them from our own process,
            // and use those addresses.
            uintptr_t   loadLibraryAAddr = (uintptr_t)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
            uintptr_t   getProcAddressAddr = (uintptr_t)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetProcAddress");

            // Do some logging to the console
            _MESSAGE("hookBase = %016I64X", hookBase);
            _MESSAGE("loadLibraryAAddr = %016I64X", loadLibraryAAddr);
            _MESSAGE("getProcAddressAddr = %016I64X", getProcAddressAddr);
          
            // We create the struct with our addresses and assembly, as explained above.
            InjectDLLThreadData data(loadLibraryAAddr, getProcAddressAddr, dllPath);

            // And write it into the 8kb of memory we created earlier.
            size_t  bytesWritten;
            WriteProcessMemory(process, (LPVOID)hookBase, (void *)&data, sizeof(data), &bytesWritten);
            
            // We'd like to refer to the start of that remote written memory as the struct we defined, rather than raw memory
            auto * remoteData = (InjectDLLThreadData *)hookBase;
            
            // Now we'll create a thread in the game that runs separately from the main thread. We'll tell it to start execution at our little bootstrap code.
            HANDLE  thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)(&remoteData->code), (void *)remoteData, 0, NULL);
            // If we succeed, we wait for a response from the process. If we time out, something went wrong.
            if(thread)
            {
                if(sync)
                {
                    switch(WaitForSingleObject(thread, noTimeout ? INFINITE : 1000 * 60))   // timeout = one minute
                    {
                    case WAIT_OBJECT_0:
                        _MESSAGE("hook thread complete");
                        result = true;
                        break;

                    case WAIT_ABANDONED:
                        _ERROR("Process::InstallHook: waiting for thread = WAIT_ABANDONED");
                        break;

                    case WAIT_TIMEOUT:
                        _ERROR("Process::InstallHook: waiting for thread = WAIT_TIMEOUT");
                        break;
                    }
                }
                else
                    result = true;

                CloseHandle(thread);
            }
            else
                _ERROR("CreateRemoteThread failed (%d)", GetLastError());

            VirtualFreeEx(process, (LPVOID)hookBase, 0, MEM_RELEASE);
        }
        else
            _ERROR("Process::InstallHook: couldn't allocate memory in target process");

        CloseHandle(process);
    }
    else
        _ERROR("Process::InstallHook: couldn't get process handle");

    return result;
}
