#include "Clipboard.hpp"
#include "WinLibs.hpp"
#include "StringUtil.hpp"

namespace mrcli::core {

std::string Clipboard::getText() {
    if (!OpenClipboard(nullptr)) {
        return {};
    }

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        CloseClipboard();
        return {};
    }

    const auto* pszText = static_cast<const wchar_t*>(GlobalLock(hData));
    if (!pszText) {
        CloseClipboard();
        return {};
    }

    std::wstring wtext(pszText);
    GlobalUnlock(hData);
    CloseClipboard();

    return StringUtil::wstringToUtf8(wtext);
}

bool Clipboard::setText(const std::string& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }

    EmptyClipboard();
    std::wstring wtext = StringUtil::utf8ToWstring(text);
    size_t bytes = (wtext.size() + 1) * sizeof(wchar_t);

    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!hGlob) {
        CloseClipboard();
        return false;
    }

    void* pMem = GlobalLock(hGlob);
    if (!pMem) {
        GlobalFree(hGlob);
        CloseClipboard();
        return false;
    }

    memcpy(pMem, wtext.c_str(), bytes);
    GlobalUnlock(hGlob);

    SetClipboardData(CF_UNICODETEXT, hGlob);
    CloseClipboard();
    return true;
}

} // namespace mrcli::core
