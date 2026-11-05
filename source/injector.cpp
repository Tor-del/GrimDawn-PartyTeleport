#include "i18n.h"
#include <tlhelp32.h>
#include <cstdio>

static int fail(const wchar_t* message) {
    message = ui::error(message);
    fwprintf(stderr, L"%s (Win32 error %lu)\n", message, GetLastError());
    MessageBoxW(nullptr, message, ui::title, MB_OK | MB_ICONERROR);
    return 1;
}
static DWORD findGame(const std::wstring& expectedPath) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W entry{sizeof(entry)}; DWORD found = 0;
    if (Process32FirstW(snapshot, &entry)) do {
        if (_wcsicmp(entry.szExeFile, L"Grim Dawn.exe")) continue;
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
        if (process) {
            wchar_t path[32768]; DWORD length = 32768;
            if (QueryFullProcessImageNameW(process, 0, path, &length) && !_wcsicmp(path, expectedPath.c_str())) found = entry.th32ProcessID;
            CloseHandle(process);
        }
    } while (!found && Process32NextW(snapshot, &entry));
    CloseHandle(snapshot); return found;
}
static bool otherGameRunning() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return true; // fail closed when the process list is unavailable
    PROCESSENTRY32W entry{sizeof(entry)}; bool found = false;
    if (Process32FirstW(snapshot, &entry)) do {
        if (!_wcsicmp(entry.szExeFile, L"Grim Dawn.exe")) { found = true; break; }
    } while (Process32NextW(snapshot, &entry));
    CloseHandle(snapshot); return found;
}
static bool alreadyLoaded(DWORD pid, const std::wstring& dll) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W entry{sizeof(entry)}; bool found = false;
    if (Module32FirstW(snapshot, &entry)) do {
        if (!_wcsicmp(entry.szExePath, dll.c_str())) { found = true; break; }
    } while (Module32NextW(snapshot, &entry));
    CloseHandle(snapshot); return found;
}
int inject(int argc, wchar_t** argv, DWORD& gamePid) {
    wchar_t path[32768]; GetModuleFileNameW(nullptr, path, 32768);
    std::wstring folder = path; folder.resize(folder.find_last_of(L"\\/"));
    std::wstring root = folder.substr(0, folder.find_last_of(L"\\/"));
    root.resize(root.find_last_of(L"\\/"));
    std::wstring gameDir = root + L"\\x64";
    bool launch = false;
    for (int i = 1; i < argc; ++i) if (!_wcsicmp(argv[i], L"--launch")) launch = true;
    if (argc > 1 && !_wcsicmp(argv[1], L"--check")) {
        bool compatible = supported(gameDir);
        wprintf(L"Game.dll SHA256: "); printf("%s\n", sha256(gameDir + L"\\Game.dll").c_str());
        wprintf(L"Engine.dll SHA256: "); printf("%s\n", sha256(gameDir + L"\\Engine.dll").c_str());
        wprintf(L"DirectInput.dll SHA256: "); printf("%s\n", sha256(gameDir + L"\\DirectInput.dll").c_str());
        wprintf(L"Compatibility: %s\n", compatible ? L"PASS" : L"FAIL (no changes made)");
        return compatible ? 0 : 2;
    }
    if (!supported(gameDir)) return fail(L"Unsupported game version. DLL hashes changed. No changes made. Rebuild the mod for this game version.");
    std::wstring dll = folder + L"\\PartyTeleport.dll", exe = gameDir + L"\\Grim Dawn.exe";
    std::wstring pending = folder + L"\\PartyTeleport.next.dll";
    if (GetFileAttributesW(pending.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (findGame(exe)) return fail(L"Update ready. Close Grim Dawn normally, then run this launcher again to install it.");
        if (!MoveFileExW(pending.c_str(), dll.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            return fail(L"Could not install the pending update. Close the game and retry.");
        puts("Party Teleport update installed.");
    }
    if (GetFileAttributesW(dll.c_str()) == INVALID_FILE_ATTRIBUTES) return fail(L"PartyTeleport.dll is missing beside the launcher.");
    std::wstring mutexName = L"Local\\PartyTeleport.Launch." + root;
    for (size_t i = 6; i < mutexName.size(); ++i) if (mutexName[i] == L'\\' || mutexName[i] == L'/') mutexName[i] = L'_';
    HANDLE launchLock = CreateMutexW(nullptr, FALSE, mutexName.c_str());
    if (!launchLock) return fail(L"Could not create launcher lock.");
    struct Lock { HANDLE h; ~Lock() { ReleaseMutex(h); CloseHandle(h); } } lock{launchLock};
    DWORD locked = WaitForSingleObject(launchLock, 0);
    if (locked != WAIT_OBJECT_0 && locked != WAIT_ABANDONED) {
        puts("Another Party Teleport launcher is already starting/attaching the game."); return 4;
    }
    DWORD pid = findGame(exe);
    HANDLE process{};
    if (!pid) {
        if (!launch) return fail(L"Running x64 Grim Dawn was not found. Start the game through Steam, then open the Party Teleport shortcut.");
        if (otherGameRunning()) return fail(L"Another Grim Dawn process is already running. Use x64 Grim Dawn from this installation. No second copy was started.");
        STARTUPINFOW startup{sizeof(startup)}; PROCESS_INFORMATION info{};
        std::wstring command = L"\"" + exe + L"\"";
        SetEnvironmentVariableW(L"SteamAppId", L"219990");
        if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, root.c_str(), &startup, &info))
            return fail(L"Could not start x64 Grim Dawn. Open Steam, launch the game, then use the mod shortcut again.");
        CloseHandle(info.hThread); CloseHandle(info.hProcess);
        // Steam may replace the bootstrap process. Re-find the exact x64 path.
        for (int i = 0; i < 600 && !pid; ++i) { Sleep(100); pid = findGame(exe); }
        if (!pid) return fail(L"The game did not become available. No additional copy will be started. Launch x64 Grim Dawn through Steam and retry.");
        Sleep(3000);
        process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
            PROCESS_VM_WRITE | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pid);
    } else {
        process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                              PROCESS_VM_WRITE | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pid);
    }
    if (!process) return fail(L"Cannot open the game process. Run the game and launcher at the same permission level.");
    gamePid = pid;
    if (alreadyLoaded(pid, dll)) { CloseHandle(process); puts("Party Teleport is already loaded."); return 0; }
    SIZE_T bytes = (dll.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) { CloseHandle(process); return fail(L"Could not allocate the DLL path in the game process."); }
    if (!WriteProcessMemory(process, remote, dll.c_str(), bytes, nullptr)) {
        VirtualFreeEx(process, remote, 0, MEM_RELEASE); CloseHandle(process); return fail(L"Could not write the DLL path.");
    }
    // Resolve LoadLibraryW relative to its actual owner module in the target process.
    auto load = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    HMODULE owner{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      reinterpret_cast<LPCWSTR>(load), &owner);
    GetModuleFileNameW(owner, path, 32768);
    const wchar_t* ownerName = wcsrchr(path, L'\\'); ownerName = ownerName ? ownerName + 1 : path;
    uintptr_t remoteOwner{};
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W entry{sizeof(entry)};
        if (Module32FirstW(snapshot, &entry)) do {
            if (!_wcsicmp(entry.szModule, ownerName)) { remoteOwner = reinterpret_cast<uintptr_t>(entry.modBaseAddr); break; }
        } while (Module32NextW(snapshot, &entry));
        CloseHandle(snapshot);
    }
    auto remoteLoad = reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteOwner +
                       (reinterpret_cast<uintptr_t>(load) - reinterpret_cast<uintptr_t>(owner)));
    HANDLE thread = remoteOwner ? CreateRemoteThread(process, nullptr, 0, remoteLoad, remote, 0, nullptr) : nullptr;
    if (!thread) {
        VirtualFreeEx(process, remote, 0, MEM_RELEASE); CloseHandle(process); return fail(L"Could not load the mod DLL into the game.");
    }
    DWORD wait = WaitForSingleObject(thread, 15000);
    // On timeout the remote thread may still need its argument; do not free it yet.
    if (wait == WAIT_OBJECT_0) VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(thread); CloseHandle(process);
    if (wait != WAIT_OBJECT_0) return fail(L"DLL loading timed out. Restart the game before retrying.");
    if (!alreadyLoaded(pid, dll)) return fail(L"The game did not load the DLL. Check that 64-bit Grim Dawn is still running.");
    puts("Party Teleport DLL loaded. Checking shared-memory status.");
    return 0;
}
