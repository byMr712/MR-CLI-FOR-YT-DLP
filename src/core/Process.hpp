#pragma once
#include "WinLibs.hpp"
#include "Handle.hpp"
#include <string>
#include <string_view>
#include <functional>

namespace mrcli::core {

struct ProcessLaunchResult {
    UniqueHandle process;
    UniqueHandle thread;
    UniqueHandle readPipe;
    DWORD processId{0};
    bool success{false};
    DWORD errorCode{0};
};

class Process {
public:
    Process() = default;
    ~Process();

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    bool spawnWithPipes(const std::wstring& commandLine);
    void suspend() noexcept;
    void resume() noexcept;
    void terminate(UINT exitCode = 1) noexcept;
    bool readOutput(char* buffer, size_t bufferSize, size_t& bytesRead);
    bool isRunning() const noexcept;
    int waitForExit();

    static int executeWait(const std::wstring& cmd);
    static ProcessLaunchResult launchRedirected(const std::wstring& commandLine);
    static void suspendProcess(HANDLE hProcess) noexcept;
    static void resumeProcess(HANDLE hProcess) noexcept;
    static void terminateProcess(HANDLE hProcess, UINT exitCode = 1) noexcept;

private:
    UniqueHandle m_process;
    UniqueHandle m_thread;
    UniqueHandle m_readPipe;
    DWORD m_processId{0};
};

} // namespace mrcli::core
