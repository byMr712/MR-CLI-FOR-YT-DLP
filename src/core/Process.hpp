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
    static ProcessLaunchResult launchRedirected(const std::wstring& commandLine);

    static void suspendProcess(HANDLE hProcess) noexcept;
    static void resumeProcess(HANDLE hProcess) noexcept;
    static void terminateProcess(HANDLE hProcess, UINT exitCode = 1) noexcept;
};

} // namespace mrcli::core
