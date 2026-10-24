#include "HttpClient.hpp"
#include "WinLibs.hpp"
#include "Handle.hpp"
#include "StringUtil.hpp"
#include <fstream>
#include <vector>

namespace mrcli::core {

bool HttpClient::downloadFile(
    std::string_view url,
    const PathArg& destinationPath,
    ProgressCallback onProgress
) {
    std::wstring wurl = StringUtil::utf8ToWstring(url);

    ScopedHInternet hInternet(InternetOpenW(
        L"MR-CLI-FOR-YT-DLP/1.2",
        INTERNET_OPEN_TYPE_PRECONFIG,
        nullptr,
        nullptr,
        0
    ));
    if (!hInternet) return false;

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE;
    ScopedHInternet hUrl(InternetOpenUrlW(
        hInternet.get(),
        wurl.c_str(),
        nullptr,
        0,
        flags,
        0
    ));

    if (!hUrl) {
        // Fallback without HTTPS flag if non-SSL
        flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
        hUrl.reset(InternetOpenUrlW(
            hInternet.get(),
            wurl.c_str(),
            nullptr,
            0,
            flags,
            0
        ));
        if (!hUrl) return false;
    }

    uint64_t contentLength = 0;
    wchar_t lengthBuf[64] = { 0 };
    DWORD lengthBufSize = sizeof(lengthBuf);
    DWORD index = 0;
    if (HttpQueryInfoW(
        hUrl.get(),
        HTTP_QUERY_CONTENT_LENGTH,
        lengthBuf,
        &lengthBufSize,
        &index
    )) {
        try {
            contentLength = std::stoull(lengthBuf);
        } catch (...) {
            contentLength = 0;
        }
    }

    std::ofstream outFile(destinationPath.path(), std::ios::binary);
    if (!outFile.is_open()) return false;

    std::vector<char> buffer(64 * 1024);
    DWORD bytesRead = 0;
    uint64_t totalDownloaded = 0;

    while (InternetReadFile(hUrl.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead) && bytesRead > 0) {
        outFile.write(buffer.data(), bytesRead);
        totalDownloaded += bytesRead;
        if (onProgress) {
            onProgress(totalDownloaded, contentLength);
        }
    }

    outFile.close();
    return totalDownloaded > 0;
}

} // namespace mrcli::core
