#pragma once
#include "WinLibs.hpp"
#include <utility>

namespace mrcli::core {

class UniqueHandle {
public:
    UniqueHandle() noexcept : m_handle(INVALID_HANDLE_VALUE) {}
    explicit UniqueHandle(HANDLE h) noexcept : m_handle(h) {}
    
    ~UniqueHandle() noexcept {
        reset();
    }

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = INVALID_HANDLE_VALUE;
    }

    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            reset();
            m_handle = other.m_handle;
            other.m_handle = INVALID_HANDLE_VALUE;
        }
        return *this;
    }

    [[nodiscard]] HANDLE get() const noexcept { return m_handle; }
    [[nodiscard]] bool isValid() const noexcept { return m_handle != NULL && m_handle != INVALID_HANDLE_VALUE; }
    explicit operator bool() const noexcept { return isValid(); }

    HANDLE release() noexcept {
        HANDLE tmp = m_handle;
        m_handle = INVALID_HANDLE_VALUE;
        return tmp;
    }

    void reset(HANDLE h = INVALID_HANDLE_VALUE) noexcept {
        if (isValid()) {
            CloseHandle(m_handle);
        }
        m_handle = h;
    }

    HANDLE* addressof() noexcept { return &m_handle; }

private:
    HANDLE m_handle{INVALID_HANDLE_VALUE};
};

class ScopedHInternet {
public:
    ScopedHInternet() noexcept : m_handle(nullptr) {}
    explicit ScopedHInternet(HINTERNET h) noexcept : m_handle(h) {}

    ~ScopedHInternet() noexcept {
        reset();
    }

    ScopedHInternet(const ScopedHInternet&) = delete;
    ScopedHInternet& operator=(const ScopedHInternet&) = delete;

    ScopedHInternet(ScopedHInternet&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    ScopedHInternet& operator=(ScopedHInternet&& other) noexcept {
        if (this != &other) {
            reset();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    [[nodiscard]] HINTERNET get() const noexcept { return m_handle; }
    [[nodiscard]] bool isValid() const noexcept { return m_handle != nullptr; }
    explicit operator bool() const noexcept { return isValid(); }

    void reset(HINTERNET h = nullptr) noexcept {
        if (m_handle) {
            InternetCloseHandle(m_handle);
            m_handle = nullptr;
        }
        m_handle = h;
    }

private:
    HINTERNET m_handle{nullptr};
};

class ScopedCoInit {
public:
    explicit ScopedCoInit(DWORD coInitFlags = COINIT_APARTMENTTHREADED) noexcept {
        m_hr = CoInitializeEx(nullptr, coInitFlags);
    }

    ~ScopedCoInit() noexcept {
        if (SUCCEEDED(m_hr)) {
            CoUninitialize();
        }
    }

    ScopedCoInit(const ScopedCoInit&) = delete;
    ScopedCoInit& operator=(const ScopedCoInit&) = delete;

    [[nodiscard]] bool succeeded() const noexcept { return SUCCEEDED(m_hr); }
    [[nodiscard]] HRESULT result() const noexcept { return m_hr; }

private:
    HRESULT m_hr{E_FAIL};
};

} // namespace mrcli::core
