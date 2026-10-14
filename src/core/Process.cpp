#include "Process.hpp"

namespace mrcli::core {

typedef LONG(NTAPI* pfnNtSuspendProcess)(HANDLE ProcessHandle);
typedef LONG(NTAPI* pfnNtResumeProcess)(HANDLE ProcessHandle);

Process::~Process() {
    terminate();
}

bool Process::spawnWithPipes(const std::wstring& commandLine) {
    auto res = launchRedirected(commandLine);
    if (!res.success) return false;
    m_process = std::move(res.process);
    m_thread = std::move(res.thread);
    m_readPipe = std::move(res.readPipe);
    m_processId = res.processId;
    return true;
}

void Process::suspend() noexcept {
    if (m_process) suspendProcess(m_process.get());
}

void Process::resume() noexcept {
    if (m_process) resumeProcess(m_process.get());
}

void Process::terminate(UINT exitCode) noexcept {
    if (m_process) {
        terminateProcess(m_process.get(), exitCode);
    }
}

bool Process::readOutput(char* buffer, size_t bufferSize, size_t& bytesRead) {
    bytesRead = 0;
    if (!m_readPipe) return false;

    DWORD bytesAvail = 0;
    if (!PeekNamedPipe(m_readPipe.get(), nullptr, 0, nullptr, &bytesAvail, nullptr)) {
        return false;
    }

    if (bytesAvail == 0) {
        return true;
    }

    DWORD dwRead = 0;
    DWORD toRead = static_cast<DWORD>((bufferSize < bytesAvail) ? bufferSize : bytesAvail);
    if (!ReadFile(m_readPipe.get(), buffer, toRead, &dwRead, nullptr)) {
        return false;
    }
    bytesRead = dwRead;
    return true;
}

bool Process::isRunning() const noexcept {
    if (!m_process) return false;
    DWORD exitCode = 0;
    if (GetExitCodeProcess(m_process.get(), &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;
}

int Process::waitForExit() {
    if (!m_process) return -1;
    WaitForSingleObject(m_process.get(), INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(m_process.get(), &exitCode);
    m_readPipe.reset();
    m_thread.reset();
    m_process.reset();
    return static_cast<int>(exitCode);
}

int Process::executeWait(const std::wstring& cmd) {
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::wstring mutableCmd = cmd;
    BOOL ok = CreateProcessW(
        nullptr,
        mutableCmd.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );
    if (!ok) return -1;

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return static_cast<int>(exitCode);
}

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
