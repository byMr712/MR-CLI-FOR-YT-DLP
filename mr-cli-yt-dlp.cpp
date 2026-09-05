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
#include <shobjidl.h>
#include <commdlg.h>
#include <locale>
#include <cctype>
#include <filesystem>
#include <wininet.h>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "User32.lib")

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
bool USE_COOKIES = true, CODEC_RECOMPILER = false, ONLY_AUDIO = false, ONLY_VIDEO = false, USE_ARCHIVE = true;
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

// ========== LANGUAGE ==========
enum Language { LANG_EN = 0, LANG_RU = 1 };
Language CURRENT_LANG = LANG_EN;

void initDefaultLanguage() {
    WORD langId = GetUserDefaultUILanguage();
    WORD primary = PRIMARYLANGID(langId);
    if (primary == LANG_RUSSIAN || primary == LANG_BELARUSIAN || primary == LANG_UKRAINIAN) {
        CURRENT_LANG = LANG_RU;
    } else {
        CURRENT_LANG = LANG_EN;
    }
}

// ========== BILINGUAL KEYBOARD MAPPING (QWERTY <-> ЙЦУКЕН) ==========
char normalizeKeyToEnglish(wint_t wc) {
    if (wc < 128) return (char)wc;
    switch (wc) {
        case 0x0439: case 0x0419: return 'q';  // й Й
        case 0x0446: case 0x0426: return 'w';  // ц Ц
        case 0x0443: case 0x0423: return 'e';  // у У
        case 0x043A: case 0x041A: return 'r';  // к К
        case 0x0435: case 0x0415: return 't';  // е Е
        case 0x043D: case 0x041D: return 'y';  // н Н
        case 0x0433: case 0x0413: return 'u';  // г Г
        case 0x0448: case 0x0428: return 'i';  // ш Ш
        case 0x0449: case 0x0429: return 'o';  // щ Щ
        case 0x0437: case 0x0417: return 'p';  // з З
        case 0x0445: case 0x0425: return '[';  // х Х
        case 0x044A: case 0x042A: return ']';  // ъ Ъ
        case 0x0444: case 0x0424: return 'a';  // ф Ф
        case 0x044B: case 0x042B: return 's';  // ы Ы
        case 0x0432: case 0x0412: return 'd';  // в В
        case 0x0430: case 0x0410: return 'f';  // а А
        case 0x043F: case 0x041F: return 'g';  // п П
        case 0x0440: case 0x0420: return 'h';  // р Р
        case 0x043E: case 0x041E: return 'j';  // о О
        case 0x043B: case 0x041B: return 'k';  // л Л
        case 0x0434: case 0x0414: return 'l';  // д Д
        case 0x0436: case 0x0416: return ';';  // ж Ж
        case 0x044D: case 0x042D: return '\''; // э Э
        case 0x044F: case 0x042F: return 'z';  // я Я
        case 0x0447: case 0x0427: return 'x';  // ч Ч
        case 0x0441: case 0x0421: return 'c';  // с С
        case 0x043C: case 0x041C: return 'v';  // м М
        case 0x0438: case 0x0418: return 'b';  // и И
        case 0x0442: case 0x0422: return 'n';  // т Т
        case 0x044C: case 0x042C: return 'm';  // ь Ь
        case 0x0431: case 0x0411: return ',';  // б Б
        case 0x044E: case 0x042E: return '.';  // ю Ю
        default: return (char)(wc & 0xFF);
    }
}

// ========== LOCALIZATION ==========
string tr(const string& en, const string& ru) {
    return (CURRENT_LANG == LANG_RU) ? ru : en;
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
    SetConsoleTitleW(L"MR CLI FOR YT-DLP v1.1.3");
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
    cout << "\n" << tr("Press any key...", "Нажмите любую клавишу...") << flush;
    (void)_getch();
    while (_kbhit()) (void)_getch();
    cout << "\n";
}

// ========== ARROW-KEY SELECTION MENU ==========
int arrowSelect(const string& title, const string& description, const vector<string>& options, int currentIdx, const vector<string>& hints = {}) {
    int selected = (currentIdx >= 0 && currentIdx < (int)options.size()) ? currentIdx : 0;
    while (true) {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(" " + title, CYAN);
        printColor("========================================", CYAN);
        if (!description.empty()) {
            cout << "\n" << description << "\n";
        }
        cout << "\n";
        for (int i = 0; i < (int)options.size(); i++) {
            if (i == selected) {
                setColor(GREEN);
                cout << " > " << options[i] << endl;
                setColor(WHITE);
            } else {
                cout << "   " << options[i] << endl;
            }
        }
        if (!hints.empty() && selected >= 0 && selected < (int)hints.size() && !hints[selected].empty()) {
            cout << "\n";
            printColor("----------------------------------------------------------------------", CYAN);
            setColor(YELLOW);
            cout << " [i] " << hints[selected] << "\n";
            setColor(WHITE);
            printColor("----------------------------------------------------------------------", CYAN);
        }
        cout << "\n" << tr("Arrow keys to select, Enter to confirm, ESC to cancel",
                           "Стрелки для выбора, Enter для подтверждения, ESC для отмены") << endl;

        wint_t key = _getwch();
        if (key == 27) return -1;
        if (key == 13) return selected;
        if (key == 0 || key == 0xE0) {
            wint_t scan = _getwch();
            if (scan == 72) selected = (selected > 0) ? selected - 1 : (int)options.size() - 1;
            else if (scan == 80) selected = (selected < (int)options.size() - 1) ? selected + 1 : 0;
        } else {
            char ch = normalizeKeyToEnglish(key);
            if (ch >= '1' && ch <= '9') {
                int idx = ch - '1';
                if (idx < (int)options.size()) return idx;
            }
            if (ch == '0' || ch == 27) return -1;
            for (int i = 0; i < (int)options.size(); i++) {
                if (options[i].length() >= 2 && options[i][0] == ' ' && options[i][1] == ch) {
                    return i;
                }
                if (options[i].length() >= 1 && options[i][0] == ch) {
                    return i;
                }
            }
        }
    }
}

// ========== CLIPBOARD HELPERS ==========
string getClipboard() {
    if (!OpenClipboard(NULL)) return "";

    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (!h) {
        CloseClipboard();
        return "";
    }

    wchar_t* t = (wchar_t*)GlobalLock(h);
    if (!t) {
        CloseClipboard();
        return "";
    }

    wstring r(t);
    GlobalUnlock(h);
    CloseClipboard();
    return wstringToUtf8(r);
}

// ========== KEYBOARD INPUT (WITH ESCAPE TO CANCEL, CTRL+V, BACKSPACE) ==========
bool inputLineWithEscape(string& result, const string& prompt) {
    if (!prompt.empty()) cout << prompt << flush;
    result.clear();

    wstring buffer;

    while (true) {
        wint_t wc = _getwch();

        if (wc == 27) { // ESC key
            cout << "\n";
            return false;
        }

        if (wc == 13) { // Enter (\r)
            cout << "\n";
            break;
        }

        if (wc == 8) { // Backspace
            if (!buffer.empty()) {
                buffer.pop_back();
                cout << "\b \b" << flush;
            }
            continue;
        }

        if (wc == 22) { // Ctrl+V (Paste)
            string clip = getClipboard();
            while (!clip.empty() && (clip.back() == '\r' || clip.back() == '\n')) clip.pop_back();
            if (!clip.empty()) {
                wstring wclip = utf8ToWstring(clip);
                buffer += wclip;
                cout << clip << flush;
            }
            continue;
        }

        if (wc == 3) { // Ctrl+C
            cout << "\n";
            return false;
        }

        if (wc == 0 || wc == 0xE0) { // Extended keys (arrows, F-keys, etc.)
            (void)_getwch(); // consume the secondary scan code
            continue;
        }

        if (wc >= 32) {
            buffer.push_back((wchar_t)wc);
            cout << wstringToUtf8(wstring(1, (wchar_t)wc)) << flush;
        }
    }

    result = wstringToUtf8(buffer);
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
    HINTERNET hSession = InternetOpenW(
        L"Mozilla/5.0 (compatible; MRCLI/1.0)",
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL, NULL, 0
    );
    if (!hSession) return false;

    wstring wUrl = utf8ToWstring(url);
    DWORD httpFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_UI;
    HINTERNET hReq = InternetOpenUrlW(hSession, wUrl.c_str(), NULL, 0, httpFlags, 0);
    if (!hReq) {
        InternetCloseHandle(hSession);
        return false;
    }

    // Query Content-Length
    DWORD contentLength = 0;
    DWORD clLen = sizeof(contentLength);
    DWORD idx = 0;
    HttpQueryInfoW(hReq, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &clLen, &idx);

    string tmpFile = destFile + ".tmp";
    wstring wTmp = utf8ToWstring(tmpFile);
    wstring wDest = utf8ToWstring(destFile);

    HANDLE hFile = CreateFileW(
        wTmp.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        InternetCloseHandle(hReq);
        InternetCloseHandle(hSession);
        return false;
    }

    char buffer[32768];
    DWORD bytesRead = 0;
    DWORD totalDownloaded = 0;

    while (InternetReadFile(hReq, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        DWORD dwWritten = 0;
        WriteFile(hFile, buffer, bytesRead, &dwWritten, NULL);
        totalDownloaded += bytesRead;
        if (contentLength > 0) {
            double pct = ((double)totalDownloaded / (double)contentLength) * 100.0;
            double curMB = (double)totalDownloaded / (1048576.0);
            double totMB = (double)contentLength / (1048576.0);
            char info[64];
            snprintf(info, sizeof(info), "(%.1f / %.1f MB)", curMB, totMB);
            printComponentProgress(label, pct, info);
        } else {
            double curMB = (double)totalDownloaded / (1048576.0);
            char info[64];
            snprintf(info, sizeof(info), "(%.1f MB)", curMB);
            printComponentProgress(label, 0, info);
        }
    }

    CloseHandle(hFile);
    InternetCloseHandle(hReq);
    InternetCloseHandle(hSession);

    MoveFileExW(wTmp.c_str(), wDest.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
    cout << "\n";
    return fileExists(destFile) && (totalDownloaded > 1000);
}

bool extractZip(const string& zipPath, const string& destDir) {
    if (!fileExists(zipPath) || !dirExists(destDir)) return false;

    wchar_t sysDir[MAX_PATH];
    if (GetSystemDirectoryW(sysDir, MAX_PATH) <= 0) return false;

    wstring tarExe = wstring(sysDir) + L"\\tar.exe";
    if (GetFileAttributesW(tarExe.c_str()) == INVALID_FILE_ATTRIBUTES) return false;

    wstring wDest = utf8ToWstring(destDir);
    while (!wDest.empty() && (wDest.back() == L'\\' || wDest.back() == L'/')) wDest.pop_back();

    wstring wZip = utf8ToWstring(zipPath);
    while (!wZip.empty() && (wZip.back() == L'\\' || wZip.back() == L'/')) wZip.pop_back();

    wstring cmd = L"\"" + tarExe + L"\" -xf \"" + wZip + L"\" -C \"" + wDest + L"\"";
    return (runProcessWait(cmd) == 0);
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
    if (!eta.empty()) line += "  " + tr("ETA ", "Осталось ") + eta;
    line += "        ";
    cout << line << flush;
}

// ========== DOWNLOAD ARCHIVE ==========
string getArchivePath() {
    return CONFIG_PATH + "archive.txt";
}

void removeIdFromArchive(const string& id) {
    if (id.empty() || id == "Destination") return;
    string arcPath = getArchivePath();
    if (!fileExists(arcPath)) return;
    ifstream in(fs::u8path(arcPath));
    if (!in.is_open()) return;
    vector<string> lines;
    string l;
    bool found = false;
    while (getline(in, l)) {
        while (!l.empty() && (l.back() == '\r' || l.back() == '\n')) l.pop_back();
        if (l.empty()) continue;
        if (l.find(id) != string::npos) {
            found = true;
            continue;
        }
        lines.push_back(l);
    }
    in.close();
    if (found) {
        ofstream out(fs::u8path(arcPath), ios::trunc | ios::binary);
        if (out.is_open()) {
            for (const auto& line : lines) {
                out << line << "\n";
            }
            out.close();
        }
    }
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
        << "USE_ARCHIVE=" << (USE_ARCHIVE ? "true" : "false") << "\n"
        << "ONLY_AUDIO=" << (ONLY_AUDIO ? "true" : "false") << "\n"
        << "ONLY_VIDEO=" << (ONLY_VIDEO ? "true" : "false") << "\n"
        << "LANGUAGE=" << (CURRENT_LANG == LANG_RU ? "ru" : "en") << "\n";
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
                else if (l.find("USE_ARCHIVE=") == 0) USE_ARCHIVE = (l.substr(12) == "true");
                else if (l.find("ONLY_AUDIO=") == 0) ONLY_AUDIO = (l.substr(11) == "true");
                else if (l.find("ONLY_VIDEO=") == 0) ONLY_VIDEO = (l.substr(11) == "true");
                else if (l.find("LANGUAGE=") == 0) CURRENT_LANG = (l.substr(9) == "ru") ? LANG_RU : LANG_EN;
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
string openFolderDialog(const wchar_t* title = L"Select folder") {
    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pfd));
    if (FAILED(hr)) return "";

    DWORD dwOptions;
    pfd->GetOptions(&dwOptions);
    pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    pfd->SetTitle(title);

    hr = pfd->Show(nullptr);
    if (FAILED(hr)) { pfd->Release(); return ""; }

    IShellItem* psi = nullptr;
    hr = pfd->GetResult(&psi);
    if (FAILED(hr)) { pfd->Release(); return ""; }

    wchar_t* pPath = nullptr;
    hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pPath);
    psi->Release();
    pfd->Release();

    if (FAILED(hr) || !pPath) return "";

    wstring result(pPath);
    CoTaskMemFree(pPath);
    if (result.back() != L'\\' && result.back() != L'/') result += L'\\';
    return wstringToUtf8(result);
}

string openFileDialog() {
    OPENFILENAMEW ofn = { 0 };
    wchar_t fn[MAX_PATH] = L"";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetConsoleWindow();
    ofn.lpstrFilter = (CURRENT_LANG == LANG_RU) ?
        L"Текстовые файлы (*.txt)\0*.txt\0Все файлы (*.*)\0*.*\0" :
        L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fn;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    wstring dlgTitle = (CURRENT_LANG == LANG_RU) ? L"Выберите файл куки" : L"Select cookies file";
    ofn.lpstrTitle = dlgTitle.c_str();

    if (GetOpenFileNameW(&ofn)) {
        return wstringToUtf8(wstring(fn));
    }
    return "";
}

// ========== COOKIE STORAGE HELPERS ==========
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

string getSavedCookiesPath() {
    return CONFIG_PATH + "cookies.txt";
}

bool hasSavedCookies() {
    return fileExists(getSavedCookiesPath());
}

bool saveCookies(const string& c) {
    string fullCookies = c;
    if (fullCookies.find("# Netscape HTTP Cookie File") == string::npos && fullCookies.find("[") != 0) {
        fullCookies = "# Netscape HTTP Cookie File\n# MR CLI FOR YT DLP v1.1.3\n\n" + fullCookies;
    }
    string txtPath = getSavedCookiesPath();
    ofstream f(fs::u8path(txtPath), ios::binary);
    if (f.is_open()) {
        f.write(fullCookies.data(), fullCookies.size());
        f.close();
        return true;
    }
    return false;
}

// ========== COOKIE EDITOR MENU ==========
string cookieEditor(bool fromSettings = false) {
    while (true) {
        string status = tr("Status: ", "Статус: ") + (hasSavedCookies() ? tr("[CONFIGURED]", "[НАСТРОЕНО]") : tr("[NOT CONFIGURED]", "[НЕ НАСТРОЕНО]"));
        string desc = "\n" + status + "\n";

        vector<string> options = {
            tr("Select cookies file (.txt)", "Выбрать файл куки (.txt)"),
            tr("Paste from clipboard", "Вставить из буфера обмена"),
            tr("Edit in Notepad", "Редактировать в Блокноте"),
            tr("Delete saved cookies", "Удалить сохранённые куки")
        };
        vector<string> hints = {
            tr("Open file picker to select a Netscape-formatted cookies.txt file", "Открыть проводник для выбора файла куки cookies.txt в формате Netscape"),
            tr("Paste Netscape cookie text or JSON cookies directly from clipboard", "Вставить куки напрямую из буфера обмена (в формате Netscape или JSON)"),
            tr("Open cookies file in Notepad to view or edit cookies manually", "Открыть файл куки в Блокноте для ручного редактирования"),
            tr("Remove saved cookies file and reset cookie settings", "Удалить сохранённый файл куки и сбросить настройки")
        };

        int sel = arrowSelect(tr("COOKIE EDITOR", "РЕДАКТОР КУКИ"), desc, options, 0, hints);
        if (sel < 0) {
            return "exit";
        }

        switch (sel) {
        case 0: {
            string p = openFileDialog();
            if (p.empty()) {
                printColor(tr("[INFO] Selection cancelled", "[ИНФО] Выбор отменён"), YELLOW);
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
                    printColor(tr("[OK] Cookies saved successfully!", "[OK] Куки успешно сохранены!"), GREEN);
                    waitForKey();
                    return fromSettings ? "settings" : "continue";
                }
            }
            else {
                printColor(tr("[ERROR] Invalid cookies file! Netscape format required.", "[ОШИБКА] Неверный файл куки! Требуется формат Netscape."), RED);
                waitForKey();
            }
            break;
        }
        case 1: {
            string c = getClipboard();
            if (c.empty()) {
                printColor(tr("[ERROR] Clipboard is empty!", "[ОШИБКА] Буфер обмена пуст!"), RED);
                waitForKey();
                break;
            }
            if (validateCookiesContent(c) && saveCookies(c)) {
                USE_COOKIES = true;
                saveConfig();
                printColor(tr("[OK] Cookies saved successfully!", "[OK] Куки успешно сохранены!"), GREEN);
                waitForKey();
                return fromSettings ? "settings" : "continue";
            }
            printColor(tr("[ERROR] Invalid cookies format in clipboard!", "[ОШИБКА] Неверный формат куки в буфере обмена!"), RED);
            waitForKey();
            break;
        }
        case 2: {
            string txtPath = getSavedCookiesPath();
            if (!fileExists(txtPath)) {
                saveCookies("# Netscape HTTP Cookie File\n# Paste cookies here, save, and close Notepad\n\n");
            }
            wstring cmd = L"notepad.exe \"" + utf8ToWstring(txtPath) + L"\"";
            runProcessWait(cmd);
            if (fileExists(txtPath) && validateCookies(txtPath)) {
                USE_COOKIES = true;
                saveConfig();
                printColor(tr("[OK] Cookies updated successfully!", "[OK] Куки успешно обновлены!"), GREEN);
                waitForKey();
                return fromSettings ? "settings" : "continue";
            }
            printColor(tr("[INFO] No valid cookies saved.", "[ИНФО] Допустимые куки не сохранены."), YELLOW);
            waitForKey();
            break;
        }
        case 3: {
            std::error_code ec;
            fs::remove(fs::u8path(CONFIG_PATH + "cookies.txt"), ec);
            fs::remove(fs::u8path(CONFIG_PATH + "cookies.dat"), ec);
            USE_COOKIES = false;
            saveConfig();
            printColor(tr("[OK] Saved cookies removed!", "[OK] Сохранённые куки удалены!"), GREEN);
            waitForKey();
            break;
        }
        default:
            return "exit";
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

// ========== STREAM TYPE DETECTION ==========
bool isAudioStream(const string& path, const string& ext) {
    if (ONLY_AUDIO) return true;
    if (ONLY_VIDEO) return false;

    string extLower = ext;
    for (char &ch : extLower) ch = (char)tolower((unsigned char)ch);

    if (extLower == "m4a" || extLower == "aac" || extLower == "opus" ||
        extLower == "ogg" || extLower == "mp3" || extLower == "flac" ||
        extLower == "wav" || extLower == "weba" || extLower == "oga" || extLower == "mka") {
        return true;
    }

    static const char* audioTags[] = {
        ".f139.", ".f140.", ".f141.", ".f249.", ".f250.", ".f251.",
        ".f256.", ".f258.", ".f325.", ".f328.", ".f599.", ".f600.",
        ".ba.", ".audio.", "_audio."
    };
    for (const char* tag : audioTags) {
        if (path.find(tag) != string::npos) return true;
    }

    if (extLower == "mp4" || extLower == "mkv" || extLower == "avi" ||
        extLower == "flv" || extLower == "mov" || extLower == "wmv" ||
        extLower == "ts" || extLower == "m4v") {
        return false;
    }

    static const char* videoTags[] = {
        ".f133.", ".f134.", ".f135.", ".f136.", ".f137.", ".f138.",
        ".f160.", ".f242.", ".f243.", ".f244.", ".f247.", ".f248.",
        ".f271.", ".f272.", ".f278.", ".f298.", ".f299.", ".f302.",
        ".f303.", ".f308.", ".f313.", ".f315.", ".f330.", ".f331.",
        ".f332.", ".f333.", ".f334.", ".f335.", ".f336.", ".f337.",
        ".f394.", ".f395.", ".f396.", ".f397.", ".f398.", ".f399.",
        ".f400.", ".f401.", ".f402.", ".f614.", ".f616.", ".f617.",
        ".f620.", ".f625.", ".f628.", ".bv.", ".video.", "_video."
    };
    for (const char* tag : videoTags) {
        if (path.find(tag) != string::npos) return false;
    }

    return false;
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
    int curItem = 0, totalItems = 0;
    string currentVideoId = "";
    bool currentVideoCompleted = false;
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
                if (!currentVideoId.empty()) {
                    removeIdFromArchive(currentVideoId);
                }
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

                    // Extract Video ID from extractor lines (e.g., "[youtube] 3RkgTGP_5fY: ...")
                    if (!line.empty() && line[0] == '[') {
                        size_t bClose = line.find(']');
                        if (bClose != string::npos && bClose + 2 < line.size()) {
                            size_t colon = line.find(':', bClose + 1);
                            if (colon != string::npos && colon > bClose + 2) {
                                string possibleId = line.substr(bClose + 2, colon - (bClose + 2));
                                while (!possibleId.empty() && possibleId.front() == ' ') possibleId.erase(possibleId.begin());
                                while (!possibleId.empty() && possibleId.back() == ' ') possibleId.pop_back();
                                if (possibleId != "Destination" && possibleId.find(' ') == string::npos && possibleId.size() >= 4 && possibleId.size() <= 64) {
                                    if (possibleId != currentVideoId) {
                                        if (!currentVideoCompleted && !currentVideoId.empty()) {
                                            removeIdFromArchive(currentVideoId);
                                        }
                                        currentVideoId = possibleId;
                                        currentVideoCompleted = false;
                                    }
                                }
                            }
                        }
                    }

                    if (line.find("[download] Downloading item ") == 0) {
                        int newCurItem = 0, newTotalItems = 0;
                        if (sscanf_s(line.c_str(), "[download] Downloading item %d of %d", &newCurItem, &newTotalItems) == 2) {
                            if (curItem > 0 && newCurItem != curItem) {
                                if (progressActive) { cout << endl; progressActive = false; }
                                cout << "\n";
                            }
                            if (newCurItem != curItem) {
                                currentVideoCompleted = false;
                            }
                            curItem = newCurItem;
                            totalItems = newTotalItems;
                            TOTAL_ITEMS_IN_PLAYLIST = newTotalItems;
                            LAST_PROCESSED_ITEM = curItem;
                            isPlaylist = (TOTAL_ITEMS_IN_PLAYLIST > 0);
                        }
                    }
                    else if (line.find("has already been recorded in the archive") != string::npos) {
                        if (progressActive) { cout << endl; progressActive = false; }
                        printColor(tr("[INFO] Video already downloaded (in archive, skipping).", "[ИНФО] Видео уже скачано (в архиве, пропуск)."), GREEN);
                        currentVideoCompleted = true;
                    }
                    else if (line.find("[download] Destination:") == 0) {
                        string path = line.substr(string("[download] Destination:").length());
                        while (!path.empty() && path.front() == ' ') path.erase(path.begin());

                        size_t slash = path.find_last_of("\\/");
                        string name = (slash != string::npos) ? path.substr(slash + 1) : path;
                        size_t dot = name.find_last_of('.');
                        string ext = "";
                        if (dot != string::npos) {
                            ext = name.substr(dot + 1);
                            name = name.substr(0, dot);
                        }

                        size_t fDot = name.find_last_of('.');
                        if (fDot != string::npos && fDot + 1 < name.size() && (name[fDot + 1] == 'f' || name[fDot + 1] == 'F')) {
                            bool allDigits = true;
                            for (size_t k = fDot + 2; k < name.size(); k++) {
                                if (!isdigit((unsigned char)name[k])) { allDigits = false; break; }
                            }
                            if (allDigits && fDot + 2 <= name.size()) {
                                name = name.substr(0, fDot);
                            }
                        }

                        // Strip leading playlist index prefix (e.g. "003 - ") from displayed name
                        if (totalItems > 0) {
                            size_t digitEnd = 0;
                            while (digitEnd < name.size() && isdigit((unsigned char)name[digitEnd])) {
                                digitEnd++;
                            }
                            if (digitEnd > 0 && digitEnd < name.size()) {
                                if (name.substr(digitEnd, 3) == " - ") {
                                    name = name.substr(digitEnd + 3);
                                }
                                else if (name.substr(digitEnd, 2) == "- ") {
                                    name = name.substr(digitEnd + 2);
                                }
                            }
                        }

                        if (progressActive) { cout << endl; progressActive = false; }

                        bool isAudio = isAudioStream(path, ext);
                        string mediaType = isAudio ? tr("Audio", "Аудио") : tr("Video", "Видео");

                        if (totalItems > 0)
                            printColor(tr("Downloading ", "Скачивание ") + mediaType + " " + to_string(curItem) + tr(" of ", " из ") + to_string(totalItems) + " - " + name, CYAN);
                        else
                            printColor(tr("Downloading ", "Скачивание ") + mediaType + tr(" into \"", " в \"") + path + "\"", CYAN);
                    }
                    else if (line.find("[Merger] Merging formats into") == 0) {
                        if (progressActive) { cout << endl; progressActive = false; }
                        string path = line.substr(string("[Merger] Merging formats into").length());
                        while (!path.empty() && path.front() == ' ') path.erase(path.begin());
                        if (!path.empty() && path.back() == '"') path.pop_back();
                        printColor(tr("[Merger] Merging video and audio into \"", "[Слияние] Объединение видео и аудио в \"") + path + "\"", CYAN);
                        currentVideoCompleted = true;
                    }
                    else if (parseDownloadLine(line, percent, speed, eta)) {
                        printProgressBar(percent, speed, eta);
                        progressActive = true;
                        if (percent == "100" || percent == "100.0" || percent == "100.0%") {
                            if (ONLY_AUDIO || ONLY_VIDEO) {
                                currentVideoCompleted = true;
                            }
                        }
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
        if (!currentVideoId.empty()) {
            removeIdFromArchive(currentVideoId);
        }
        return false;
    }

    // Manual FFmpeg merge fallback if required
    if (mergeFailed && !mergeErrorPath.empty() && FFMPEG_FOUND && !FFMPEG_PATH.empty()) {
        printColor(tr("\n[INFO] Merge failed. Trying manual FFmpeg merge...", "\n[ИНФО] Ошибка слияния. Попытка ручного слияния через FFmpeg..."), YELLOW);

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

            printColor(tr("[INFO] Running manual merge: ", "[ИНФО] Запуск ручного слияния: ") + mergeCmd, CYAN);
            int mergeResult = execCmd(mergeCmd);

            if (mergeResult == 0) {
                printColor(tr("[OK] Manual merge successful!", "[OK] Ручное слияние успешно завершено!"), GREEN);
                std::error_code ec;
                fs::remove(fs::u8path(videoFile), ec);
                fs::remove(fs::u8path(audioFile), ec);
                string finalFile = dir + baseName + ".mp4";
                fs::rename(fs::u8path(outputFile), fs::u8path(finalFile), ec);
                if (ec) {
                    printColor(tr("[WARNING] Merged file created as: ", "[ПРЕДУПРЕЖДЕНИЕ] Объединённый файл создан как: ") + outputFile, YELLOW);
                }
                else {
                    printColor(tr("[OK] Final file: ", "[OK] Итоговый файл: ") + finalFile, GREEN);
                }
                return true;
            }
        }
    }

    if (mergeFailed && !currentVideoId.empty()) {
        removeIdFromArchive(currentVideoId);
    }

    if (!currentVideoCompleted && !currentVideoId.empty() && (LAST_ERROR != NOT_ERROR || exitCode != 0)) {
        removeIdFromArchive(currentVideoId);
    }

    if (LAST_ERROR != NOT_ERROR) {
        return false;
    }

    if (totalItems > 0 && curItem >= totalItems) {
        return true;
    }

    return (exitCode == 0);
}

// ========== COMMAND BUILDING ==========
wstring buildCommand(const string& url, const string& start, const string& end, bool isPlaylist) {
    wstring cmd = L"\"" + utf8ToWstring(YTDLP_PATH.empty() ? (CONFIG_PATH + "yt-dlp.exe") : YTDLP_PATH) + L"\"";

    if (USE_COOKIES && hasSavedCookies()) {
        cmd += L" --cookies \"" + utf8ToWstring(getSavedCookiesPath()) + L"\"";
    }

    if (USE_ARCHIVE && !ARCHIVE_PATH.empty()) {
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
        printColor(retry ? tr(" RETRYING DOWNLOAD", " ПОВТОРНАЯ ПОПЫТКА СКАЧИВАНИЯ") : tr(" STARTING DOWNLOAD", " НАЧАЛО СКАЧИВАНИЯ"), CYAN);
        printColor("============================================", CYAN);
        cout << "\n" << tr("Enter URL (Paste: Ctrl+V, Cancel: ESC):\n", "Введите URL (Вставка: Ctrl+V, Отмена: ESC):\n");

        if (!inputLineWithEscape(u, "> ")) {
            printColor("\n" + tr("[INFO] Cancelled", "[ИНФО] Отменено"), YELLOW);
            waitForKey();
            return;
        }

        // Clean up input
        while (!u.empty() && (u.front() == ' ' || u.front() == '"' || u.front() == '\'')) u.erase(u.begin());
        while (!u.empty() && (u.back() == ' ' || u.back() == '"' || u.back() == '\'')) u.pop_back();

        if (u.empty()) {
            printColor(tr("[ERROR] URL cannot be empty!", "[ОШИБКА] URL не может быть пустым!"), RED);
            waitForKey();
            return;
        }

        // Playlist URL detection
        if (u.find("&list=") != string::npos && u.find("/playlist?list=") == string::npos) {
            vector<string> plOptions = {
                tr("Download entire playlist", "Скачать весь плейлист"),
                tr("Download only this video", "Скачать только это видео")
            };
            int plChoice = arrowSelect(
                tr("PLAYLIST DETECTED", "ОБНАРУЖЕН ПЛЕЙЛИСТ"),
                tr("URL contains both a video and a playlist:\nChoose download mode:",
                   "URL содержит и видео, и плейлист:\nВыберите режим скачивания:"),
                plOptions, 0
            );
            if (plChoice < 0) {
                printColor("\n" + tr("[INFO] Cancelled", "[ИНФО] Отменено"), YELLOW);
                waitForKey();
                return;
            }
            if (plChoice == 0) {
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
            cout << "\n" << tr("Start index in playlist (Enter=first, ESC to skip):\n", "Начальный номер в плейлисте (Enter=с начала, ESC=пропустить):\n");
            if (!inputLineWithEscape(inp, tr("Your choice: ", "Ваш выбор: "))) {
                cout << tr("[INFO] Defaulting to start\n", "[ИНФО] Выбрано начало плейлиста\n");
            }
            else if (!inp.empty()) {
                s = inp;
            }

            cout << "\n" << tr("End index in playlist (Enter=last, ESC to skip):\n", "Конечный номер в плейлисте (Enter=до конца, ESC=пропустить):\n");
            if (!inputLineWithEscape(inp, tr("Your choice: ", "Ваш выбор: "))) {
                cout << tr("[INFO] Defaulting to end\n", "[ИНФО] Выбран конец плейлиста\n");
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
    printColor(retry ? tr(" RETRYING DOWNLOAD...", " ПОВТОРНАЯ ПОПЫТКА СКАЧИВАНИЯ...") : tr(" STARTING DOWNLOAD...", " НАЧАЛО СКАЧИВАНИЯ..."), CYAN);
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
        printColor(tr("[ERROR] YouTube rate-limited requests. Waiting up to 1h (ESC to cancel)...", "[ОШИБКА] Превышен лимит запросов YouTube. Ожидание до 1 часа (ESC для отмены)..."), RED);
        printColor("============================================", RED);

        bool cancelled = false;
        for (int sec = 0; sec < 3660 && !cancelled; sec++) {
            if (_kbhit() && _getch() == 27) cancelled = true;
            else Sleep(1000);
            if (sec > 0 && sec % 300 == 0)
                printColor(tr("[INFO] ~", "[ИНФО] ~") + to_string((3660 - sec) / 60) + tr(" min remaining (ESC to cancel)", " мин осталось (ESC для отмены)"), YELLOW);
        }

        LAST_ERROR = NOT_ERROR;
        if (cancelled) {
            printColor(tr("[INFO] Cancelled by user.", "[ИНФО] Отменено пользователем."), YELLOW);
            waitForKey();
            return;
        }

        printColor("============================================", CYAN);
        printColor(tr("[INFO] Rate-limit cooldown complete. Continuing download...", "[ИНФО] Период ограничения завершён. Продолжение скачивания..."), CYAN);
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

        string reason = (LAST_ERROR == COOKIE_ERROR) ?
            tr("YouTube requires verification (Bot check / Login required)!\nPlease update cookies or use an account cookie file.",
               "YouTube требует подтверждения (Проверка на бота / Требуется вход)!\nПожалуйста, обновите куки или используйте файл куки аккаунта.") :
            tr("This video has age restriction!\nPlease use account cookies that can access this video.",
               "Это видео имеет возрастное ограничение!\nПожалуйста, используйте куки аккаунта с доступом к этому видео.");

        vector<string> ckOptions = {
            tr("Update cookies", "Обновить куки")
        };
        int selCk = arrowSelect(tr("AUTHENTICATION REQUIRED", "ТРЕБУЕТСЯ АВТОРИЗАЦИЯ"), reason, ckOptions, 0);
        if (selCk == 0) {
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
                printColor(tr("[OK] Playlist processing completed!", "[OK] Обработка плейлиста завершена!"), GREEN);
                printColor("============================================", GREEN);
                if (!skippedVideos.empty()) {
                    printColor("\n============================================", YELLOW);
                    printColor(tr("[INFO] Skipped items in this session:", "[ИНФО] Пропущенные элементы в этой сессии:"), YELLOW);
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
            printColor(tr("[INFO] Video ", "[ИНФО] Видео ") + to_string(currentIndex) + tr(" is a scheduled live stream. Skipping...", " является запланированной трансляцией. Пропуск..."), YELLOW);
            printColor("============================================", RED);
            Sleep(2000);

            LAST_ERROR = NOT_ERROR;
            startDownload(u, nextStart, e, true, true);
            return;
        }
        else {
            printColor("============================================", RED);
            printColor(tr("[ERROR] This video is a live stream that has not started yet.", "[ОШИБКА] Это видео является трансляцией, которая ещё не началась."), RED);
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
            printColor("\n" + tr("[INFO] Reached end of playlist range.", "[ИНФО] Достигнут конец диапазона плейлиста."), YELLOW);
            printColor(tr("[OK] Download finished!", "[OK] Скачивание завершено!"), GREEN);
            if (!skippedVideos.empty()) {
                printColor("\n============================================", YELLOW);
                printColor(tr("[INFO] Skipped items:", "[ИНФО] Пропущенные элементы:"), YELLOW);
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
            string skippedVideo = tr("Item ", "Элемент ") + to_string(currentIndex);
            skippedVideos.push_back(skippedVideo);
        }

        printColor("\n============================================", RED);
        printColor(tr("[ERROR] Video ", "[ОШИБКА] Видео ") + to_string(currentIndex) + tr(" could not be downloaded (skipping).", " не удалось скачать (пропуск)."), RED);
        printColor("============================================", RED);
        printColor("\n" + tr("[INFO] Continuing with next item in 3 seconds...\n", "[ИНФО] Продолжение со следующим элементом через 3 секунды...\n"), CYAN);
        Sleep(3000);
        LAST_ERROR = NOT_ERROR;
        startDownload(u, nextStart, e, true, true);
        return;
    }

    // ========== SINGLE VIDEO - ERRORS ==========
    if (!ok) {
        string desc = tr(
            "Download could not be completed!\n\n"
            "Possible reasons:\n"
            "  - Expired / missing cookies (Login required)\n"
            "  - Region block or Private / Deleted video\n"
            "  - Network connection issues\n"
            "  - YouTube changed internal player algorithms",
            "Не удалось завершить скачивание!\n\n"
            "Возможные причины:\n"
            "  - Истёкшие или отсутствующие куки (Требуется вход)\n"
            "  - Региональная блокировка или приватное / удалённое видео\n"
            "  - Проблемы с сетевым подключением\n"
            "  - YouTube изменил внутренние алгоритмы плеера"
        );
        vector<string> errOptions = {
            tr("Update cookies", "Обновить куки"),
            tr("Try updating yt-dlp", "Попробовать обновить yt-dlp")
        };
        int selErr = arrowSelect(tr("DOWNLOAD ERROR", "ОШИБКА СКАЧИВАНИЯ"), desc, errOptions, 0);
        if (selErr == 0) {
            clearScreen();
            if (cookieEditor(false) == "continue") {
                LAST_ERROR = NOT_ERROR;
                cookieErrorHandled = false;
                startDownload(u, s, e, isPl, true);
                return;
            }
        }
        else if (selErr == 1) {
            clearScreen();
            printColor("============================================", CYAN);
            printColor(tr("[INFO] Downloading latest yt-dlp...", "[ИНФО] Загрузка последней версии yt-dlp..."), CYAN);
            printColor("============================================", CYAN);
            string destExe = CONFIG_PATH + "yt-dlp.exe";
            if (downloadFile("https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe", destExe, "yt-dlp")) {
                YTDLP_FOUND = fileExists(destExe);
                YTDLP_PATH = destExe;
                printColor("\n" + tr("[OK] yt-dlp updated successfully! Retrying download...", "[OK] yt-dlp успешно обновлен! Повторная попытка..."), GREEN);
            }
            else {
                printColor("\n" + tr("[ERROR] Failed to update yt-dlp.", "[ОШИБКА] Не удалось обновить yt-dlp."), RED);
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
        printColor(tr("[OK] Download completed successfully!", "[OK] Скачивание успешно завершено!"), GREEN);
        printColor("============================================", GREEN);
        waitForKey();
    }
}

// ========== SETTINGS MENUS ==========
void codecRecompilerMenu() {
    string status = tr("Status: [", "Статус: [") +
                    (CODEC_RECOMPILER ? tr("ENABLED", "ВКЛЮЧЕНО") : tr("DISABLED", "ВЫКЛЮЧЕНО")) + "]";
    string desc = "\n" + status + "\n\n" +
                  tr("When enabled, yt-dlp and FFmpeg re-encode downloaded videos",
                     "Когда включено, yt-dlp и FFmpeg перекодируют скачанные видео") + "\n" +
                  tr("(including 4K/VP9/AV1) into universal H.264 + AAC MP4 container.",
                     "(включая 4K/VP9/AV1) в универсальный MP4-контейнер H.264 + AAC.") + "\n" +
                  tr("This ensures maximum compatibility with older TVs and players.",
                     "Это обеспечивает максимальную совместимость со старыми ТВ и плеерами.");

    vector<string> options = {
        tr("[ON] - Re-encode to H.264 MP4", "[ВКЛ] - Перекодировать в H.264 MP4"),
        tr("[OFF] - No re-encoding (default)", "[ВЫКЛ] - Без перекодирования (по умолчанию)")
    };

    vector<string> hints = {
        tr("Videos will be re-encoded for maximum device compatibility",
           "Видео будут перекодированы для максимальной совместимости с устройствами"),
        tr("Original video format is preserved",
           "Оригинальный формат видео сохраняется")
    };

    int sel = arrowSelect(
        tr("CODEC RECOMPILER", "ПЕРЕКОДИРОВЩИК КОДЕКОВ"),
        desc,
        options,
        CODEC_RECOMPILER ? 0 : 1,
        hints
    );

    if (sel >= 0) {
        CODEC_RECOMPILER = (sel == 0);
        saveConfig();
        printColor("\n" + tr("[OK] Codec recompiler is now ", "[OK] Перекодировщик кодеков теперь ") +
                   (CODEC_RECOMPILER ? tr("ON", "ВКЛ") : tr("OFF", "ВЫКЛ")), GREEN);
        waitForKey();
    }
}

void selectVideoQuality() {
    // 1. Resolution
    vector<string> resOpts = {
        "2160p (4K UHD)",
        "1440p (2K QHD)",
        "1080p (FullHD)",
        "720p (HD)",
        "480p (SD)",
        "360p (Low)"
    };
    vector<string> resVals = {"2160", "1440", "1080", "720", "480", "360"};
    vector<string> resHints = {
        tr("Ultra High Definition (3840x2160). High quality, large file size.", "Ультравысокое разрешение 4K (3840x2160). Отличное качество, большой размер."),
        tr("Quad High Definition (2560x1440). Great quality for 2K displays.", "Высокое разрешение 2K (2560x1440). Отличное качество для 2K мониторов."),
        tr("Full HD (1920x1080). Standard high definition for most displays.", "Full HD (1920x1080). Стандартное разрешение высокой четкости."),
        tr("HD (1280x720). Good balance of quality and file size.", "HD (1280x720). Хороший баланс качества и размера файла."),
        tr("Standard Definition (854x480). Compact file size for mobile devices.", "Стандартное разрешение (854x480). Компактный размер для телефонов."),
        tr("Low Definition (640x360). Lowest bandwidth and smallest file size.", "Низкое разрешение (640x360). Минимальный размер файла и трафик.")
    };
    int curRes = 2;
    for (int i = 0; i < (int)resVals.size(); i++) {
        if (resVals[i] == VIDEO_RESOLUTION) { curRes = i; break; }
    }
    int selRes = arrowSelect(tr("VIDEO RESOLUTION", "РАЗРЕШЕНИЕ ВИДЕО"),
        tr("Select preferred maximum video resolution:", "Выберите желаемое максимальное разрешение видео:"),
        resOpts, curRes, resHints);
    if (selRes < 0) return;
    VIDEO_RESOLUTION = resVals[selRes];

    // 2. Framerate
    vector<string> fpsOpts = {
        tr("60fps (or best available)", "60 кадр/с (или лучшая доступная)"),
        tr("30fps (standard)", "30 кадр/с (стандартная)")
    };
    vector<string> fpsVals = {"60", "30"};
    vector<string> fpsHints = {
        tr("Smooth high framerate playback (up to 60fps). Recommended for dynamic video.", "Плавное воспроизведение до 60 кадров/с. Рекомендуется для динамичных видео."),
        tr("Standard framerate (up to 30fps). Smaller file size.", "Стандартная частота до 30 кадров/с. Меньший размер файла.")
    };
    int curFps = (VIDEO_FPS == "30") ? 1 : 0;
    int selFps = arrowSelect(tr("FRAME RATE (FPS)", "ЧАСТОТА КАДРОВ (FPS)"),
        tr("Select preferred maximum frame rate:", "Выберите желаемую частоту кадров:"),
        fpsOpts, curFps, fpsHints);
    if (selFps < 0) return;
    VIDEO_FPS = fpsVals[selFps];

    // 3. Format
    vector<string> fmtOpts = {
        "MP4 (H.264 / AVC) - " + tr("Maximum compatibility", "Максимальная совместимость"),
        "MP4 (AV1)         - " + tr("Next-gen compression", "Новейшее сжатие"),
        "WEBM (AV1)        - " + tr("Open web format (AV1)", "Открытый веб-формат (AV1)"),
        "WEBM (VP9)        - " + tr("YouTube web format (VP9)", "Веб-формат YouTube (VP9)")
    };
    vector<string> fmtVals = {"MP4(H.264)", "MP4(AV1)", "WEBM(AV1)", "WEBM(VP9)"};
    vector<string> fmtHints = {
        tr("H.264 video in MP4 container. Universally compatible with all PCs, TVs, and mobile devices.", "H.264 в контейнере MP4. Воспроизводится на любых смартфонах, ТВ и компьютерах."),
        tr("AV1 video in MP4 container. Advanced high-efficiency compression on supported players.", "AV1 в контейнере MP4. Современное эффективное сжатие на поддерживаемых устройствах."),
        tr("AV1 video in WebM container. Modern open web standard.", "AV1 в контейнере WebM. Современный открытый веб-стандарт."),
        tr("VP9 video in WebM container. Native YouTube format with good compression.", "VP9 в контейнере WebM. Родной формат YouTube с хорошим сжатием.")
    };
    int curFmt = 0;
    for (int i = 0; i < (int)fmtVals.size(); i++) {
        if (fmtVals[i] == VIDEO_FORMAT) { curFmt = i; break; }
    }
    int selFmt = arrowSelect(tr("VIDEO FORMAT", "ВИДЕОФОРМАТ"),
        tr("Select container and video codec preference:", "Выберите предпочитаемый контейнер и видеокодек:"),
        fmtOpts, curFmt, fmtHints);
    if (selFmt < 0) return;
    VIDEO_FORMAT = fmtVals[selFmt];

    saveConfig();
    printColor("\n" + tr("[OK] Video settings updated!", "[OK] Настройки видео успешно обновлены!"), GREEN);
    waitForKey();
}

void selectAudioQuality() {
    vector<string> opts = {
        "M4A (AAC)     - " + tr("Universal high quality (Recommended)", "Универсальное высокое качество (Рекомендуется)"),
        "WEBM (Opus)   - " + tr("High-efficiency modern audio", "Сверхэффективный современный звук"),
        "WEBM (Vorbis) - " + tr("Open OGG Vorbis audio", "Открытый звук OGG Vorbis")
    };
    vector<string> vals = {"M4A(AAC)", "WEBM(Opus)", "WEBM(Vorbis)"};
    vector<string> hints = {
        tr("Advanced Audio Coding in M4A. Universal compatibility with all Apple, Windows, and Android players.", "Формат AAC в M4A. Универсальная совместимость со всеми устройствами Apple, Windows и Android."),
        tr("Opus audio in WebM container. Excellent speech and music quality at compact file sizes.", "Звук Opus в контейнере WebM. Превосходное качество речи и музыки при малом размере."),
        tr("Vorbis audio in WebM container. Classic open-source compressed audio.", "Звук Vorbis в контейнере WebM. Классический открытый сжатый звук.")
    };
    int cur = 0;
    for (int i = 0; i < (int)vals.size(); i++) {
        if (vals[i] == AUDIO_FORMAT) { cur = i; break; }
    }
    int sel = arrowSelect(tr("AUDIO QUALITY", "КАЧЕСТВО АУДИО"),
        tr("Select preferred audio format:", "Выберите предпочитаемый аудиоформат:"),
        opts, cur, hints);
    if (sel >= 0) {
        AUDIO_FORMAT = vals[sel];
        saveConfig();
        printColor("\n" + tr("[OK] Audio format updated!", "[OK] Аудиоформат успешно обновлен!"), GREEN);
        waitForKey();
    }
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
    vector<string> opts = {
        tr("Update yt-dlp (yt-dlp.exe)", "Обновить yt-dlp (yt-dlp.exe)"),
        tr("Re-download FFmpeg", "Перескачать FFmpeg"),
        tr("Re-download QuickJS", "Перескачать QuickJS")
    };
    vector<string> hints = {
        tr("Downloads the latest release of yt-dlp.exe from GitHub (~20MB)", "Скачивает последний релиз yt-dlp.exe с GitHub (~20MB)"),
        tr("Downloads and extracts the latest FFmpeg build from GitHub (~160MB)", "Скачивает и распаковывает свежую сборку FFmpeg с GitHub (~160MB)"),
        tr("Downloads and extracts QuickJS JavaScript engine for YouTube signature solving (~1MB)", "Скачивает и распаковывает движок QuickJS для решения алгоритмов YouTube (~1MB)")
    };

    int sel = arrowSelect(tr("COMPONENT UPDATER", "ОБНОВЛЕНИЕ КОМПОНЕНТОВ"),
        tr("Select component to download or update:", "Выберите компонент для загрузки или обновления:"),
        opts, 0, hints);
    if (sel < 0) return;

    if (sel == 0) {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(tr(" Downloading latest yt-dlp (~20MB)...", " Загрузка последней версии yt-dlp (~20MB)..."), CYAN);
        printColor("========================================", CYAN);
        string destExe = CONFIG_PATH + "yt-dlp.exe";
        if (downloadFile("https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe", destExe, "yt-dlp")) {
            YTDLP_FOUND = fileExists(destExe);
            YTDLP_PATH = destExe;
            printColor("\n" + tr("[OK] yt-dlp updated successfully!", "[OK] yt-dlp успешно обновлен!"), GREEN);
        }
        else {
            printColor("\n" + tr("[ERROR] Failed to update yt-dlp!", "[ОШИБКА] Не удалось обновить yt-dlp!"), RED);
        }
        waitForKey();
    }
    else if (sel == 1) {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(tr(" Downloading latest FFmpeg (~160MB)...", " Загрузка последней версии FFmpeg (~160MB)..."), CYAN);
        printColor("========================================", CYAN);
        string zipFile = CONFIG_PATH + "ffmpeg.zip";
        if (downloadFile("https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip", zipFile, "FFmpeg")) {
            printComponentProgress("FFmpeg", 100.0, tr("Extracting components...", "Распаковка компонентов..."));
            if (extractZip(zipFile, CONFIG_PATH)) {
                organizeExtractedTool("ffmpeg.exe", CONFIG_PATH);
                FFMPEG_FOUND = fileExists(CONFIG_PATH + "ffmpeg.exe");
                if (FFMPEG_FOUND) {
                    FFMPEG_PATH = CONFIG_PATH + "ffmpeg.exe";
                    cout << "\n";
                    printColor(tr("[OK] FFmpeg updated successfully!", "[OK] FFmpeg успешно обновлен!"), GREEN);
                }
                else {
                    cout << "\n";
                    printColor(tr("[ERROR] Failed to locate ffmpeg.exe after extraction!", "[ОШИБКА] Не удалось найти ffmpeg.exe после распаковки!"), RED);
                }
            }
            else {
                cout << "\n";
                printColor(tr("[ERROR] Failed to extract FFmpeg archive!", "[ОШИБКА] Не удалось распаковать архив FFmpeg!"), RED);
            }
            std::error_code ec;
            fs::remove(fs::u8path(zipFile), ec);
        }
        else {
            printColor(tr("[ERROR] Failed to download FFmpeg!", "[ОШИБКА] Не удалось скачать FFmpeg!"), RED);
        }
        waitForKey();
    }
    else if (sel == 2) {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(tr(" Downloading QuickJS (~1MB)...", " Загрузка QuickJS (~1MB)..."), CYAN);
        printColor("========================================", CYAN);
        string zipFile = CONFIG_PATH + "quickjs.zip";
        if (downloadFile("https://bellard.org/quickjs/binary_releases/quickjs-win-x86_64-2026-06-04.zip", zipFile, "QuickJS")) {
            printComponentProgress("QuickJS", 100.0, tr("Extracting components...", "Распаковка компонентов..."));
            if (extractZip(zipFile, CONFIG_PATH)) {
                organizeExtractedTool("qjs.exe", CONFIG_PATH);
                QJS_FOUND = fileExists(CONFIG_PATH + "qjs.exe");
                if (QJS_FOUND) {
                    QJS_PATH = CONFIG_PATH + "qjs.exe";
                    cout << "\n";
                    printColor(tr("[OK] QuickJS updated successfully!", "[OK] QuickJS успешно обновлен!"), GREEN);
                }
                else {
                    cout << "\n";
                    printColor(tr("[WARNING] Could not find qjs.exe in archive (QuickJS is optional).", "[ПРЕДУПРЕЖДЕНИЕ] Не удалось найти qjs.exe в архиве (QuickJS необязателен)."), YELLOW);
                }
            }
            else {
                cout << "\n";
                printColor(tr("[WARNING] Failed to extract QuickJS archive.", "[ПРЕДУПРЕЖДЕНИЕ] Не удалось распаковать архив QuickJS."), YELLOW);
            }
            std::error_code ec;
            fs::remove(fs::u8path(zipFile), ec);
        }
        else {
            printColor(tr("[WARNING] Failed to download QuickJS (QuickJS is optional).", "[ПРЕДУПРЕЖДЕНИЕ] Не удалось скачать QuickJS (QuickJS необязателен)."), YELLOW);
        }
        waitForKey();
    }
}

void toggleUseArchive() {
    USE_ARCHIVE = !USE_ARCHIVE;
    saveConfig();
}

void clearArchiveMenu() {
    string arcPath = getArchivePath();
    if (!fileExists(arcPath)) {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(tr(" CLEAR DOWNLOAD ARCHIVE HISTORY", " ОЧИСТКА ИСТОРИИ ЗАГРУЗОК (АРХИВА)"), CYAN);
        printColor("========================================", CYAN);
        printColor("\n" + tr("[INFO] Download archive is already empty.", "[ИНФО] Архив загрузок уже пуст."), YELLOW);
        waitForKey();
        return;
    }
    vector<string> opts = {
        tr("No (Cancel)", "Нет (Отмена)"),
        tr("Yes, clear download history", "Да, очистить историю загрузок")
    };
    int sel = arrowSelect(
        tr("CLEAR DOWNLOAD ARCHIVE HISTORY", "ОЧИСТКА ИСТОРИИ ЗАГРУЗОК (АРХИВА)"),
        tr("Are you sure you want to clear the download history archive?", "Вы уверены, что хотите очистить архив истории загрузок?"),
        opts, 0
    );
    if (sel == 1) {
        std::error_code ec;
        fs::remove(fs::u8path(arcPath), ec);
        printColor("\n" + tr("[OK] Download archive history cleared successfully!", "[OK] История загрузок успешно очищена!"), GREEN);
        waitForKey();
    }
}

// ========== SETTINGS ==========
void settingsMenu() {
    int selected = 0;
    while (true) {
        vector<string> opts = {
            tr("Download location: [", "Папка для скачивания: [") + DOWNLOAD_PATH + "]",
            tr("Video quality: [", "Качество видео: [") + VIDEO_RESOLUTION + "p " + VIDEO_FPS + "fps " + VIDEO_FORMAT + "]",
            tr("Audio quality: [", "Качество аудио: [") + AUDIO_FORMAT + "]",
            tr("Codec recompiler: [", "Перекодировщик кодеков: [") + (CODEC_RECOMPILER ? tr("ON", "ВКЛ") : tr("OFF", "ВЫКЛ")) + "]",
            tr("Skip already downloaded (archive): [", "Пропускать уже скачанное (архив): [") + (USE_ARCHIVE ? tr("ON", "ВКЛ") : tr("OFF", "ВЫКЛ")) + "]",
            tr("Only audio: [", "Только аудио: [") + (ONLY_AUDIO ? tr("ON", "ВКЛ") : tr("OFF", "ВЫКЛ")) + "]",
            tr("Only video: [", "Только видео: [") + (ONLY_VIDEO ? tr("ON", "ВКЛ") : tr("OFF", "ВЫКЛ")) + "]",
            tr("Clear download history (archive)", "Очистить историю загрузок (архив)"),
            tr("Update cookies", "Обновить куки"),
            tr("Update yt-dlp & components", "Обновить yt-dlp и компоненты"),
            tr("Return (ESC)", "Назад (ESC)")
        };
        vector<int> actions = {
            '1', '2', '3', '4', '5', '6', '7', '8', '9', 'u', '0'
        };

        clearScreen();
        printColor("========================================", CYAN);
        printColor(tr(" SETTINGS", " НАСТРОЙКИ"), CYAN);
        printColor("========================================", CYAN);
        cout << "\n";
        for (int i = 0; i < (int)opts.size(); i++) {
            string prefix = "";
            if (actions[i] >= '1' && actions[i] <= '9') {
                prefix = string(1, (char)actions[i]) + ". ";
            } else if (actions[i] == 'u') {
                prefix = "u. ";
            } else if (actions[i] == '0') {
                prefix = "0. ";
            }
            if (i == selected) {
                setColor(GREEN);
                cout << " > " << prefix << opts[i] << endl;
                setColor(WHITE);
            } else {
                cout << "   " << prefix << opts[i] << endl;
            }
        }
        cout << "========================================\n";
        cout << "\n" << tr("Arrow keys to select, Enter to confirm, ESC or 0 to return",
                            "Стрелки для выбора, Enter для подтверждения, ESC или 0 для возврата") << endl;

        char ch = 0;
        wint_t key = _getwch();
        if (key == 27 || key == '0') return;
        if (key == 13) {
            ch = (char)actions[selected];
        } else if (key == 0 || key == 0xE0) {
            wint_t scan = _getwch();
            if (scan == 72) {
                selected = (selected > 0) ? selected - 1 : (int)opts.size() - 1;
            } else if (scan == 80) {
                selected = (selected < (int)opts.size() - 1) ? selected + 1 : 0;
            }
            continue;
        } else {
            char norm = normalizeKeyToEnglish(key);
            for (int i = 0; i < (int)actions.size(); i++) {
                if (actions[i] == norm) {
                    ch = (char)actions[i];
                    selected = i;
                    break;
                }
            }
            if (ch == 0) continue;
        }

        switch (ch) {
        case '1': {
            wstring folderTitle = (CURRENT_LANG == LANG_RU) ? L"Выберите папку для сохранения" : L"Select folder";
            string p = openFolderDialog(folderTitle.c_str());
            if (!p.empty()) {
                DOWNLOAD_PATH = p;
                if (!dirExists(DOWNLOAD_PATH)) createDirRecursive(DOWNLOAD_PATH);
                saveConfig();
                printColor(tr("[OK] Download location updated!", "[OK] Папка загрузки обновлена!"), GREEN);
            }
            else {
                printColor(tr("[INFO] Not changed", "[ИНФО] Не изменено"), YELLOW);
            }
            waitForKey();
            break;
        }
        case '2': selectVideoQuality(); break;
        case '3': selectAudioQuality(); break;
        case '4': codecRecompilerMenu(); break;
        case '5': toggleUseArchive(); break;
        case '6': toggleOnlyAudio(); break;
        case '7': toggleOnlyVideo(); break;
        case '8': clearArchiveMenu(); break;
        case '9': clearScreen(); cookieEditor(true); saveConfig(); break;
        case 'u': case 'U': updateComponentsMenu(); break;
        case '0': return;
        default: break;
        }
    }
}

// ========== PEER FFMPEG SHARING ==========
bool checkAndCopyFromPeerFFmpeg() {
    wchar_t docPath[MAX_PATH];
    string peerConfigs = "";
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, docPath))) {
        wstring p = wstring(docPath) + L"\\MR-CLI-FOR-FFMPEG\\configs\\";
        peerConfigs = wstringToUtf8(p);
    }
    if (peerConfigs.empty() || !fileExists(peerConfigs + "ffmpeg.exe")) {
        string fallback = "C:\\MR-CLI-FOR-FFMPEG\\configs\\";
        if (fileExists(fallback + "ffmpeg.exe")) {
            peerConfigs = fallback;
        }
    }
    if (peerConfigs.empty() || !fileExists(peerConfigs + "ffmpeg.exe")) {
        return false;
    }

    string otherAppName = "MR CLI FOR FFMPEG";
    string title = tr("FOUND FFMPEG", "ОБНАРУЖЕН FFMPEG");
    string desc = tr(
        "The program just detected that you also use " + otherAppName + ".\n"
        "It already has FFmpeg installed.\n"
        "Would you like to use a shared copy to avoid downloading again?",
        "Программа только что обнаружила, что вы также используете " + otherAppName + ".\n"
        "У неё уже установлен FFmpeg.\n"
        "Хотите использовать общую копию, чтобы не скачивать повторно?"
    );
    vector<string> opts = {
        tr("Yes, use copy (fast and offline)", "Да, использовать копию (быстро и без интернета)"),
        tr("No, download anew (download speed depends on your network)", "Нет, скачать заново (скорость загрузки зависит от вашей сети)")
    };

    int sel = arrowSelect(title, desc, opts, 0);
    if (sel != 0) {
        return false;
    }

    if (!dirExists(CONFIG_PATH)) {
        createDirRecursive(CONFIG_PATH);
    }

    clearScreen();
    printColor("========================================", CYAN);
    printColor(" " + tr("COPYING FFMPEG FROM ", "КОПИРОВАНИЕ FFMPEG ИЗ ") + otherAppName, CYAN);
    printColor("========================================", CYAN);
    cout << "\n";
    printColor(tr("[INFO] Copying FFmpeg components...", "[ИНФО] Копирование компонентов FFmpeg..."), CYAN);

    vector<string> filesToCopy = {"ffmpeg.exe", "ffprobe.exe", "ffplay.exe"};
    bool copiedAny = false;
    for (const auto& f : filesToCopy) {
        string src = peerConfigs + f;
        string dst = CONFIG_PATH + f;
        if (fileExists(src)) {
            wstring wSrc = utf8ToWstring(src);
            wstring wDst = utf8ToWstring(dst);
            if (CopyFileW(wSrc.c_str(), wDst.c_str(), FALSE)) {
                copiedAny = true;
                printColor(tr("[OK] Copied: ", "[OK] Скопирован: ") + f, GREEN);
            }
            else {
                printColor(tr("[ERROR] Failed to copy: ", "[ОШИБКА] Не удалось скопировать: ") + f, RED);
            }
        }
    }

    if (copiedAny && fileExists(CONFIG_PATH + "ffmpeg.exe")) {
        FFMPEG_FOUND = true;
        FFMPEG_PATH = CONFIG_PATH + "ffmpeg.exe";
        cout << "\n";
        printColor(tr("[OK] FFmpeg successfully copied!", "[OK] FFmpeg успешно скопирован!"), GREEN);
        Sleep(1500);
        return true;
    }

    return false;
}

// ========== DEPENDENCY CHECKS & AUTO INSTALLER ==========
bool checkDependencies() {
    YTDLP_PATH = CONFIG_PATH + "yt-dlp.exe";
    FFMPEG_PATH = CONFIG_PATH + "ffmpeg.exe";
    QJS_PATH = CONFIG_PATH + "qjs.exe";

    YTDLP_FOUND = fileExists(YTDLP_PATH);
    FFMPEG_FOUND = fileExists(FFMPEG_PATH);
    QJS_FOUND = fileExists(QJS_PATH);

    if (!FFMPEG_FOUND) {
        if (checkAndCopyFromPeerFFmpeg()) {
            FFMPEG_FOUND = fileExists(FFMPEG_PATH);
        }
    }

    if (!YTDLP_FOUND || !FFMPEG_FOUND || !QJS_FOUND) {
        printColor("============================================", RED);
        printColor(tr("[ERROR] Missing recommended components on your computer", "[ОШИБКА] На вашем компьютере отсутствуют рекомендуемые компоненты"), RED);
        printColor("============================================", RED);

        string missing = "";
        if (!YTDLP_FOUND) missing += "yt-dlp ";
        if (!FFMPEG_FOUND) missing += "FFmpeg ";
        if (!QJS_FOUND) missing += "QuickJS ";

        vector<string> instOpts = {
            tr("Install automatically (Recommended)", "Установить автоматически (Рекомендуется)"),
            tr("Skip / Manual installation", "Пропустить / Ручная установка")
        };
        int selInst = arrowSelect(
            tr("AUTO INSTALLER", "АВТОУСТАНОВЩИК"),
            tr("Missing recommended components: ", "Отсутствуют рекомендуемые компоненты: ") + missing + "\n" +
            tr("Would you like to install them automatically?", "Установить их автоматически?"),
            instOpts, 0
        );

        if (selInst != 0) {
            if (!YTDLP_FOUND) {
                clearScreen();
                printColor("============================================", YELLOW);
                printColor(tr(" [ERROR] yt-dlp is required to run the application!", " [ОШИБКА] yt-dlp необходим для работы программы!"), RED);
                printColor(tr(" [INFO] Download from: https://github.com/yt-dlp/yt-dlp/releases", " [ИНФО] Скачайте с: https://github.com/yt-dlp/yt-dlp/releases"), YELLOW);
                printColor(tr(" [INFO] Place 'yt-dlp.exe' in: ", " [ИНФО] Поместите 'yt-dlp.exe' в: ") + CONFIG_PATH, YELLOW);
                printColor("============================================", YELLOW);
                waitForKey();
                return false;
            }
            // If yt-dlp is present but ffmpeg/qjs are skipped, continue with warning
            printColor("\n" + tr("[WARNING] Proceeding without optional components. Some features may be limited.", "[ПРЕДУПРЕЖДЕНИЕ] Продолжение без опциональных компонентов. Некоторые функции могут быть ограничены."), YELLOW);
            Sleep(1500);
            return true;
        }

        // ========== INSTALLATION ==========
        if (!dirExists(CONFIG_PATH)) {
            createDirRecursive(CONFIG_PATH);
        }

        if (!YTDLP_FOUND) {
            printColor("\n" + tr("[INFO] Installing yt-dlp (~20MB)...", "[ИНФО] Установка yt-dlp (~20MB)..."), CYAN);
            if (downloadFile("https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe", CONFIG_PATH + "yt-dlp.exe", "yt-dlp")) {
                YTDLP_FOUND = true;
                printColor(tr("[OK] yt-dlp installed successfully!", "[OK] yt-dlp успешно установлен!"), GREEN);
            }
            else {
                printColor(tr("[ERROR] Failed to install yt-dlp!", "[ОШИБКА] Не удалось установить yt-dlp!"), RED);
            }
        }

        if (!FFMPEG_FOUND) {
            printColor("\n" + tr("[INFO] Installing FFmpeg (~160MB)...", "[ИНФО] Установка FFmpeg (~160MB)..."), CYAN);
            string zipFile = CONFIG_PATH + "ffmpeg.zip";
            if (downloadFile("https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip", zipFile, "FFmpeg")) {
                printComponentProgress("FFmpeg", 100.0, tr("Extracting components...", "Распаковка компонентов..."));
                if (extractZip(zipFile, CONFIG_PATH)) {
                    organizeExtractedTool("ffmpeg.exe", CONFIG_PATH);
                    FFMPEG_FOUND = fileExists(CONFIG_PATH + "ffmpeg.exe");
                    if (FFMPEG_FOUND) {
                        FFMPEG_PATH = CONFIG_PATH + "ffmpeg.exe";
                        cout << "\n";
                        printColor(tr("[OK] FFmpeg and FFprobe installed successfully!", "[OK] FFmpeg и FFprobe успешно установлены!"), GREEN);
                    }
                    else {
                        cout << "\n";
                        printColor(tr("[ERROR] Failed to locate ffmpeg.exe after extraction!", "[ОШИБКА] Не удалось найти ffmpeg.exe после распаковки!"), RED);
                    }
                }
                else {
                    cout << "\n";
                    printColor(tr("[ERROR] Failed to extract FFmpeg archive!", "[ОШИБКА] Не удалось распаковать архив FFmpeg!"), RED);
                }

                std::error_code ec;
                fs::remove(fs::u8path(zipFile), ec);
            }
            else {
                printColor(tr("[ERROR] Failed to download FFmpeg!", "[ОШИБКА] Не удалось скачать FFmpeg!"), RED);
            }
        }

        if (!QJS_FOUND) {
            printColor("\n" + tr("[INFO] Installing QuickJS (~1MB)...", "[ИНФО] Установка QuickJS (~1MB)..."), CYAN);
            string zipFile = CONFIG_PATH + "quickjs.zip";
            if (downloadFile("https://bellard.org/quickjs/binary_releases/quickjs-win-x86_64-2026-06-04.zip", zipFile, "QuickJS")) {
                printComponentProgress("QuickJS", 100.0, tr("Extracting components...", "Распаковка компонентов..."));
                if (extractZip(zipFile, CONFIG_PATH)) {
                    organizeExtractedTool("qjs.exe", CONFIG_PATH);
                    QJS_FOUND = fileExists(CONFIG_PATH + "qjs.exe");
                    if (QJS_FOUND) {
                        QJS_PATH = CONFIG_PATH + "qjs.exe";
                        cout << "\n";
                        printColor(tr("[OK] QuickJS installed successfully!", "[OK] QuickJS успешно установлен!"), GREEN);
                    }
                    else {
                        cout << "\n";
                        printColor(tr("[WARNING] Could not find qjs.exe in archive (QuickJS is optional).", "[ПРЕДУПРЕЖДЕНИЕ] Не удалось найти qjs.exe в архиве (QuickJS необязателен)."), YELLOW);
                    }
                }
                else {
                    cout << "\n";
                    printColor(tr("[WARNING] Failed to extract QuickJS archive.", "[ПРЕДУПРЕЖДЕНИЕ] Не удалось распаковать архив QuickJS."), YELLOW);
                }

                std::error_code ec;
                fs::remove(fs::u8path(zipFile), ec);
            }
            else {
                printColor(tr("[WARNING] Failed to download QuickJS (QuickJS is optional).", "[ПРЕДУПРЕЖДЕНИЕ] Не удалось скачать QuickJS (QuickJS необязателен)."), YELLOW);
            }
        }

        // Final verification
        YTDLP_FOUND = fileExists(YTDLP_PATH);
        FFMPEG_FOUND = fileExists(FFMPEG_PATH);
        QJS_FOUND = fileExists(QJS_PATH);

        if (YTDLP_FOUND) {
            printColor("\n" + tr("[OK] Dependencies check completed!", "[OK] Проверка зависимостей завершена!"), GREEN);
            Sleep(1000);
            return true;
        }
        else {
            printColor("\n" + tr("[ERROR] yt-dlp is missing and could not be installed!", "[ОШИБКА] yt-dlp отсутствует и не может быть установлен!"), RED);
            waitForKey();
            return false;
        }
    }

    return true;
}

// ========== MAIN MENU ==========
char mainMenuSelect() {
    vector<string> opts = {
        "--- " + tr("OPERATIONS", "ОПЕРАЦИИ") + " ---",
        " 1. " + tr("Start download", "Начать скачивание"),
        "--- " + tr("PROGRAM", "ПРОГРАММА") + " ---",
        " s. " + tr("Settings", "Настройки"),
        " l. " + string(CURRENT_LANG == LANG_EN ? "Language: English" : "Язык: Русский"),
        " 0. " + tr("Exit (ESC)", "Выход (ESC)"),
    };
    vector<int> actions = {
        -1,
        '1',
        -1,
        's', 'l', '0'
    };

    int selected = 1;
    while (true) {
        clearScreen();
        printColor("========================================", CYAN);
        printColor(" MR CLI FOR YT DLP v1.1.3", CYAN);
        printColor("========================================", CYAN);
        printColor("========================================", GREEN);
        printColor(" YT-DLP:  " + string(YTDLP_FOUND ? tr("[OK] installed", "[OK] установлен") : tr("[ERROR] not found", "[ОШИБКА] не найден")), YTDLP_FOUND ? GREEN : RED);
        printColor(" FFMPEG:  " + string(FFMPEG_FOUND ? tr("[OK] installed", "[OK] установлен") : tr("[WARNING] not installed", "[ПРЕДУПРЕЖДЕНИЕ] не установлен")), FFMPEG_FOUND ? GREEN : YELLOW);
        printColor(" QuickJS: " + string(QJS_FOUND ? tr("[OK] installed", "[OK] установлен") : tr("[WARNING] not installed", "[ПРЕДУПРЕЖДЕНИЕ] не установлен")), QJS_FOUND ? GREEN : YELLOW);
        printColor("========================================", GREEN);
        cout << "========================================\n";
        for (int i = 0; i < (int)opts.size(); i++) {
            if (actions[i] == -1) {
                cout << opts[i] << "\n";
            } else if (i == selected) {
                setColor(GREEN);
                cout << " > " << opts[i] << endl;
                setColor(WHITE);
            } else {
                cout << "   " << opts[i] << endl;
            }
        }
        cout << "========================================\n";
        cout << "\n" << tr("Arrow keys to select, Enter to confirm, ESC or 0 to exit",
                            "Стрелки для выбора, Enter для подтверждения, ESC или 0 для выхода") << endl;

        wint_t key = _getwch();
        if (key == 27 || key == '0') return '0';
        if (key == 13) return (char)actions[selected];
        if (key == 0 || key == 0xE0) {
            wint_t scan = _getwch();
            if (scan == 72) {
                do { selected = (selected > 0) ? selected - 1 : (int)opts.size() - 1; } while (actions[selected] == -1);
            } else if (scan == 80) {
                do { selected = (selected < (int)opts.size() - 1) ? selected + 1 : 0; } while (actions[selected] == -1);
            }
        } else {
            char ch = normalizeKeyToEnglish(key);
            for (int i = 0; i < (int)opts.size(); i++) {
                if (actions[i] == -1) continue;
                if (opts[i].length() >= 2 && opts[i][0] == ' ' && opts[i][1] == ch) {
                    return (char)actions[i];
                }
                if (opts[i].length() >= 1 && opts[i][0] == ch) {
                    return (char)actions[i];
                }
            }
        }
    }
}

int main() {
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
    initDefaultLanguage();
    loadConfig();

    if (!checkDependencies()) {
        CoUninitialize();
        return 1;
    }

    while (true) {
        char ch = mainMenuSelect();
        if (ch == 27 || ch == '0') {
            cout << tr("Exiting...", "Выход...") << "\n";
            CoUninitialize();
            return 0;
        }
        switch (ch) {
        case '1': startDownload(); break;
        case 's': case '2': settingsMenu(); break;
        case 'l':
            CURRENT_LANG = (CURRENT_LANG == LANG_EN) ? LANG_RU : LANG_EN;
            saveConfig();
            break;
        default:
            printColor(tr("[ERROR] Invalid choice!", "[ОШИБКА] Неверный выбор!"), RED);
            waitForKey();
        }
    }

    CoUninitialize();
    return 0;
}