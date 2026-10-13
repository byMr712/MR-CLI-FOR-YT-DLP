#include "Process.hpp"

namespace mrcli::core {

typedef LONG(NTAPI* pfnNtSuspendProcess)(HANDLE ProcessHandle);
typedef LONG(NTAPI* pfnNtResumeProcess)(HANDLE ProcessHandle);

ProcessLaunchResult Process::launchRedirected(const std::wstring& commandLine) {
    ProcessLaunchResult res;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hReadPipe = nullptr;
    HANDLE hWritePipe = nullptr;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        res.success = false;
        res.errorCode = GetLastError();
        return res;
    }

    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::wstring mutableCmd = commandLine;
    BOOL ok = CreateProcessW(
        nullptr,
        mutableCmd.data(),
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    CloseHandle(hWritePipe);

    if (!ok) {
        CloseHandle(hReadPipe);
        res.success = false;
        res.errorCode = GetLastError();
        return res;
    }

    res.process.reset(pi.hProcess);
    res.thread.reset(pi.hThread);
    res.readPipe.reset(hReadPipe);
    res.processId = pi.dwProcessId;
    res.success = true;
    res.errorCode = 0;

    return res;
}

void Process::suspendProcess(HANDLE hProcess) noexcept {
    if (!hProcess || hProcess == INVALID_HANDLE_VALUE) return;
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (hNtDll) {
        auto pSuspend = reinterpret_cast<pfnNtSuspendProcess>(GetProcAddress(hNtDll, "NtSuspendProcess"));
        if (pSuspend) {
            pSuspend(hProcess);
        }
    }
}

void Process::resumeProcess(HANDLE hProcess) noexcept {
    if (!hProcess || hProcess == INVALID_HANDLE_VALUE) return;
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (hNtDll) {
        auto pResume = reinterpret_cast<pfnNtResumeProcess>(GetProcAddress(hNtDll, "NtResumeProcess"));
        if (pResume) {
            pResume(hProcess);
        }
    }
}

void Process::terminateProcess(HANDLE hProcess, UINT exitCode) noexcept {
    if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
        TerminateProcess(hProcess, exitCode);
    }
}

} // namespace mrcli::core
