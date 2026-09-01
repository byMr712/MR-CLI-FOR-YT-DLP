#include <iostream>
#include <string>
#include <cstdlib>
#include <conio.h>
#include <windows.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <shlobj.h>
#include <commdlg.h>
#include <locale>
#include <cctype>
#include <filesystem>
#include <wininet.h>
#include <winhttp.h>
#include <wincrypt.h>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "Winhttp.lib")
#pragma comment(lib, "Crypt32.lib")

using namespace std;
namespace fs = std::filesystem;

// ========== CONFIG & CONSTANTS ==========
enum ErrorType {
    NOT_ERROR = 0,
    COOKIE_ERROR = 1,
    AGE_ERROR = 2,
    LIVE_ERROR = 3,
    MERGE_ERROR = 4,
    VIDEO_ERROR = 5,
    RATE_LIMIT_ERROR = 6,
    CANCELLED_BY_USER = 7
};

string YTDLP_PATH, FFMPEG_PATH, QJS_PATH, SCRIPT_DIR, DOWNLOAD_PATH, CONFIG_PATH;
string COOKIES_FILE = "cookies.txt";
string VIDEO_RESOLUTION = "1080", VIDEO_FPS = "60", VIDEO_FORMAT = "MP4(H.264)";
string AUDIO_FORMAT = "M4A(AAC)";
bool USE_COOKIES = true, CODEC_RECOMPILER = false, ONLY_AUDIO = false, ONLY_VIDEO = false;
bool YTDLP_FOUND = false, FFMPEG_FOUND = false, QJS_FOUND = false;
int LAST_ERROR = NOT_ERROR;
string ARCHIVE_PATH = "";
int TOTAL_ITEMS_IN_PLAYLIST = 0;
bool PLAYLIST_END_REACHED = false;
int LAST_PROCESSED_ITEM = 0;

// ========== UTF-8 HELPERS ==========
string wstringToUtf8(const wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (size <= 1) return "";
    string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, NULL, NULL);
    return result;
}

wstring utf8ToWstring(const string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (size <= 0) return L"";
    wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
    if (!result.empty() && result.back() == L'\0') result.pop_back();
    return result;
}

// ========== COLORS ==========
enum Color { BLACK = 0, BLUE = 1, GREEN = 2, RED = 4, YELLOW = 6, WHITE = 7, CYAN = 11 };

void setColor(int c) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)c);
}

void printColor(const string& t, int c = WHITE, bool nl = true) {
    setColor(c);
    cout << t;
    setColor(WHITE);
    if (nl) cout << endl;
}

// ========== CONSOLE & SCREEN HELPERS ==========
void setUTF8() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF8");

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
    SetConsoleTitleW(L"MR CLI FOR YT-DLP v1.1.0");
}

void clearScreen() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        cout << "\033[2J\033[H" << flush;
        return;
    }
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        cout << "\033[2J\033[H" << flush;
        return;
    }
    DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD count = 0;
    COORD homeCoords = { 0, 0 };
    FillConsoleOutputCharacterW(hOut, (WCHAR)' ', cellCount, homeCoords, &count);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, homeCoords, &count);
    SetConsoleCursorPosition(hOut, homeCoords);
}

void waitForKey() {
    cout << "\nPress any key...";
    (void)_getch();
    while (_kbhit()) (void)_getch();
    cout << "\n";
}

char getMenuChoice() {
    while (_kbhit()) {
        (void)_getch();
    }
    char c = _getch();
    if (c == 27) return 27; // ESC
    if (c >= 'A' && c <= 'Z') {
        c += 32;
    }
    return c;
}

// ========== CLIPBOARD (DYNAMIC RUNTIME RESOLUTION) ==========
string getClipboard() {
    typedef BOOL(WINAPI* pfnOpenClipboard)(HWND);
    typedef HANDLE(WINAPI* pfnGetClipboardData)(UINT);
    typedef BOOL(WINAPI* pfnCloseClipboard)(void);

    HMODULE hUser = LoadLibraryW(L"user32.dll");
    if (!hUser) return "";

    pfnOpenClipboard fnOpen = (pfnOpenClipboard)GetProcAddress(hUser, "OpenClipboard");
    pfnGetClipboardData fnGet = (pfnGetClipboardData)GetProcAddress(hUser, "GetClipboardData");
    pfnCloseClipboard fnClose = (pfnCloseClipboard)GetProcAddress(hUser, "CloseClipboard");

    if (!fnOpen || !fnGet || !fnClose) {
        FreeLibrary(hUser);
        return "";
    }

    if (!fnOpen(NULL)) {
        FreeLibrary(hUser);
        return "";
    }

    HANDLE h = fnGet(CF_UNICODETEXT);
    if (!h) {
        fnClose();
        FreeLibrary(hUser);
        return "";
    }

    wchar_t* t = (wchar_t*)GlobalLock(h);
    if (!t) {
        fnClose();
        FreeLibrary(hUser);
        return "";
    }

    wstring r(t);
    GlobalUnlock(h);
    fnClose();
    FreeLibrary(hUser);
    return wstringToUtf8(r);
}

// ========== KEYBOARD INPUT (WITH NATIVE EDITING & 0 TO RETURN) ==========
bool inputLineWithEscape(string& result, const string& prompt) {
    if (!prompt.empty()) cout << prompt;
    result.clear();

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prevMode = 0;
    GetConsoleMode(hIn, &prevMode);
    SetConsoleMode(hIn, prevMode | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE);

    wchar_t wbuf[4096];
    DWORD readChars = 0;
    if (!ReadConsoleW(hIn, wbuf, 4095, &readChars, NULL)) {
        return false;
    }

    while (readChars > 0 && (wbuf[readChars - 1] == L'\r' || wbuf[readChars - 1] == L'\n')) {
        readChars--;
    }
    wbuf[readChars] = L'\0';

    wstring ws(wbuf, readChars);
    result = wstringToUtf8(ws);

    if (result == "0") {
        return false;
    }
    return true;
}

// ========== FILE & DIRECTORY HELPERS ==========
bool fileExists(const string& p) {
    if (p.empty()) return false;
    std::error_code ec;
    return fs::is_regular_file(fs::u8path(p), ec);
}

bool dirExists(const string& p) {
    if (p.empty()) return false;
    std::error_code ec;
    return fs::is_directory(fs::u8path(p), ec);
}

bool createDirRecursive(const string& p) {
    if (p.empty()) return false;
    std::error_code ec;
    return fs::create_directories(fs::u8path(p), ec) || dirExists(p);
}

int runProcessWait(const wstring& cmdLine) {
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    wstring mutableCmd = cmdLine;
    if (!CreateProcessW(NULL, &mutableCmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exitCode;
}

int execCmd(const string& cmd) {
    return runProcessWait(utf8ToWstring(cmd));
}

string findFileRecursive(const string& dir, const string& f) {
    if (dir.empty() || !dirExists(dir)) return "";
    try {
        std::error_code ec;
        for (const auto& entry : fs::recursive_directory_iterator(fs::u8path(dir), fs::directory_options::skip_permission_denied, ec)) {
            if (!ec && entry.is_regular_file(ec)) {
                if (entry.path().filename().u8string() == f || entry.path().filename().string() == f) {
                    return entry.path().u8string();
                }
            }
        }
    } catch (...) {}
    return "";
}

// ========== NATIVE DOWNLOADER & ZIP EXTRACTOR ==========
void printComponentProgress(const string& label, double percent, const string& extraInfo = "") {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    const int barWidth = 25;
    int pos = (int)(barWidth * percent / 100.0);

    string line = "\r";
    if (!label.empty()) {
        line += "[" + label + "] ";
    }
    line += "[";
    for (int i = 0; i < barWidth; i++) {
        line += (i < pos) ? '=' : (i == pos ? '>' : ' ');
    }
    line += "] ";

    char pctBuf[32];
    snprintf(pctBuf, sizeof(pctBuf), "%3.0f%%", percent);
    line += pctBuf;

    if (!extraInfo.empty()) {
        line += " " + extraInfo;
    }
    line += "        ";
    cout << line << flush;
}

bool downloadFile(const string& url, const string& destFile, const string& label = "") {
    HINTERNET hInternet = InternetOpenW(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) MR-CLI/1.1", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return false;

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_SECURE;
    wstring wUrl = utf8ToWstring(url);
    HINTERNET hUrl = InternetOpenUrlW(hInternet, wUrl.c_str(), NULL, 0, flags, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }

    // Query Content-Length
    DWORD contentLength = 0;
    DWORD clLen = sizeof(contentLength);
    DWORD index = 0;
    HttpQueryInfoW(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &clLen, &index);

    ofstream outFile(fs::u8path(destFile), ios::binary);
    if (!outFile.is_open()) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }

    DWORD bytesRead = 0;
    DWORD totalDownloaded = 0;
    char buffer[65536];

    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        outFile.write(buffer, bytesRead);
        totalDownloaded += bytesRead;
        if (contentLength > 0) {
            double pct = ((double)totalDownloaded / (double)contentLength) * 100.0;
            double curMB = (double)totalDownloaded / (1024.0 * 1024.0);
            double totMB = (double)contentLength / (1024.0 * 1024.0);
            char info[64];
            snprintf(info, sizeof(info), "(%.1f / %.1f MB)", curMB, totMB);
            printComponentProgress(label, pct, info);
        } else {
            double curMB = (double)totalDownloaded / (1024.0 * 1024.0);
            char info[64];
            snprintf(info, sizeof(info), "(%.1f MB)", curMB);
            printComponentProgress(label, 0, info);
        }
    }

    outFile.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    cout << "\n";
    return fileExists(destFile) && (totalDownloaded > 1000);
}

bool extractZip(const string& zipPath, const string& destDir) {
    if (!fileExists(zipPath) || !dirExists(destDir)) return false;

    // 1. Ultra-fast native Windows tar.exe (System32 bsdtar on Windows 10/11 - instant extraction)
    wchar_t sysDir[MAX_PATH];
    if (GetSystemDirectoryW(sysDir, MAX_PATH) > 0) {
        wstring tarExe = wstring(sysDir) + L"\\tar.exe";
        if (GetFileAttributesW(tarExe.c_str()) != INVALID_FILE_ATTRIBUTES) {
            wstring wDest = utf8ToWstring(destDir);
            while (!wDest.empty() && (wDest.back() == L'\\' || wDest.back() == L'/')) wDest.pop_back();

            wstring wZip = utf8ToWstring(zipPath);
            while (!wZip.empty() && (wZip.back() == L'\\' || wZip.back() == L'/')) wZip.pop_back();

            wstring cmd = L"\"" + tarExe + L"\" -xf \"" + wZip + L"\" -C \"" + wDest + L"\"";
            if (runProcessWait(cmd) == 0) {
                return true;
            }
        }
    }

    // 2. Fallback to Windows Shell COM (for older Windows versions without tar.exe)
    IShellDispatch* pISD = nullptr;
    if (FAILED(CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pISD)) || !pISD) {
        return false;
    }

    VARIANT vZip, vDest;
    VariantInit(&vZip);
    VariantInit(&vDest);

    fs::path absZip = fs::absolute(fs::u8path(zipPath));
    fs::path absDest = fs::absolute(fs::u8path(destDir));

    vZip.vt = VT_BSTR;
    vZip.bstrVal = SysAllocString(absZip.wstring().c_str());

    vDest.vt = VT_BSTR;
    vDest.bstrVal = SysAllocString(absDest.wstring().c_str());

    Folder* pZipFolder = nullptr;
    Folder* pDestFolder = nullptr;
    bool success = false;

    if (SUCCEEDED(pISD->NameSpace(vZip, &pZipFolder)) && pZipFolder) {
        if (SUCCEEDED(pISD->NameSpace(vDest, &pDestFolder)) && pDestFolder) {
            FolderItems* pItems = nullptr;
            if (SUCCEEDED(pZipFolder->Items(&pItems)) && pItems) {
                VARIANT vItems, vOptions;
                VariantInit(&vItems);
                VariantInit(&vOptions);
                vItems.vt = VT_DISPATCH;
                vItems.pdispVal = pItems;
                vOptions.vt = VT_I4;
                vOptions.lVal = 4 | 16 | 512 | 1024; // FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI

                HRESULT hr = pDestFolder->CopyHere(vItems, vOptions);
                success = SUCCEEDED(hr);
                pItems->Release();
            }
            pDestFolder->Release();
        }
        pZipFolder->Release();
    }

    VariantClear(&vZip);
    VariantClear(&vDest);
    pISD->Release();
    return success;
}

void organizeExtractedTool(const string& targetExe, const string& destDir) {
    std::error_code ec;
    fs::path targetDir = fs::weakly_canonical(fs::u8path(destDir), ec);
    if (ec || targetDir.empty()) {
        targetDir = fs::u8path(destDir);
        while (targetDir.has_filename() && targetDir.filename().empty()) {
            targetDir = targetDir.parent_path();
        }
    }

    // 1. Search for targetExe in subdirectories of targetDir
    fs::path foundExePath;
    for (const auto& entry : fs::recursive_directory_iterator(targetDir, fs::directory_options::skip_permission_denied, ec)) {
        if (!ec && entry.is_regular_file(ec)) {
            if (entry.path().filename().u8string() == targetExe || entry.path().filename().string() == targetExe) {
                // If it is in a subdirectory (not directly in targetDir), this is our extracted tool
                if (entry.path().parent_path() != targetDir) {
                    foundExePath = entry.path();
                    break;
                }
            }
        }
    }

    if (foundExePath.empty()) {
        return;
    }

    fs::path binDir = foundExePath.parent_path();

    // 2. Copy all .exe / tool files from binDir into targetDir
    for (const auto& entry : fs::directory_iterator(binDir, ec)) {
        if (!ec && entry.is_regular_file(ec)) {
            fs::copy_file(entry.path(), targetDir / entry.path().filename(), fs::copy_options::overwrite_existing, ec);
        }
    }

    // 3. Delete any extracted subdirectories inside targetDir
    for (const auto& entry : fs::directory_iterator(targetDir, ec)) {
        if (!ec && entry.is_directory(ec)) {
            fs::remove_all(entry.path(), ec);
        }
    }
}

// ========== PROGRESS PARSER & BAR ==========
bool parseDownloadLine(const string& line, string& percent, string& speed, string& eta) {
    if (line.find("[download]") == string::npos) return false;
    istringstream iss(line);
    vector<string> tokens;
    string tok;
    while (iss >> tok) tokens.push_back(tok);
    percent.clear(); speed.clear(); eta.clear();
    for (size_t i = 0; i < tokens.size(); i++) {
        if (!tokens[i].empty() && tokens[i].back() == '%')
            percent = tokens[i].substr(0, tokens[i].size() - 1);
        if (tokens[i] == "at" && i + 1 < tokens.size())
            speed = tokens[i + 1];
        if (tokens[i] == "ETA" && i + 1 < tokens.size())
            eta = tokens[i + 1];
    }
    return !percent.empty();
}

void printProgressBar(const string& percentStr, const string& speed, const string& eta) {
    double percent = atof(percentStr.c_str());
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    const int barWidth = 30;
    int pos = (int)(barWidth * percent / 100.0);
    string line = "\r[";
    for (int i = 0; i < barWidth; i++) line += (i < pos) ? '=' : (i == pos ? '>' : ' ');
    line += "] " + percentStr + "%";
    if (!speed.empty()) line += "  " + speed;
    if (!eta.empty()) line += "  ETA " + eta;
    line += "        ";
    cout << line << flush;
}

// ========== DOWNLOAD ARCHIVE ==========
string getArchivePath() {
    return CONFIG_PATH + "archive.txt";
}

// ========== CONFIG ==========
void saveConfig() {
    string configPath = CONFIG_PATH + "mr-config.txt";
    ofstream f(configPath, ios::out | ios::binary);
    if (!f.is_open()) return;
    unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    f.write((char*)bom, sizeof(bom));
    f << "DOWNLOAD_PATH=" << DOWNLOAD_PATH << "\n"
        << "COOKIES_FILE=" << COOKIES_FILE << "\n"
        << "USE_COOKIES=" << (USE_COOKIES ? "true" : "false") << "\n"
        << "VIDEO_RESOLUTION=" << VIDEO_RESOLUTION << "\n"
        << "VIDEO_FPS=" << VIDEO_FPS << "\n"
        << "VIDEO_FORMAT=" << VIDEO_FORMAT << "\n"
        << "AUDIO_FORMAT=" << AUDIO_FORMAT << "\n"
        << "CODEC_RECOMPILER=" << (CODEC_RECOMPILER ? "true" : "false") << "\n"
        << "ONLY_AUDIO=" << (ONLY_AUDIO ? "true" : "false") << "\n"
        << "ONLY_VIDEO=" << (ONLY_VIDEO ? "true" : "false") << "\n";
    f.close();
}

void loadConfig() {
    string configPath = CONFIG_PATH + "mr-config.txt";

    if (fileExists(configPath)) {
        ifstream f(configPath);
        if (f.is_open()) {
            string l;
            while (getline(f, l)) {
                if (l.length() >= 3 && (unsigned char)l[0] == 0xEF &&
                    (unsigned char)l[1] == 0xBB && (unsigned char)l[2] == 0xBF) {
                    l = l.substr(3);
                }
                while (!l.empty() && (l.back() == '\r' || l.back() == '\n')) l.pop_back();

                if (l.find("DOWNLOAD_PATH=") == 0) DOWNLOAD_PATH = l.substr(14);
                else if (l.find("COOKIES_FILE=") == 0) COOKIES_FILE = l.substr(13);
                else if (l.find("USE_COOKIES=") == 0) USE_COOKIES = (l.substr(12) == "true");
                else if (l.find("VIDEO_RESOLUTION=") == 0) VIDEO_RESOLUTION = l.substr(17);
                else if (l.find("VIDEO_FPS=") == 0) VIDEO_FPS = l.substr(10);
                else if (l.find("VIDEO_FORMAT=") == 0) VIDEO_FORMAT = l.substr(13);
                else if (l.find("AUDIO_FORMAT=") == 0) AUDIO_FORMAT = l.substr(13);
                else if (l.find("CODEC_RECOMPILER=") == 0) CODEC_RECOMPILER = (l.substr(17) == "true");
                else if (l.find("ONLY_AUDIO=") == 0) ONLY_AUDIO = (l.substr(11) == "true");
                else if (l.find("ONLY_VIDEO=") == 0) ONLY_VIDEO = (l.substr(11) == "true");
            }
            f.close();
        }
    }

    if (DOWNLOAD_PATH.empty()) {
        wchar_t buf[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, buf))) {
            wstring wPath = wstring(buf) + L"\\MR-CLI-FOR-YT-DLP\\downloads\\";
            string path = wstringToUtf8(wPath);
            if (!dirExists(path)) createDirRecursive(path);
            DOWNLOAD_PATH = path;
        }
        else {
            DOWNLOAD_PATH = "C:\\MR-CLI-FOR-YT-DLP\\downloads\\";
            if (!dirExists(DOWNLOAD_PATH)) createDirRecursive(DOWNLOAD_PATH);
        }
        saveConfig();
    }
    else {
        if (!dirExists(DOWNLOAD_PATH)) createDirRecursive(DOWNLOAD_PATH);
    }
}

// ========== DIALOGS ==========
string openFolderDialog() {
    BROWSEINFOW bi = { 0 };
    bi.lpszTitle = L"Select folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return "";

    wchar_t p[MAX_PATH];
    if (SHGetPathFromIDListW(pidl, p)) {
        CoTaskMemFree(pidl);
        wstring wp(p);
        if (wp.back() != L'\\' && wp.back() != L'/') wp += L'\\';
        return wstringToUtf8(wp);
    }
    CoTaskMemFree(pidl);
    return "";
}

string openFileDialog() {
    OPENFILENAMEW ofn = { 0 };
    wchar_t fn[MAX_PATH] = L"";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetConsoleWindow();
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fn;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    ofn.lpstrTitle = L"Select cookies file";

    if (GetOpenFileNameW(&ofn)) {
        return wstringToUtf8(wstring(fn));
    }
    return "";
}

// ========== ENCRYPTED COOKIES (WINDOWS DPAPI) ==========
static string g_tempCookiesPath = "";

void cleanupTempCookiesFile() {
    if (!g_tempCookiesPath.empty() && fileExists(g_tempCookiesPath)) {
        // Secure wipe: overwrite with zeros before deleting
        ofstream f(fs::u8path(g_tempCookiesPath), ios::in | ios::out | ios::binary);
        if (f.is_open()) {
            f.seekp(0, ios::end);
            size_t sz = (size_t)f.tellp();
            f.seekp(0, ios::beg);
            vector<char> zeros(sz, 0);
            f.write(zeros.data(), sz);
            f.close();
        }
        std::error_code ec;
        fs::remove(fs::u8path(g_tempCookiesPath), ec);
        g_tempCookiesPath.clear();
    }
}

bool encryptData(const string& plaintext, vector<BYTE>& encryptedData) {
    if (plaintext.empty()) return false;
    DATA_BLOB inBlob;
    inBlob.pbData = (BYTE*)plaintext.data();
    inBlob.cbData = (DWORD)plaintext.size();
    DATA_BLOB outBlob;
    if (CryptProtectData(&inBlob, L"MR_CLI_COOKIES", NULL, NULL, NULL, 0, &outBlob)) {
        encryptedData.assign(outBlob.pbData, outBlob.pbData + outBlob.cbData);
        LocalFree(outBlob.pbData);
        return true;
    }
    return false;
}

bool decryptData(const vector<BYTE>& encryptedData, string& plaintext) {
    if (encryptedData.empty()) return false;
    DATA_BLOB inBlob;
    inBlob.pbData = (BYTE*)encryptedData.data();
    inBlob.cbData = (DWORD)encryptedData.size();
    DATA_BLOB outBlob;
    if (CryptUnprotectData(&inBlob, NULL, NULL, NULL, NULL, 0, &outBlob)) {
        plaintext.assign((char*)outBlob.pbData, outBlob.cbData);
        LocalFree(outBlob.pbData);
        return true;
    }
    return false;
}

bool validateCookiesContent(const string& c) {
    if (c.find("# Netscape HTTP Cookie File") != string::npos) return true;
    stringstream ss(c); string l;
    while (getline(ss, l)) {
        if (l.empty() || l[0] == '#') continue;
        int tabs = 0; for (char ch : l) if (ch == '\t') tabs++;
        if (tabs >= 5) return true;
    }
    if (c.find("\"domain\":") != string::npos && c.find("\"name\":") != string::npos) return true;
    return false;
}

bool validateCookies(const string& p) {
    ifstream f(fs::u8path(p), ios::binary); if (!f.is_open()) return false;
    string c((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    f.close(); return validateCookiesContent(c);
}

bool saveCookies(const string& c) {
    string fullCookies = c;
    if (fullCookies.find("# Netscape HTTP Cookie File") == string::npos && fullCookies.find("[") != 0) {
        fullCookies = "# Netscape HTTP Cookie File\n# MR CLI FOR YT DLP v1.1.0\n\n" + fullCookies;
    }
    vector<BYTE> enc;
    if (encryptData(fullCookies, enc)) {
        string encPath = CONFIG_PATH + "cookies.dat";
        ofstream f(fs::u8path(encPath), ios::binary);
        if (f.is_open()) {
            f.write((char*)enc.data(), enc.size());
            f.close();
            // Securely remove plain cookies.txt if it was there
            string txtPath = CONFIG_PATH + "cookies.txt";
            if (fileExists(txtPath)) {
                std::error_code ec;
                fs::remove(fs::u8path(txtPath), ec);
            }
            return true;
        }
    }
    return false;
}

string getDecryptedCookies() {
    string encPath = CONFIG_PATH + "cookies.dat";
    if (fileExists(encPath)) {
        ifstream f(fs::u8path(encPath), ios::binary);
        if (f.is_open()) {
            vector<BYTE> enc((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
            f.close();
            string plain;
            if (decryptData(enc, plain)) {
                return plain;
            }
        }
    }
    // Auto-migration from legacy cookies.txt
    string txtPath = CONFIG_PATH + "cookies.txt";
    if (fileExists(txtPath)) {
        ifstream f(fs::u8path(txtPath), ios::binary);
        if (f.is_open()) {
            string plain((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
            f.close();
            saveCookies(plain);
            return plain;
        }
    }
    return "";
}

bool hasSavedCookies() {
    return fileExists(CONFIG_PATH + "cookies.dat") || fileExists(CONFIG_PATH + "cookies.txt");
}

string prepareTempCookiesFile() {
    cleanupTempCookiesFile();
    string plain = getDecryptedCookies();
    if (plain.empty()) return "";

    wchar_t tempDir[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempDir) == 0) return "";

    wstring tempFile = wstring(tempDir) + L".mr_yt_cookies_" + to_wstring(GetCurrentProcessId()) + L".tmp";
    string u8TempFile = wstringToUtf8(tempFile);

    ofstream f(fs::u8path(u8TempFile), ios::binary);
    if (!f.is_open()) return "";
    f.write(plain.data(), plain.size());
    f.close();

    SetFileAttributesW(tempFile.c_str(), FILE_ATTRIBUTE_TEMPORARY);
    g_tempCookiesPath = u8TempFile;
    return u8TempFile;
}

// ========== MENUS ==========
string cookieEditor(bool fromSettings = false) {
    while (true) {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(" COOKIE EDITOR (ENCRYPTED STORAGE)", CYAN);
        printColor("========================================", CYAN);
        cout << "\nStatus: " << (hasSavedCookies() ? "[PROTECTED / ENCRYPTED]" : "[NOT CONFIGURED]") << "\n";
        cout << "\n1. Select cookies file (.txt)\n2. Paste from clipboard\n3. Edit in Notepad (Temporary decrypted)\n4. Delete saved cookies\n0. Exit to main menu\n\nYour choice: ";
        char ch = getMenuChoice();
        if (ch == 27) {
            cout << "ESC" << endl;
            return "exit";
        }
        cout << ch << endl;
        switch (ch) {
        case '1': {
            string p = openFileDialog();
            if (p.empty()) {
                printColor("[INFO] Selection cancelled", YELLOW);
                waitForKey();
                break;
            }
            if (validateCookies(p)) {
                ifstream f(fs::u8path(p), ios::binary);
                string c((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
                f.close();
                if (saveCookies(c)) {
                    USE_COOKIES = true;
                    saveConfig();
                    printColor("[OK] Cookies encrypted and saved securely!", GREEN);
                    waitForKey();
                    return fromSettings ? "settings" : "continue";
                }
            }
            else {
                printColor("[ERROR] Invalid cookies file! Netscape format required.", RED);
                waitForKey();
            }
            break;
        }
        case '2': {
            string c = getClipboard();
            if (c.empty()) {
                printColor("[ERROR] Clipboard is empty!", RED);
                waitForKey();
                break;
            }
            if (validateCookiesContent(c) && saveCookies(c)) {
                USE_COOKIES = true;
                saveConfig();
                printColor("[OK] Cookies encrypted and saved securely!", GREEN);
                waitForKey();
                return fromSettings ? "settings" : "continue";
            }
            printColor("[ERROR] Invalid cookies format in clipboard!", RED);
            waitForKey();
            break;
        }
        case '3': {
            string plain = getDecryptedCookies();
            if (plain.empty()) {
                plain = "# Netscape HTTP Cookie File\n# https://curl.haxx.se/rfc/cookie_spec.html\n# Paste cookies here, save, and close Notepad\n\n";
            }
            string tempEdit = prepareTempCookiesFile();
            if (tempEdit.empty()) {
                printColor("[ERROR] Failed to prepare temporary edit file!", RED);
                waitForKey();
                break;
            }
            wstring cmd = L"notepad.exe \"" + utf8ToWstring(tempEdit) + L"\"";
            runProcessWait(cmd);
            if (fileExists(tempEdit) && validateCookies(tempEdit)) {
                ifstream f(fs::u8path(tempEdit), ios::binary);
                string c((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
                f.close();
                if (saveCookies(c)) {
                    USE_COOKIES = true;
                    saveConfig();
                    printColor("[OK] Cookies updated and encrypted securely!", GREEN);
                    cleanupTempCookiesFile();
                    waitForKey();
                    return fromSettings ? "settings" : "continue";
                }
            }
            cleanupTempCookiesFile();
            printColor("[INFO] No changes saved.", YELLOW);
            waitForKey();
            break;
        }
        case '4': {
            std::error_code ec;
            fs::remove(fs::u8path(CONFIG_PATH + "cookies.dat"), ec);
            fs::remove(fs::u8path(CONFIG_PATH + "cookies.txt"), ec);
            USE_COOKIES = false;
            saveConfig();
            printColor("[OK] Saved cookies removed!", GREEN);
            waitForKey();
            break;
        }
        case '0': {
            cout << "Exit" << endl;
            return "exit";
        }
        default:
            printColor("[ERROR] Invalid choice!", RED);
            waitForKey();
        }
    }
}

// ========== FORMAT BUILDING ==========
string buildFormat() {
    // 1. Resolution filters (supports standard landscape and vertical Shorts)
    string resExact = "height=" + VIDEO_RESOLUTION;
    string resLte = "height<=?" + VIDEO_RESOLUTION;

    // 2. Frame Rate filter
    string fpsFilter = (VIDEO_FPS == "30") ? "[fps<=?30]" : "[fps<=?60]";

    // 3. Audio format filter
    string audioFmt;
    if (AUDIO_FORMAT.find("M4A") != string::npos) audioFmt = "ba[ext=m4a]/ba[acodec^=mp4a]/ba[acodec^=aac]";
    else if (AUDIO_FORMAT.find("Opus") != string::npos) audioFmt = "ba[ext=webm][acodec^=opus]/ba[acodec^=opus]";
    else if (AUDIO_FORMAT.find("Vorbis") != string::npos) audioFmt = "ba[ext=webm][acodec^=vorbis]/ba[acodec^=vorbis]";
    else audioFmt = "ba[ext=m4a]/ba";

    // 4. Video Codec and Extension filter
    string codecFilter;
    string ext;
    if (VIDEO_FORMAT.find("MP4(AV1)") != string::npos) { codecFilter = "[vcodec^=av01]"; ext = "[ext=mp4]"; }
    else if (VIDEO_FORMAT.find("MP4(H.264)") != string::npos) { codecFilter = "[vcodec^=avc1]"; ext = "[ext=mp4]"; }
    else if (VIDEO_FORMAT.find("WEBM(AV1)") != string::npos) { codecFilter = "[vcodec^=av01]"; ext = "[ext=webm]"; }
    else if (VIDEO_FORMAT.find("WEBM(VP9)") != string::npos) { codecFilter = "[vcodec^=vp9]";  ext = "[ext=webm]"; }
    else { codecFilter = "[vcodec^=avc1]"; ext = "[ext=mp4]"; }

    // Multi-tier video selectors:
    // Tier 1: Preferred container + Preferred codec + Exact resolution + Preferred FPS
    string vExactPref = "bv" + ext + codecFilter + "[" + resExact + "]" + fpsFilter;
    // Tier 2: Any container/codec + Exact resolution + Preferred FPS
    string vExactAny = "bv[" + resExact + "]" + fpsFilter;
    // Tier 3: Preferred container + Preferred codec + Target resolution or lower + Preferred FPS
    string vLtePref = "bv" + ext + codecFilter + "[" + resLte + "]" + fpsFilter;
    // Tier 4: Any container/codec + Target resolution or lower + Preferred FPS
    string vLteAny = "bv[" + resLte + "]" + fpsFilter;

    if (ONLY_AUDIO) {
        return "(" + audioFmt + ")/ba/bestaudio";
    }

    if (ONLY_VIDEO) {
        return vExactPref + "/" + vExactAny + "/" + vLtePref + "/" + vLteAny + "/bestvideo";
    }

    // Combined video + audio with resilient fallbacks for YouTube and all other supported sites
    return "(" + vExactPref + "+" + audioFmt + ")/" +
           "(" + vExactAny + "+" + audioFmt + ")/" +
           "(" + vLtePref + "+" + audioFmt + ")/" +
           "(" + vLteAny + "+" + audioFmt + ")/" +
           "(" + vLteAny + "+ba)/" +
           "bestvideo+bestaudio/best";
}

// ========== NETWORK ERROR DETECTION ==========
bool isNetworkError(const string& line) {
    static const char* patterns[] = {
        "Failed to resolve",
        "getaddrinfo failed",
        "Errno 11001",
        "WinError 10054",
        "WinError 10060",
        "WinError 10061",
        "Network is unreachable",
        "Connection reset by peer",
        "RemoteDisconnected",
        "The read operation timed out"
    };
    for (auto p : patterns) {
        if (line.find(p) != string::npos) return true;
    }
    return false;
}

// ========== CHECK INTERNET ==========
bool checkInternet() {
    DWORD flags = 0;
    if (InternetGetConnectedState(&flags, 0)) {
        return true;
    }
    return (InternetCheckConnectionW(L"https://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0) == TRUE);
}

// ========== WAIT FOR INTERNET ==========
bool waitForInternetAndRetry() {
    int attemptCount = 1;
    while (!checkInternet()) {
        printColor("\n============================================", RED);
        printColor("[ERROR] Internet connection lost! Waiting 15 seconds... Attempt " + to_string(attemptCount), RED);
        printColor("============================================", RED);
        Sleep(15000);
        attemptCount++;
        if (_kbhit() && _getch() == 27) {
            printColor("[INFO] Cancelled by user.", YELLOW);
            return false;
        }
    }

    printColor("\n============================================", CYAN);
    printColor("[INFO] Connection restored. Continuing download...", CYAN);
    printColor("============================================\n", CYAN);
    return true;
}

// ========== PROCESS EXECUTION WITH LIVE PROGRESS ==========
bool execWithProgress(const wstring& cmdLine) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe = NULL, hWritePipe = NULL;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        printColor("[ERROR] Failed to create process pipe!", RED);
        return false;
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

    wstring mutableCmd = cmdLine;
    BOOL success = CreateProcessW(
        NULL,
        &mutableCmd[0],
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    // Parent must close write end so ReadFile signals EOF when child exits
    CloseHandle(hWritePipe);

    if (!success) {
        CloseHandle(hReadPipe);
        printColor("[ERROR] Failed to launch yt-dlp process! Error code: " + to_string(GetLastError()), RED);
        return false;
    }

    string line;
    bool progressActive = false;
    bool isPlaylist = false;
    bool mergeFailed = false;
    bool inArchive = false;
    int curItem = 0, totalItems = 0;
    string mergeErrorPath = "";
    char buffer[4096];
    DWORD bytesRead = 0;

    while (true) {
        // Check if user pressed ESC to cancel active download
        if (_kbhit()) {
            int key = _getch();
            if (key == 27) { // ESC
                if (progressActive) { cout << endl; progressActive = false; }
                printColor("\n[INFO] Download cancelled by user (ESC).", YELLOW);
                LAST_ERROR = CANCELLED_BY_USER;
                TerminateProcess(pi.hProcess, 1);
                break;
            }
        }

        DWORD bytesAvail = 0;
        if (!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &bytesAvail, NULL)) {
            break; // Pipe broken, child exited
        }

        if (bytesAvail == 0) {
            DWORD waitRes = WaitForSingleObject(pi.hProcess, 50);
            if (waitRes == WAIT_OBJECT_0) {
                // Process finished, read remaining bytes
                if (!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &bytesAvail, NULL) || bytesAvail == 0) {
                    break;
                }
            }
            continue;
        }

        if (!ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0) {
            break;
        }

        for (DWORD i = 0; i < bytesRead; i++) {
            char c = buffer[i];
            if (c == '\r' || c == '\n') {
                if (!line.empty()) {
                    string percent, speed, eta;

                    if (line == "ERROR:" || line == "ERROR: ") {
                        line.clear();
                        continue;
                    }

                    if (line.find("Retrying") != string::npos && !isNetworkError(line)) {
                        line.clear();
                        continue;
                    }

                    if (isNetworkError(line)) {
                        if (progressActive) { cout << endl; progressActive = false; }
                        printColor("[WARNING] Network hiccup detected: " + line, YELLOW);
                    }

                    if (line.find("[download] Downloading item ") == 0) {
                        int newCurItem = 0, newTotalItems = 0;
                        if (sscanf_s(line.c_str(), "[download] Downloading item %d of %d", &newCurItem, &newTotalItems) == 2) {
                            curItem = newCurItem;
                            totalItems = newTotalItems;
                            TOTAL_ITEMS_IN_PLAYLIST = newTotalItems;
                            LAST_PROCESSED_ITEM = curItem;
                            isPlaylist = (TOTAL_ITEMS_IN_PLAYLIST > 0);
                        }
                    }
                    else if (line.find("has already been recorded in the archive") != string::npos) {
                        if (progressActive) { cout << endl; progressActive = false; }
                        printColor("[INFO] Video already downloaded (in archive, skipping).", GREEN);
                        inArchive = true;
                    }
                    else if (line.find("[download] Destination:") == 0) {
                        string path = line.substr(string("[download] Destination:").length());
                        while (!path.empty() && path.front() == ' ') path.erase(path.begin());

                        size_t slash = path.find_last_of("\\/");
                        string name = (slash != string::npos) ? path.substr(slash + 1) : path;
                        size_t dot = name.find_last_of('.');
                        if (dot != string::npos) name = name.substr(0, dot);

                        if (progressActive) { cout << endl; progressActive = false; }

                        string prefix = "";
                        if (ONLY_AUDIO) prefix = "[Only audio] ";
                        else if (ONLY_VIDEO) prefix = "[Only video] ";

                        if (totalItems > 0)
                            printColor(prefix + "Downloading " + to_string(curItem) + " of " + to_string(totalItems) + " - " + name, CYAN);
                        else
                            printColor(prefix + "Downloading into \"" + path + "\"", CYAN);
                    }
                    else if (line.find("[Merger] Merging formats into") == 0) {
                        if (progressActive) { cout << endl; progressActive = false; }
                        string path = line.substr(string("[Merger] Merging formats into").length());
                        while (!path.empty() && path.front() == ' ') path.erase(path.begin());
                        if (!path.empty() && path.back() == '"') path.pop_back();
                        printColor("[Merger] Merging video and audio into \"" + path + "\"", CYAN);
                    }
                    else if (parseDownloadLine(line, percent, speed, eta)) {
                        printProgressBar(percent, speed, eta);
                        progressActive = true;
                    }
                    else if (line.find("[Merger]") == 0 || line.find("[ExtractAudio]") == 0 ||
                        line.find("[VideoConvertor]") == 0 || line.find("[Fixup") == 0) {
                        if (progressActive) { cout << endl; progressActive = false; }
                        if (line.find("[FixupM4a]") == string::npos) {
                            printColor(line, CYAN);
                        }
                    }
                    else if (line.find("WARNING") != string::npos &&
                        line.find("No title found in player responses") != string::npos) {
                        // Ignore harmless yt-dlp internal player response warning
                    }
                    else if (line.find("Video unavailable. This video is private") != string::npos ||
                        line.find("unavailable video is hidden") != string::npos ||
                        line.find("This video is unavailable") != string::npos) {
                        if (progressActive) { cout << endl; progressActive = false; }
                        printColor(line, YELLOW);
                    }
                    else if (line.find("rate-limited by YouTube") != string::npos ||
                        line.find("Your account has been rate-limited") != string::npos ||
                        line.find("HTTP Error 429") != string::npos) {
                        LAST_ERROR = RATE_LIMIT_ERROR;
                        if (progressActive) { cout << endl; progressActive = false; }
                        printColor("\n[RATE LIMIT] " + line, RED);
                    }
                    else if (line.find("ERROR") != string::npos || line.find("WARNING") != string::npos) {
                        if (line.find("[PYI-") != string::npos && inArchive) {
                            line.clear();
                            continue;
                        }
                        if (progressActive) { cout << endl; progressActive = false; }

                        if (line.find("Sign in to confirm you're not a bot") != string::npos ||
                            line.find("Login required") != string::npos ||
                            line.find("invalid cookies") != string::npos ||
                            line.find("no longer valid") != string::npos) {
                            LAST_ERROR = COOKIE_ERROR;
                        }
                        else if (line.find("Sign in to confirm your age") != string::npos) {
                            LAST_ERROR = AGE_ERROR;
                        }
                        else if (line.find("This live event will begin in a few moments") != string::npos ||
                            line.find("is a live stream") != string::npos) {
                            if (LAST_ERROR != COOKIE_ERROR && LAST_ERROR != AGE_ERROR) {
                                LAST_ERROR = LIVE_ERROR;
                            }
                        }
                        else {
                            if (LAST_ERROR == NOT_ERROR) {
                                LAST_ERROR = VIDEO_ERROR;
                            }
                        }

                        if (line.find("ERROR") != string::npos &&
                            (line.find("merge") != string::npos || line.find("Merger") != string::npos)) {
                            mergeFailed = true;
                            size_t quote1 = line.find('"');
                            if (quote1 != string::npos) {
                                size_t quote2 = line.find('"', quote1 + 1);
                                if (quote2 != string::npos) {
                                    mergeErrorPath = line.substr(quote1 + 1, quote2 - quote1 - 1);
                                }
                            }
                        }

                        printColor(line, line.find("ERROR") != string::npos ? RED : YELLOW);
                    }
                    line.clear();
                }
            }
            else {
                line += c;
            }
        }
    }

    if (progressActive) cout << endl;

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (LAST_ERROR == CANCELLED_BY_USER) {
        return false;
    }

    // Manual FFmpeg merge fallback if required
    if (mergeFailed && !mergeErrorPath.empty() && FFMPEG_FOUND && !FFMPEG_PATH.empty()) {
        printColor("\n[INFO] Merge failed. Trying manual FFmpeg merge...", YELLOW);

        string dir = mergeErrorPath.substr(0, mergeErrorPath.find_last_of("\\/") + 1);
        string baseName = mergeErrorPath.substr(mergeErrorPath.find_last_of("\\/") + 1);
        size_t dot = baseName.find_last_of('.');
        if (dot != string::npos) baseName = baseName.substr(0, dot);

        string videoFile = "", audioFile = "";
        WIN32_FIND_DATAW fd;
        wstring wdir = utf8ToWstring(dir);
        HANDLE hFind = FindFirstFileW((wdir + L"*.*").c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (wstring(fd.cFileName) == L"." || wstring(fd.cFileName) == L"..") continue;
                string fileName = wstringToUtf8(fd.cFileName);
                if (fileName.find(baseName) != string::npos) {
                    if (fileName.find(".mp4") != string::npos && fileName.find(".m4a") == string::npos) {
                        videoFile = dir + fileName;
                    }
                    else if (fileName.find(".m4a") != string::npos || fileName.find(".webm") != string::npos) {
                        audioFile = dir + fileName;
                    }
                }
            } while (FindNextFileW(hFind, &fd) != 0);
            FindClose(hFind);
        }

        if (!videoFile.empty() && !audioFile.empty()) {
            string outputFile = dir + baseName + "_merged.mp4";
            string mergeCmd = "\"" + FFMPEG_PATH + "\" -i \"" + videoFile + "\" -i \"" + audioFile + "\" -c:v copy -c:a aac -y \"" + outputFile + "\"";

            printColor("[INFO] Running manual merge: " + mergeCmd, CYAN);
            int mergeResult = execCmd(mergeCmd);

            if (mergeResult == 0) {
                printColor("[OK] Manual merge successful!", GREEN);
                std::error_code ec;
                fs::remove(fs::u8path(videoFile), ec);
                fs::remove(fs::u8path(audioFile), ec);
                string finalFile = dir + baseName + ".mp4";
                fs::rename(fs::u8path(outputFile), fs::u8path(finalFile), ec);
                if (ec) {
                    printColor("[WARNING] Merged file created as: " + outputFile, YELLOW);
                }
                else {
                    printColor("[OK] Final file: " + finalFile, GREEN);
                }
                return true;
            }
        }
    }

    if (inArchive) {
        cleanupTempCookiesFile();
        LAST_ERROR = NOT_ERROR;
        return true;
    }

    if (totalItems > 0 && curItem >= totalItems) {
        cleanupTempCookiesFile();
        return true;
    }

    cleanupTempCookiesFile();
    return (exitCode == 0);
}

// ========== COMMAND BUILDING ==========
wstring buildCommand(const string& url, const string& start, const string& end, bool isPlaylist) {
    wstring cmd = L"\"" + utf8ToWstring(YTDLP_PATH.empty() ? (CONFIG_PATH + "yt-dlp.exe") : YTDLP_PATH) + L"\"";

    if (USE_COOKIES && hasSavedCookies()) {
        string tempCookiePath = prepareTempCookiesFile();
        if (!tempCookiePath.empty()) {
            cmd += L" --cookies \"" + utf8ToWstring(tempCookiePath) + L"\"";
        }
    }

    if (!ARCHIVE_PATH.empty()) {
        cmd += L" --download-archive \"" + utf8ToWstring(ARCHIVE_PATH) + L"\"";
    }

    string fmt = buildFormat();
    cmd += L" -f \"" + utf8ToWstring(fmt) + L"\"";

    // Speed up downloads & prevent corrupted Windows filenames
    cmd += L" --windows-filenames --no-mtime --concurrent-fragments 4 --retries 10 --fragment-retries 10";

    // Pass FFmpeg location if available
    if (FFMPEG_FOUND && !FFMPEG_PATH.empty()) {
        size_t pos = FFMPEG_PATH.find_last_of("\\/");
        string ffmpegDir = (pos != string::npos) ? FFMPEG_PATH.substr(0, pos) : FFMPEG_PATH;
        cmd += L" --ffmpeg-location \"" + utf8ToWstring(ffmpegDir) + L"\"";
    }

    // Playlist request delays to prevent rate-limiting
    if (isPlaylist) {
        cmd += L" --sleep-requests 1 --sleep-interval 2 --max-sleep-interval 5";
    }

    // QuickJS JavaScript runtime for modern YouTube challenge solving
    if (QJS_FOUND && !QJS_PATH.empty() && fileExists(QJS_PATH)) {
        cmd += L" --js-runtimes \"quickjs:" + utf8ToWstring(QJS_PATH) + L"\"";
    }

    // Codec Recompiler (Re-encode to H.264 MP4 for maximum compatibility)
    if (CODEC_RECOMPILER) {
        cmd += L" --recode-video mp4";
    }

    string tag = "";
    if (ONLY_VIDEO) {
        tag = " [only video]";
        cmd += L" --no-audio";
        if (VIDEO_FORMAT.find("WEBM") != string::npos) cmd += L" --merge-output-format webm";
        else cmd += L" --merge-output-format mp4";
    }
    else if (ONLY_AUDIO) {
        tag = " [only audio]";
        cmd += L" -x";
        if (AUDIO_FORMAT.find("M4A") != string::npos) cmd += L" --audio-format m4a";
        else if (AUDIO_FORMAT.find("Opus") != string::npos) cmd += L" --audio-format opus";
        else if (AUDIO_FORMAT.find("Vorbis") != string::npos) cmd += L" --audio-format ogg";
        else cmd += L" --audio-format m4a";
    }
    else {
        if (VIDEO_FORMAT.find("WEBM") != string::npos) cmd += L" --merge-output-format webm";
        else cmd += L" --merge-output-format mp4";
    }

    if (isPlaylist) {
        if (!start.empty()) cmd += L" --playlist-start " + utf8ToWstring(start);
        if (!end.empty()) cmd += L" --playlist-end " + utf8ToWstring(end);
    }

    cmd += L" --encoding utf-8";

    // Use %(ext)s in output template so yt-dlp resolves the real container dynamically
    string out = isPlaylist ?
        DOWNLOAD_PATH + "[Playlist] %(playlist_title)s/%(playlist_index)03d - [%(channel)s] %(title)s" + tag :
        DOWNLOAD_PATH + "%(title)s" + tag;

    out += ".%(ext)s";
    cmd += L" -o \"" + utf8ToWstring(out) + L"\"";

    cmd += L" \"" + utf8ToWstring(url) + L"\"";

    return cmd;
}

// ========== DOWNLOAD WORKFLOW ==========
void startDownload(const string& url = "", const string& start = "", const string& end = "", bool isPlaylist = false, bool retry = false) {
    string u = url, s = start, e = end;
    bool isPl = isPlaylist;
    static bool cookieErrorHandled = false;
    static vector<string> skippedVideos;

    if (u.empty()) {
        if (!retry) {
            clearScreen();
        }
        else {
            cout << "\n";
        }
        printColor("============================================", CYAN);
        printColor(retry ? " RETRYING DOWNLOAD" : " STARTING DOWNLOAD", CYAN);
        printColor("============================================", CYAN);
        cout << "\nEnter URL (Paste: Ctrl+V, Cancel: ESC):\n";

        if (!inputLineWithEscape(u, "> ")) {
            printColor("\n[INFO] Cancelled", YELLOW);
            waitForKey();
            return;
        }

        // Clean up input
        while (!u.empty() && (u.front() == ' ' || u.front() == '"' || u.front() == '\'')) u.erase(u.begin());
        while (!u.empty() && (u.back() == ' ' || u.back() == '"' || u.back() == '\'')) u.pop_back();

        if (u.empty()) {
            printColor("[ERROR] URL cannot be empty!", RED);
            waitForKey();
            return;
        }

        // Playlist URL detection
        if (u.find("&list=") != string::npos && u.find("/playlist?list=") == string::npos) {
            clearScreen();
            printColor("============================================", CYAN);
            printColor(" URL contains both a video and a playlist:", CYAN);
            printColor("============================================", CYAN);
            cout << "\n1. Download entire playlist\n2. Download only this video\n\nYour choice: ";
            char plChoice = getMenuChoice();
            cout << plChoice << endl;
            if (plChoice == '1') {
                isPl = true;
            }
            else {
                size_t pos = u.find("&list=");
                if (pos != string::npos) u = u.substr(0, pos);
                isPl = false;
            }
        }
        else if (u.find("/playlist?list=") != string::npos || u.find("list=") != string::npos) {
            isPl = true;
        }
        else {
            isPl = false;
        }

        if (isPl) {
            string inp;
            cout << "\nStart index in playlist (Enter=first, ESC to skip):\n";
            if (!inputLineWithEscape(inp, "Your choice: ")) {
                cout << "[INFO] Defaulting to start\n";
            }
            else if (!inp.empty()) {
                s = inp;
            }

            cout << "\nEnd index in playlist (Enter=last, ESC to skip):\n";
            if (!inputLineWithEscape(inp, "Your choice: ")) {
                cout << "[INFO] Defaulting to end\n";
            }
            else if (!inp.empty()) {
                e = inp;
            }
        }

        cookieErrorHandled = false;
        skippedVideos.clear();
        TOTAL_ITEMS_IN_PLAYLIST = 0;
        LAST_PROCESSED_ITEM = 0;
        PLAYLIST_END_REACHED = false;
    }

    if (cookieErrorHandled) {
        return;
    }

    LAST_ERROR = NOT_ERROR;

    wstring cmd = buildCommand(u, s, e, isPl);

    if (!retry) {
        clearScreen();
    }

    printColor("============================================", CYAN);
    printColor(retry ? " RETRYING DOWNLOAD..." : " STARTING DOWNLOAD...", CYAN);
    printColor("============================================", CYAN);
    cout << endl;

    bool ok = execWithProgress(cmd);

    if (LAST_ERROR == CANCELLED_BY_USER) {
        LAST_ERROR = NOT_ERROR;
        waitForKey();
        return;
    }

    if (LAST_ERROR == RATE_LIMIT_ERROR) {
        printColor("\n============================================", RED);
        printColor("[ERROR] YouTube rate-limited requests. Waiting up to 1h (ESC to cancel)...", RED);
        printColor("============================================", RED);

        bool cancelled = false;
        for (int sec = 0; sec < 3660 && !cancelled; sec++) {
            if (_kbhit() && _getch() == 27) cancelled = true;
            else Sleep(1000);
            if (sec > 0 && sec % 300 == 0)
                printColor("[INFO] ~" + to_string((3660 - sec) / 60) + " min remaining (ESC to cancel)", YELLOW);
        }

        LAST_ERROR = NOT_ERROR;
        if (cancelled) {
            printColor("[INFO] Cancelled by user.", YELLOW);
            waitForKey();
            return;
        }

        printColor("============================================", CYAN);
        printColor("[INFO] Rate-limit cooldown complete. Continuing download...", CYAN);
        printColor("============================================", CYAN);

        if (isPl) {
            int currentIndex = (LAST_PROCESSED_ITEM > 0) ? LAST_PROCESSED_ITEM : (s.empty() ? 1 : stoi(s));
            startDownload(u, to_string(currentIndex), e, true, false);
        }
        else {
            startDownload(u, s, e, isPl, false);
        }
        return;
    }

    if (LAST_ERROR == COOKIE_ERROR || LAST_ERROR == AGE_ERROR) {
        cookieErrorHandled = true;

        int currentIndex = 1;
        if (!s.empty()) {
            try { currentIndex = stoi(s); } catch (...) { currentIndex = 1; }
        }

        printColor("============================================", RED);
        if (LAST_ERROR == COOKIE_ERROR) {
            printColor("[ERROR] YouTube requires verification (Bot check / Login required)!", RED);
            printColor("[ERROR] Please update cookies or use an account cookie file.", RED);
        }
        else {
            printColor("[ERROR] This video has age restriction!", YELLOW);
            printColor("[ERROR] Please use account cookies that can access this video.", YELLOW);
        }
        printColor("============================================", RED);

        printColor("\n============================================", YELLOW);
        printColor("[INFO] Please update your cookies to continue.", YELLOW);
        printColor("============================================", YELLOW);
        cout << "\n1 - Update cookies\n0 - Main menu (ESC)\nYour choice: ";
        char ch = getMenuChoice();
        if (ch == 27 || ch == '0') {
            cout << "ESC" << endl;
            LAST_ERROR = NOT_ERROR;
            cookieErrorHandled = false;
            return;
        }
        cout << ch << endl;
        if (ch == '1') {
            clearScreen();
            string result = cookieEditor(false);
            if (result == "continue") {
                cookieErrorHandled = false;
                cout << "\n";
                LAST_ERROR = NOT_ERROR;
                if (isPl) {
                    startDownload(u, to_string(currentIndex), e, true, true);
                }
                else {
                    startDownload(u, s, e, isPl, true);
                }
                return;
            }
        }
        LAST_ERROR = NOT_ERROR;
        cookieErrorHandled = false;
        return;
    }

    if (LAST_ERROR == LIVE_ERROR) {
        if (isPl) {
            int currentIndex = (LAST_PROCESSED_ITEM > 0) ? LAST_PROCESSED_ITEM : (s.empty() ? 1 : 1);
            int nextIndex = currentIndex + 1;
            string nextStart = to_string(nextIndex);

            bool isLastVideo = false;
            if (!e.empty()) {
                try {
                    int endIndex = stoi(e);
                    if (currentIndex >= endIndex) isLastVideo = true;
                } catch (...) {}
            }
            else if (TOTAL_ITEMS_IN_PLAYLIST > 0 && currentIndex >= TOTAL_ITEMS_IN_PLAYLIST) {
                isLastVideo = true;
            }

            if (isLastVideo) {
                printColor("============================================", GREEN);
                printColor("[OK] Playlist processing completed!", GREEN);
                printColor("============================================", GREEN);
                if (!skippedVideos.empty()) {
                    printColor("\n============================================", YELLOW);
                    printColor("[INFO] Skipped items in this session:", YELLOW);
                    for (const string& vid : skippedVideos) {
                        printColor("  - " + vid, YELLOW);
                    }
                    printColor("============================================", YELLOW);
                }
                waitForKey();
                LAST_ERROR = NOT_ERROR;
                cookieErrorHandled = false;
                TOTAL_ITEMS_IN_PLAYLIST = 0;
                return;
            }

            printColor("============================================", RED);
            printColor("[INFO] Video " + to_string(currentIndex) + " is a scheduled live stream. Skipping...", YELLOW);
            printColor("============================================", RED);
            Sleep(2000);

            LAST_ERROR = NOT_ERROR;
            startDownload(u, nextStart, e, true, true);
            return;
        }
        else {
            printColor("============================================", RED);
            printColor("[ERROR] This video is a live stream that has not started yet.", RED);
            printColor("============================================", RED);
            waitForKey();
            LAST_ERROR = NOT_ERROR;
            cookieErrorHandled = false;
            return;
        }
    }

    // ========== PLAYLIST HANDLING ==========
    if (!ok && isPl) {
        int currentIndex = (LAST_PROCESSED_ITEM > 0) ? LAST_PROCESSED_ITEM : (s.empty() ? 1 : 1);
        int nextIndex = currentIndex + 1;
        string nextStart = to_string(nextIndex);

        bool isLastVideo = false;
        if (!e.empty()) {
            try {
                int endIndex = stoi(e);
                if (currentIndex >= endIndex) isLastVideo = true;
            } catch (...) {}
        }
        else if (TOTAL_ITEMS_IN_PLAYLIST > 0 && currentIndex >= TOTAL_ITEMS_IN_PLAYLIST) {
            isLastVideo = true;
        }

        if (isLastVideo) {
            printColor("\n[INFO] Reached end of playlist range.", YELLOW);
            printColor("[OK] Download finished!", GREEN);
            if (!skippedVideos.empty()) {
                printColor("\n============================================", YELLOW);
                printColor("[INFO] Skipped items:", YELLOW);
                for (const string& vid : skippedVideos) {
                    printColor("  - " + vid, YELLOW);
                }
                printColor("============================================", YELLOW);
            }
            waitForKey();
            LAST_ERROR = NOT_ERROR;
            cookieErrorHandled = false;
            TOTAL_ITEMS_IN_PLAYLIST = 0;
            return;
        }

        if (LAST_ERROR == VIDEO_ERROR) {
            string skippedVideo = "Item " + to_string(currentIndex);
            skippedVideos.push_back(skippedVideo);
        }

        printColor("\n============================================", RED);
        printColor("[ERROR] Video " + to_string(currentIndex) + " could not be downloaded (skipping).", RED);
        printColor("============================================", RED);
        printColor("\n[INFO] Continuing with next item in 3 seconds...\n", CYAN);
        Sleep(3000);
        LAST_ERROR = NOT_ERROR;
        startDownload(u, nextStart, e, true, true);
        return;
    }

    // ========== SINGLE VIDEO - ERRORS ==========
    if (!ok) {
        printColor("\n============================================", RED);
        printColor("[ERROR] Download could not be completed!", RED);
        printColor("============================================", RED);
        printColor("\nPossible reasons:", RED);
        printColor("  - Expired / missing cookies (Login required)", RED);
        printColor("  - Region block or Private / Deleted video", RED);
        printColor("  - Network connection issues", RED);
        printColor("  - YouTube changed internal player algorithms", RED);
        cout << "\n1 - Update cookies\n2 - Try updating yt-dlp\n0 - Main menu (ESC)\nYour choice: ";
        char ch = getMenuChoice();
        if (ch == 27 || ch == '0') {
            cout << "ESC" << endl;
            LAST_ERROR = NOT_ERROR;
            cookieErrorHandled = false;
            return;
        }
        cout << ch << endl;
        if (ch == '1') {
            clearScreen();
            if (cookieEditor(false) == "continue") {
                LAST_ERROR = NOT_ERROR;
                cookieErrorHandled = false;
                startDownload(u, s, e, isPl, true);
                return;
            }
        }
        else if (ch == '2') {
            printColor("\n[INFO] Downloading latest yt-dlp...", CYAN);
            string destExe = CONFIG_PATH + "yt-dlp.exe";
            if (downloadFile("https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe", destExe, "yt-dlp")) {
                YTDLP_FOUND = fileExists(destExe);
                YTDLP_PATH = destExe;
                printColor("\n[OK] yt-dlp updated successfully! Retrying download...", GREEN);
            }
            else {
                printColor("\n[ERROR] Failed to update yt-dlp.", RED);
            }
            Sleep(1500);
            LAST_ERROR = NOT_ERROR;
            cookieErrorHandled = false;
            startDownload(u, s, e, isPl, true);
            return;
        }
        LAST_ERROR = NOT_ERROR;
        cookieErrorHandled = false;
    }
    else {
        LAST_ERROR = NOT_ERROR;
        cookieErrorHandled = false;
        printColor("============================================", GREEN);
        printColor("[OK] Download completed successfully!", GREEN);
        printColor("============================================", GREEN);
        waitForKey();
    }
}

// ========== SETTINGS MENUS ==========
void codecRecompilerMenu() {
    clearScreen();
    printColor("========================================", CYAN);
    printColor(" CODEC RECOMPILER", CYAN);
    printColor("========================================", CYAN);
    cout << "\nStatus: [" << (CODEC_RECOMPILER ? "ENABLED" : "DISABLED") << "]\n";
    cout << "\nWhen enabled, yt-dlp and FFmpeg re-encode downloaded videos";
    cout << "\n(including 4K/VP9/AV1) into universal H.264 + AAC MP4 container.";
    cout << "\nThis ensures maximum compatibility with older TVs and players.\n";
    cout << "\n1. Toggle ON/OFF\n0. Return (ESC)\n\nYour choice: ";
    char ch = getMenuChoice();
    if (ch == '1') {
        CODEC_RECOMPILER = !CODEC_RECOMPILER;
        saveConfig();
        printColor("\n[OK] Codec recompiler is now " + string(CODEC_RECOMPILER ? "ON" : "OFF"), GREEN);
        waitForKey();
    }
}

void selectVideoQuality() {
    clearScreen();
    printColor("========================================", CYAN);
    printColor(" Select video resolution", CYAN);
    printColor("========================================", CYAN);
    cout << "\n1) 2160p (4k)\n2) 1440p (2k)\n3) 1080p (FullHD)\n4) 720p (HD)\n5) 480p\n6) 360p\n\nYour choice: ";
    char ch = getMenuChoice(); cout << ch << endl;
    switch (ch) {
    case '1': VIDEO_RESOLUTION = "2160"; break;
    case '2': VIDEO_RESOLUTION = "1440"; break;
    case '3': VIDEO_RESOLUTION = "1080"; break;
    case '4': VIDEO_RESOLUTION = "720"; break;
    case '5': VIDEO_RESOLUTION = "480"; break;
    case '6': VIDEO_RESOLUTION = "360"; break;
    default: printColor("[ERROR] Invalid choice!", RED); waitForKey(); return;
    }

    clearScreen();
    printColor("========================================", CYAN);
    printColor(" Select frame rate", CYAN);
    printColor("========================================", CYAN);
    cout << "\n1) 60fps (or best)\n2) 30fps\n\nYour choice: ";
    ch = getMenuChoice(); cout << ch << endl;
    switch (ch) {
    case '1': VIDEO_FPS = "60"; break;
    case '2': VIDEO_FPS = "30"; break;
    default: printColor("[ERROR] Invalid choice!", RED); waitForKey(); return;
    }

    clearScreen();
    printColor("========================================", CYAN);
    printColor(" Select video format", CYAN);
    printColor("========================================", CYAN);
    cout << "\n1) MP4(H.264)\n2) MP4(AV1)\n3) WEBM(AV1)\n4) WEBM(VP9)\n\nYour choice: ";
    ch = getMenuChoice(); cout << ch << endl;
    switch (ch) {
    case '1': VIDEO_FORMAT = "MP4(H.264)"; break;
    case '2': VIDEO_FORMAT = "MP4(AV1)"; break;
    case '3': VIDEO_FORMAT = "WEBM(AV1)"; break;
    case '4': VIDEO_FORMAT = "WEBM(VP9)"; break;
    default: printColor("[ERROR] Invalid choice!", RED); waitForKey(); return;
    }
    saveConfig();
    printColor("[OK] Video settings updated!", GREEN);
    waitForKey();
}

void selectAudioQuality() {
    clearScreen();
    printColor("========================================", CYAN);
    printColor(" Select audio format", CYAN);
    printColor("========================================", CYAN);
    cout << "\n1) M4A(AAC)\n2) WEBM(Opus)\n3) WEBM(Vorbis)\n\nYour choice: ";
    char ch = getMenuChoice(); cout << ch << endl;
    switch (ch) {
    case '1': AUDIO_FORMAT = "M4A(AAC)"; break;
    case '2': AUDIO_FORMAT = "WEBM(Opus)"; break;
    case '3': AUDIO_FORMAT = "WEBM(Vorbis)"; break;
    default: printColor("[ERROR] Invalid choice!", RED); waitForKey(); return;
    }
    saveConfig();
    printColor("[OK] Audio format updated!", GREEN);
    waitForKey();
}

void toggleOnlyAudio() {
    ONLY_AUDIO = !ONLY_AUDIO;
    if (ONLY_AUDIO) ONLY_VIDEO = false;
    saveConfig();
}

void toggleOnlyVideo() {
    ONLY_VIDEO = !ONLY_VIDEO;
    if (ONLY_VIDEO) ONLY_AUDIO = false;
    saveConfig();
}

// ========== UPDATE COMPONENTS ==========
void updateComponentsMenu() {
    clearScreen();
    printColor("========================================", CYAN);
    printColor(" COMPONENT UPDATER", CYAN);
    printColor("========================================", CYAN);
    cout << "\n1. Update yt-dlp (yt-dlp -U)\n2. Re-download FFmpeg\n3. Re-download QuickJS\n0. Return (ESC)\n\nYour choice: ";
    char ch = getMenuChoice();
    if (ch == '1') {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(" Downloading latest yt-dlp (~20MB)...", CYAN);
        printColor("========================================", CYAN);
        string destExe = CONFIG_PATH + "yt-dlp.exe";
        if (downloadFile("https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe", destExe, "yt-dlp")) {
            YTDLP_FOUND = fileExists(destExe);
            YTDLP_PATH = destExe;
            printColor("\n[OK] yt-dlp updated successfully!", GREEN);
        }
        else {
            printColor("\n[ERROR] Failed to update yt-dlp!", RED);
        }
        waitForKey();
    }
    else if (ch == '2') {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(" Downloading latest FFmpeg (~160MB)...", CYAN);
        printColor("========================================", CYAN);
        string zipFile = CONFIG_PATH + "ffmpeg.zip";
        if (downloadFile("https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip", zipFile, "FFmpeg")) {
            printComponentProgress("FFmpeg", 100.0, "Extracting components...");
            if (extractZip(zipFile, CONFIG_PATH)) {
                organizeExtractedTool("ffmpeg.exe", CONFIG_PATH);
                FFMPEG_FOUND = fileExists(CONFIG_PATH + "ffmpeg.exe");
                if (FFMPEG_FOUND) {
                    FFMPEG_PATH = CONFIG_PATH + "ffmpeg.exe";
                    cout << "\n";
                    printColor("[OK] FFmpeg updated successfully!", GREEN);
                }
                else {
                    cout << "\n";
                    printColor("[ERROR] Failed to locate ffmpeg.exe after extraction!", RED);
                }
            }
            else {
                cout << "\n";
                printColor("[ERROR] Failed to extract FFmpeg archive!", RED);
            }
            std::error_code ec;
            fs::remove(fs::u8path(zipFile), ec);
        }
        else {
            printColor("[ERROR] Failed to download FFmpeg!", RED);
        }
        waitForKey();
    }
    else if (ch == '3') {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(" Downloading QuickJS (~1MB)...", CYAN);
        printColor("========================================", CYAN);
        string zipFile = CONFIG_PATH + "quickjs.zip";
        if (downloadFile("https://bellard.org/quickjs/binary_releases/quickjs-win-x86_64-2026-06-04.zip", zipFile, "QuickJS")) {
            printComponentProgress("QuickJS", 100.0, "Extracting components...");
            if (extractZip(zipFile, CONFIG_PATH)) {
                organizeExtractedTool("qjs.exe", CONFIG_PATH);
                QJS_FOUND = fileExists(CONFIG_PATH + "qjs.exe");
                if (QJS_FOUND) {
                    QJS_PATH = CONFIG_PATH + "qjs.exe";
                    cout << "\n";
                    printColor("[OK] QuickJS updated successfully!", GREEN);
                }
                else {
                    cout << "\n";
                    printColor("[WARNING] Could not find qjs.exe in archive (QuickJS is optional).", YELLOW);
                }
            }
            else {
                cout << "\n";
                printColor("[WARNING] Failed to extract QuickJS archive.", YELLOW);
            }
            std::error_code ec;
            fs::remove(fs::u8path(zipFile), ec);
        }
        else {
            printColor("[WARNING] Failed to download QuickJS (QuickJS is optional).", YELLOW);
        }
        waitForKey();
    }
}

// ========== SETTINGS ==========
void settingsMenu() {
    while (true) {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(" SETTINGS", CYAN);
        printColor("========================================", CYAN);
        cout << "\n1. Download location: [" << DOWNLOAD_PATH << "]"
            << "\n2. Video quality: [" << VIDEO_RESOLUTION << "p " << VIDEO_FPS << "fps " << VIDEO_FORMAT << "]"
            << "\n3. Audio quality: [" << AUDIO_FORMAT << "]"
            << "\n4. Codec recompiler: [" << (CODEC_RECOMPILER ? "ON" : "OFF") << "]"
            << "\n5. Only audio: [" << (ONLY_AUDIO ? "ON" : "OFF") << "]"
            << "\n6. Only video: [" << (ONLY_VIDEO ? "ON" : "OFF") << "]"
            << "\n7. Update cookies"
            << "\n8. Update yt-dlp & components"
            << "\n0. Return (ESC)\n\nYour choice: ";
        char ch = getMenuChoice();
        if (ch == 27 || ch == '0') {
            return;
        }
        cout << ch << endl;
        switch (ch) {
        case '1': {
            string p = openFolderDialog();
            if (!p.empty()) {
                DOWNLOAD_PATH = p;
                if (!dirExists(DOWNLOAD_PATH)) createDirRecursive(DOWNLOAD_PATH);
                saveConfig();
                printColor("[OK] Download location updated!", GREEN);
            }
            else {
                printColor("[INFO] Not changed", YELLOW);
            }
            waitForKey();
            break;
        }
        case '2': selectVideoQuality(); break;
        case '3': selectAudioQuality(); break;
        case '4': codecRecompilerMenu(); break;
        case '5': toggleOnlyAudio(); break;
        case '6': toggleOnlyVideo(); break;
        case '7': clearScreen(); cookieEditor(true); saveConfig(); break;
        case '8': updateComponentsMenu(); break;
        default: printColor("[ERROR] Invalid choice!", RED); waitForKey();
        }
    }
}

// ========== DEPENDENCY CHECKS & AUTO INSTALLER ==========
bool checkDependencies() {
    YTDLP_PATH = CONFIG_PATH + "yt-dlp.exe";
    FFMPEG_PATH = CONFIG_PATH + "ffmpeg.exe";
    QJS_PATH = CONFIG_PATH + "qjs.exe";

    YTDLP_FOUND = fileExists(YTDLP_PATH);
    FFMPEG_FOUND = fileExists(FFMPEG_PATH);
    QJS_FOUND = fileExists(QJS_PATH);

    if (!YTDLP_FOUND || !FFMPEG_FOUND || !QJS_FOUND) {
        printColor("============================================", RED);
        printColor("[ERROR] Missing recommended components on your computer", RED);
        printColor("============================================", RED);

        string missing = "";
        if (!YTDLP_FOUND) missing += "yt-dlp ";
        if (!FFMPEG_FOUND) missing += "FFmpeg ";
        if (!QJS_FOUND) missing += "QuickJS ";

        printColor("\n============================================", CYAN);
        printColor(" AUTO INSTALLER", CYAN);
        printColor("============================================", CYAN);

        while (true) {
            printColor("\nInstall " + missing + "automatically? (y/n): \n", CYAN, false);

            char ch = getMenuChoice();

            if (ch == 'y' || ch == 'Y') {
                cout << "y" << endl;
                break;
            }
            else if (ch == 'n' || ch == 'N' || ch == 27) {
                cout << "n" << endl;
                if (!YTDLP_FOUND) {
                    printColor("\n============================================", YELLOW);
                    printColor(" [ERROR] yt-dlp is required to run the application!", RED);
                    printColor(" [INFO] Download from: https://github.com/yt-dlp/yt-dlp/releases", YELLOW);
                    printColor(" [INFO] Place 'yt-dlp.exe' in: " + CONFIG_PATH, YELLOW);
                    printColor("============================================", YELLOW);
                    waitForKey();
                    return false;
                }
                // If yt-dlp is present but ffmpeg/qjs are skipped, continue with warning
                printColor("\n[WARNING] Proceeding without optional components. Some features may be limited.", YELLOW);
                Sleep(1500);
                return true;
            }
        }

        // ========== INSTALLATION ==========
        if (!dirExists(CONFIG_PATH)) {
            createDirRecursive(CONFIG_PATH);
        }

        if (!YTDLP_FOUND) {
            printColor("\n[INFO] Installing yt-dlp (~20MB)...", CYAN);
            if (downloadFile("https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe", CONFIG_PATH + "yt-dlp.exe", "yt-dlp")) {
                YTDLP_FOUND = true;
                printColor("[OK] yt-dlp installed successfully!", GREEN);
            }
            else {
                printColor("[ERROR] Failed to install yt-dlp!", RED);
            }
        }

        if (!FFMPEG_FOUND) {
            printColor("\n[INFO] Installing FFmpeg (~160MB)...", CYAN);
            string zipFile = CONFIG_PATH + "ffmpeg.zip";
            if (downloadFile("https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip", zipFile, "FFmpeg")) {
                printComponentProgress("FFmpeg", 100.0, "Extracting components...");
                if (extractZip(zipFile, CONFIG_PATH)) {
                    organizeExtractedTool("ffmpeg.exe", CONFIG_PATH);
                    FFMPEG_FOUND = fileExists(CONFIG_PATH + "ffmpeg.exe");
                    if (FFMPEG_FOUND) {
                        FFMPEG_PATH = CONFIG_PATH + "ffmpeg.exe";
                        cout << "\n";
                        printColor("[OK] FFmpeg and FFprobe installed successfully!", GREEN);
                    }
                    else {
                        cout << "\n";
                        printColor("[ERROR] Failed to locate ffmpeg.exe after extraction!", RED);
                    }
                }
                else {
                    cout << "\n";
                    printColor("[ERROR] Failed to extract FFmpeg archive!", RED);
                }

                std::error_code ec;
                fs::remove(fs::u8path(zipFile), ec);
            }
            else {
                printColor("[ERROR] Failed to download FFmpeg!", RED);
            }
        }

        if (!QJS_FOUND) {
            printColor("\n[INFO] Installing QuickJS (~1MB)...", CYAN);
            string zipFile = CONFIG_PATH + "quickjs.zip";
            if (downloadFile("https://bellard.org/quickjs/binary_releases/quickjs-win-x86_64-2026-06-04.zip", zipFile, "QuickJS")) {
                printComponentProgress("QuickJS", 100.0, "Extracting components...");
                if (extractZip(zipFile, CONFIG_PATH)) {
                    organizeExtractedTool("qjs.exe", CONFIG_PATH);
                    QJS_FOUND = fileExists(CONFIG_PATH + "qjs.exe");
                    if (QJS_FOUND) {
                        QJS_PATH = CONFIG_PATH + "qjs.exe";
                        cout << "\n";
                        printColor("[OK] QuickJS installed successfully!", GREEN);
                    }
                    else {
                        cout << "\n";
                        printColor("[WARNING] Could not find qjs.exe in archive (QuickJS is optional).", YELLOW);
                    }
                }
                else {
                    cout << "\n";
                    printColor("[WARNING] Failed to extract QuickJS archive.", YELLOW);
                }

                std::error_code ec;
                fs::remove(fs::u8path(zipFile), ec);
            }
            else {
                printColor("[WARNING] Failed to download QuickJS (QuickJS is optional).", YELLOW);
            }
        }

        // Final verification
        YTDLP_FOUND = fileExists(YTDLP_PATH);
        FFMPEG_FOUND = fileExists(FFMPEG_PATH);
        QJS_FOUND = fileExists(QJS_PATH);

        if (YTDLP_FOUND) {
            printColor("\n[OK] Dependencies check completed!", GREEN);
            Sleep(1000);
            return true;
        }
        else {
            printColor("\n[ERROR] yt-dlp is missing and could not be installed!", RED);
            waitForKey();
            return false;
        }
    }

    return true;
}

// ========== MAIN MENU ==========
void displayMenu() {
    clearScreen();
    printColor("========================================", CYAN);
    printColor(" MR CLI FOR YT DLP v1.1.0", CYAN);
    printColor("========================================", CYAN);
    printColor("========================================", GREEN);
    printColor(" YT-DLP:  " + string(YTDLP_FOUND ? "[OK] installed" : "[ERROR] not found"), YTDLP_FOUND ? GREEN : RED);
    printColor(" FFMPEG:  " + string(FFMPEG_FOUND ? "[OK] installed" : "[WARNING] not installed"), FFMPEG_FOUND ? GREEN : YELLOW);
    printColor(" QuickJS: " + string(QJS_FOUND ? "[OK] installed" : "[WARNING] not installed"), QJS_FOUND ? GREEN : YELLOW);
    printColor("========================================", GREEN);
    cout << "========================================\n1. Start download\n2. Settings\n0. Exit\n========================================\n\nYour number choice: ";
}

int main() {
    atexit(cleanupTempCookiesFile);

    // Initialize COM for Windows Shell, Folder Dialogs, and Shell Zip extraction
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    setUTF8();

    // Create base directories in User Documents
    wchar_t docPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, docPath))) {
        wstring basePath = wstring(docPath) + L"\\MR-CLI-FOR-YT-DLP\\";
        string basePathStr = wstringToUtf8(basePath);

        CONFIG_PATH = basePathStr + "configs\\";
        SCRIPT_DIR = basePathStr;
        DOWNLOAD_PATH = basePathStr + "downloads\\";

        if (!dirExists(CONFIG_PATH)) createDirRecursive(CONFIG_PATH);
        if (!dirExists(DOWNLOAD_PATH)) createDirRecursive(DOWNLOAD_PATH);
    }
    else {
        CONFIG_PATH = "C:\\MR-CLI-FOR-YT-DLP\\configs\\";
        DOWNLOAD_PATH = "C:\\MR-CLI-FOR-YT-DLP\\downloads\\";
        if (!dirExists(CONFIG_PATH)) createDirRecursive(CONFIG_PATH);
        if (!dirExists(DOWNLOAD_PATH)) createDirRecursive(DOWNLOAD_PATH);
    }

    ARCHIVE_PATH = getArchivePath();
    loadConfig();

    if (!checkDependencies()) {
        CoUninitialize();
        return 1;
    }

    while (true) {
        displayMenu();
        char ch = getMenuChoice();
        cout << ch << "\n\n";
        switch (ch) {
        case '1': startDownload(); break;
        case '2': settingsMenu(); break;
        case '0':
            cout << "Exiting...\n";
            CoUninitialize();
            return 0;
        default:
            printColor("[ERROR] Invalid choice!", RED);
            waitForKey();
        }
    }

    CoUninitialize();
    return 0;
}