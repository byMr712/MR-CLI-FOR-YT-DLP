#include <iostream>
#include <string>
#include <cstdlib>
#include <conio.h>
#include <windows.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <direct.h>
#include <sys/stat.h>
#include <algorithm>
#include <shlobj.h>
#include <commdlg.h>
#include <locale>
#include <cctype>

using namespace std;

// ========== CONFIG ==========
enum ErrorType {
    NOT_ERROR = 0,
    COOKIE_ERROR = 1,
    AGE_ERROR = 2,
    LIVE_ERROR = 3,
    MERGE_ERROR = 4,
    VIDEO_ERROR = 5
};

string YTDLP_PATH, FFMPEG_PATH, SCRIPT_DIR, DOWNLOAD_PATH;
string COOKIES_FILE = "cookies.txt";
string VIDEO_RESOLUTION = "1080", VIDEO_FPS = "60", VIDEO_FORMAT = "MP4(H.264)";
string AUDIO_FORMAT = "M4A(AAC)";
bool USE_COOKIES = true, CODEC_RECOMPILER = false, ONLY_AUDIO = false, ONLY_VIDEO = false;
bool YTDLP_FOUND = false, FFMPEG_FOUND = false;
int LAST_ERROR = NOT_ERROR;
string ARCHIVE_PATH = "";
int TOTAL_ITEMS_IN_PLAYLIST = 0;
bool PLAYLIST_END_REACHED = false;
int currentIndex = 0;

// Resume
string RESUME_URL, RESUME_START, RESUME_END;
bool RESUME_IS_PLAYLIST = false;
int RESUME_FAILED_INDEX = -1;

// Механизм очистки предыдущих строк
void clearLastLines(int count) {
    cout << "\033[" << count << "A"; // Подняться на count строк вверх
    for (int i = 0; i < count; i++) {
        cout << "\033[K"; // Очистить строку
        if (i < count - 1) {
            cout << "\033[E"; // Спуститься на одну строку вниз
        }
    }
    cout << "\033[" << count - 1 << "A"; // Вернуться наверх
}

// ========== UTF-8 HELPERS ==========
string wstringToUtf8(const wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, NULL, NULL);
    return result;
}

wstring utf8ToWstring(const string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
    return result;
}

// ========== COLORS ==========
enum Color { BLACK = 0, BLUE = 1, GREEN = 2, RED = 4, YELLOW = 6, WHITE = 7, CYAN = 11 };

void setColor(int c) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c); }
void printColor(const string& t, int c = WHITE, bool nl = true) {
    setColor(c); cout << t; setColor(WHITE); if (nl) cout << endl;
}

// ========== HELPERS ==========
bool inputWithEscape(string& result, const string& prompt) {
    cout << prompt;
    result.clear();

    char ch;
    while (true) {
        ch = _getch();
        if (ch == 27) { // ESC
            result.clear();
            return false;
        }
        if (ch == '\r') { // Enter
            cout << endl;
            return true;
        }
        if (ch == '\b' || ch == 127) { // Backspace
            if (!result.empty()) {
                result.pop_back();
                cout << "\b \b";
            }
            continue;
        }
        if (ch >= 32 && ch <= 126) { // Printable chars
            result += ch;
            cout << ch;
        }
    }
}

bool inputLineWithEscape(string& result, const string& prompt) {
    cout << prompt;
    result.clear();

    char ch;
    while (true) {
        ch = _getch();
        if (ch == 27) { // ESC
            result.clear();
            cout << endl;
            return false;
        }
        if (ch == '\r') { // Enter
            cout << endl;
            return true;
        }
        if (ch == '\b' || ch == 127) { // Backspace
            if (!result.empty()) {
                result.pop_back();
                cout << "\b \b";
            }
            continue;
        }
        if (ch >= 32 && ch <= 126) { // Printable chars
            result += ch;
            cout << ch;
        }
    }
}

void setUTF8() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF8");
    SetConsoleOutputCP(65001);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

void clearScreen() { system("cls"); }
void waitForKey() { cout << "\nPress any key..."; (void)_getch(); while (_kbhit()) (void)_getch();  cout << "\n"; }

char getMenuChoice() {
    while (_kbhit()) {
        (void)_getch();
    }
    char c = _getch();
    if (c == 27) return 27;
    if (c >= 'A' && c <= 'Z') {
        c += 32;
    }
    return c;
}

string getScriptDir() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    wstring p(buf);
    size_t pos = p.find_last_of(L"\\/");
    if (pos != wstring::npos) {
        return wstringToUtf8(p.substr(0, pos + 1));
    }
    return "";
}

bool fileExists(const string& p) {
    wstring wp = utf8ToWstring(p);
    DWORD a = GetFileAttributesW(wp.c_str());
    return (a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY));
}

bool dirExists(const string& p) {
    wstring wp = utf8ToWstring(p);
    DWORD a = GetFileAttributesW(wp.c_str());
    return (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY));
}

bool createDir(const string& p) {
    wstring wp = utf8ToWstring(p);
    return CreateDirectoryW(wp.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

string getEnv(const string& n) {
    wstring wn = utf8ToWstring(n);
    wchar_t* b = nullptr;
    size_t s = 0;
    if (_wdupenv_s(&b, &s, wn.c_str()) != 0 || !b) return "";
    wstring r(b);
    free(b);
    return wstringToUtf8(r);
}

bool inPath(const string& f, string& full) {
    wstring wf = utf8ToWstring(f);
    wstring pathStr = utf8ToWstring(getEnv("PATH"));
    wstringstream ss(pathStr);
    wstring p;
    while (getline(ss, p, L';')) {
        wstring testPath = p + L"\\" + wf;
        DWORD a = GetFileAttributesW(testPath.c_str());
        if (a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY)) {
            full = wstringToUtf8(testPath);
            return true;
        }
    }
    return false;
}

bool parseDownloadLine(const string& line, string& percent, string& speed, string& eta) {
    if (line.find("[download]") != 0) return false;
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

string findFileRecursive(const string& dir, const string& f) {
    wstring wdir = utf8ToWstring(dir);
    wstring wf = utf8ToWstring(f);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((wdir + L"*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return "";
    do {
        if (wstring(fd.cFileName) == L"." || wstring(fd.cFileName) == L"..") continue;
        wstring full = wdir + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            string r = findFileRecursive(wstringToUtf8(full + L"\\"), f);
            if (!r.empty()) { FindClose(h); return r; }
        }
        else if (wf == fd.cFileName) { FindClose(h); return wstringToUtf8(full); }
    } while (FindNextFileW(h, &fd) != 0);
    FindClose(h);
    return "";
}

// ========== DOWNLOAD ARCHIVE ==========
string createTempArchive() {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);

    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &timeinfo);

    string archiveName = "ytdlp_archive_" + string(timestamp) + ".txt";
    string fullPath = string(tempPath) + archiveName;

    ofstream f(fullPath);
    if (f.is_open()) {
        f.close();
        return fullPath;
    }
    return "";
}

void deleteArchive(const string& path) {
    if (!path.empty() && fileExists(path)) {
        remove(path.c_str());
    }
}

void cleanupOldArchives() {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);

    string searchPattern = string(tempPath) + "ytdlp_archive_*.txt";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                string fullPath = string(tempPath) + fd.cFileName;
                FILETIME ft = fd.ftCreationTime;
                SYSTEMTIME st;
                FileTimeToSystemTime(&ft, &st);

                time_t currentTime = time(nullptr);
                struct tm now;
                localtime_s(&now, &currentTime);

                SYSTEMTIME nowSt;
                nowSt.wYear = now.tm_year + 1900;
                nowSt.wMonth = now.tm_mon + 1;
                nowSt.wDay = now.tm_mday;
                nowSt.wHour = now.tm_hour;
                nowSt.wMinute = now.tm_min;
                nowSt.wSecond = now.tm_sec;

                FILETIME nowFt;
                SystemTimeToFileTime(&nowSt, &nowFt);

                ULARGE_INTEGER ftTime, nowTime;
                ftTime.LowPart = ft.dwLowDateTime;
                ftTime.HighPart = ft.dwHighDateTime;
                nowTime.LowPart = nowFt.dwLowDateTime;
                nowTime.HighPart = nowFt.dwHighDateTime;

                // Если файл старше 1 часа (3600 секунд * 10^7 = 36000000000)
                if (nowTime.QuadPart - ftTime.QuadPart > 36000000000) {
                    remove(fullPath.c_str());
                }
            }
        } while (FindNextFileA(hFind, &fd) != 0);
        FindClose(hFind);
    }
}

// ========== CONFIG ==========
void saveConfig() {
    string configPath = SCRIPT_DIR + "mr-config.txt";
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
    string configPath = SCRIPT_DIR + "mr-config.txt";

    if (fileExists(configPath)) {
        ifstream f(configPath);
        if (f.is_open()) {
            string l;
            while (getline(f, l)) {
                if (l.length() >= 3 && (unsigned char)l[0] == 0xEF &&
                    (unsigned char)l[1] == 0xBB && (unsigned char)l[2] == 0xBF) {
                    l = l.substr(3);
                }

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
            if (!dirExists(path)) createDir(path);
            DOWNLOAD_PATH = path;
        }
        else {
            DOWNLOAD_PATH = "C:\\MR-CLI-FOR-YT-DLP\\downloads\\";
            if (!dirExists(DOWNLOAD_PATH)) createDir(DOWNLOAD_PATH);
        }
        saveConfig();
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
        if (wp.back() != L'\\') wp += L'\\';
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
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile = fn;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle = L"Select cookies file";

    if (GetOpenFileNameW(&ofn)) {
        return wstringToUtf8(wstring(fn));
    }
    return "";
}

string getClipboard() {
    if (!OpenClipboard(NULL)) return "";
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (!h) { CloseClipboard(); return ""; }
    wchar_t* t = (wchar_t*)GlobalLock(h);
    if (!t) { CloseClipboard(); return ""; }
    wstring r(t);
    GlobalUnlock(h);
    CloseClipboard();
    return wstringToUtf8(r);
}

// ========== COOKIES ==========
bool validateCookiesContent(const string& c) {
    if (c.find("# Netscape HTTP Cookie File") != string::npos) return true;
    stringstream ss(c); string l;
    while (getline(ss, l)) {
        if (l.empty() || l[0] == '#') continue;
        int tabs = 0; for (char ch : l) if (ch == '\t') tabs++;
        if (tabs >= 5) return true;
    }
    return false;
}

bool validateCookies(const string& p) {
    ifstream f(p); if (!f.is_open()) return false;
    string c((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    f.close(); return validateCookiesContent(c);
}

bool saveCookies(const string& c) {
    string cookiesPath = SCRIPT_DIR + COOKIES_FILE;
    ofstream f(cookiesPath); if (!f.is_open()) return false;
    if (c.find("# Netscape HTTP Cookie File") == string::npos)
        f << "# Netscape HTTP Cookie File\n# MR CLI FOR YT DLP v1.04\n\n";
    f << c; f.close(); return true;
}

// ========== MENUS ==========
string cookieEditor(bool fromSettings = false) {
    while (true) {
        printColor("========================================", CYAN);
        printColor(" COOKIE EDITOR", CYAN);
        printColor("========================================", CYAN);
        cout << "\n1. Select cookies file (.txt)\n2. Paste from clipboard\n0. Exit to main menu\n\nYour choice: ";
        char ch = getMenuChoice();
        if (ch == 27) {
            cout << "ESC" << endl;
            return "exit";
        }
        cout << ch << endl;
        switch (ch) {
        case '1': {
            string p = openFileDialog();
            if (!p.empty() && validateCookies(p)) {
                ifstream f(p); string c((istreambuf_iterator<char>(f)), istreambuf_iterator<char>()); f.close();
                if (saveCookies(c)) { USE_COOKIES = true; saveConfig(); clearLastLines(10); return fromSettings ? "settings" : "continue"; }

            }
            else printColor("[ERROR] Invalid cookies file!", RED);
            waitForKey(); clearLastLines(12); break;
        }
        case '2': {
            string c = getClipboard();
            if (c.empty()) { printColor("[ERROR] Clipboard empty!", RED); waitForKey(); break; }
            if (validateCookiesContent(c) && saveCookies(c)) {
                USE_COOKIES = true; saveConfig(); clearLastLines(10);
                return fromSettings ? "settings" : "continue";
            }
            printColor("[ERROR] Invalid cookies!", RED); waitForKey(); clearLastLines(12); break;
        }
        case '0': {
            cout << "Exit" << endl;
            return "exit";
        }
        default: printColor("[ERROR] Invalid choice!", RED); waitForKey(); clearLastLines(12);
        }
    }
}

// ========== FORMAT BUILDING ==========
string buildFormat() {
    string exactRes, lteRes;
    if (VIDEO_RESOLUTION == "2160") { exactRes = "height=2160";  lteRes = "height<=2160"; }
    else if (VIDEO_RESOLUTION == "1440") { exactRes = "height=1440";  lteRes = "height<=1440"; }
    else if (VIDEO_RESOLUTION == "1080") { exactRes = "height=1080";  lteRes = "height<=1080"; }
    else if (VIDEO_RESOLUTION == "720") { exactRes = "height=720";   lteRes = "height<=720"; }
    else if (VIDEO_RESOLUTION == "480") { exactRes = "height=480";   lteRes = "height<=480"; }
    else if (VIDEO_RESOLUTION == "360") { exactRes = "height=360";   lteRes = "height<=360"; }

    string audioFmt;
    if (AUDIO_FORMAT.find("M4A") != string::npos) audioFmt = "ba[ext=m4a]";
    else if (AUDIO_FORMAT.find("Opus") != string::npos) audioFmt = "ba[ext=webm][acodec^=opus]";
    else if (AUDIO_FORMAT.find("Vorbis") != string::npos) audioFmt = "ba[ext=webm][acodec^=vorbis]";

    string codecFilter;
    string ext;
    if (VIDEO_FORMAT.find("MP4(AV1)") != string::npos) { codecFilter = "[vcodec^=av01]"; ext = "[ext=mp4]"; }
    else if (VIDEO_FORMAT.find("MP4(H.264)") != string::npos) { codecFilter = "[vcodec^=avc1]"; ext = "[ext=mp4]"; }
    else if (VIDEO_FORMAT.find("WEBM(AV1)") != string::npos) { codecFilter = "[vcodec^=av01]"; ext = "[ext=webm]"; }
    else if (VIDEO_FORMAT.find("WEBM(VP9)") != string::npos) { codecFilter = "[vcodec^=vp9]";  ext = "[ext=webm]"; }

    string vExactPref = "bv" + ext + codecFilter + "[" + exactRes + "]";
    string vExactAny = "bv[" + exactRes + "]";
    string vLtePref = "bv" + ext + codecFilter + "[" + lteRes + "]";
    string vLteAny = "bv[" + lteRes + "]";

    if (ONLY_AUDIO) return audioFmt + "/bestaudio";

    if (ONLY_VIDEO)
        return vExactPref + "/" + vExactAny + "/" + vLtePref + "/" + vLteAny + "/bestvideo";

    return vExactPref + "+" + audioFmt + "/" +
        vExactAny + "+" + audioFmt + "/" +
        vLtePref + "+" + audioFmt + "/" +
        vLteAny + "+" + audioFmt + "/best";
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
        "Network is unreachable"
    };
    for (auto p : patterns) {
        if (line.find(p) != string::npos) return true;
    }
    return false;
}

// ========== CHECK INTERNET ==========
bool checkInternet() {
    string cmd = "curl -s -o nul -I --connect-timeout 5 https://youtube.com";
    if (system(cmd.c_str()) == 0) {
        return true;
    }

    if (system("ping -n 1 www.youtube.com > nul 2>&1") == 0) {
        return true;
    }

    return false;
}

// ========== WAIT FOR INTERNET ==========
bool waitForInternetAndRetry(FILE* pipe, const string& bat, const string& cmd) {
    int attemptCount = 1;
    while (!checkInternet()) {
        printColor("\n============================================", RED);
        printColor("[ERROR] Internet connection lost! Waiting 30 seconds... Attempt " + to_string(attemptCount), RED);
        printColor("============================================", RED);
        Sleep(30000);
        attemptCount++;
    }

    printColor("\n============================================", CYAN);
    printColor("[INFO] Connection restored. Continuing download...", CYAN);
    printColor("============================================\n", CYAN);
    _pclose(pipe);
    remove(bat.c_str());
    return true;
}

// ========== DOWNLOAD ==========
bool execWithProgress(const string& cmd) {
    string bat = SCRIPT_DIR + "run.bat";
    ofstream f(bat);
    if (!f.is_open()) return false;
    f << "@echo off\n";
    f << "chcp 65001 > nul\n";
    f << cmd << " 2>&1\n";
    f.close();

    FILE* pipe = _popen(("\"" + bat + "\"").c_str(), "r");
    if (!pipe) { remove(bat.c_str()); return false; }

    string line;
    bool progressActive = false;
    bool isPlaylist = false;  // ИНИЦИАЛИЗИРУЕМ КАК FALSE
    bool mergeFailed = false;
    int curItem = 0, totalItems = 0;
    int c;
    string mergeErrorPath = "";
    string videoFile = "";
    string audioFile = "";
    string lastLine = "";

    while ((c = fgetc(pipe)) != EOF) {
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

                if ((line.find("ERROR") != string::npos || line.find("WARNING") != string::npos) && isNetworkError(line)) {
                    if (!checkInternet()) {
                        LAST_ERROR = VIDEO_ERROR;
                        if (waitForInternetAndRetry(pipe, bat, cmd)) {
                            return execWithProgress(cmd);
                        }
                    }
                }

                if (line.find("[download] Downloading item ") == 0) {
                    int newCurItem, newTotalItems;
                    if (sscanf_s(line.c_str(), "[download] Downloading item %d of %d", &newCurItem, &newTotalItems) == 2) {
                        curItem = newCurItem;
                        totalItems = newTotalItems;
                        TOTAL_ITEMS_IN_PLAYLIST = newTotalItems;
                        isPlaylist = (TOTAL_ITEMS_IN_PLAYLIST > 0);  // ОПРЕДЕЛЯЕМ ЗДЕСЬ
                    }
                }
                else if (line.find("[download] Destination:") == 0) {
                    string path = line.substr(string("[download] Destination:").length());
                    while (!path.empty() && path.front() == ' ') path.erase(path.begin());

                    size_t slash = path.find_last_of("\\/");
                    string name = (slash != string::npos) ? path.substr(slash + 1) : path;
                    size_t dot = name.find_last_of('.');
                    if (dot != string::npos) name = name.substr(0, dot);

                    if (progressActive) { cout << endl; progressActive = false; }

                    string prefix;
                    if (ONLY_AUDIO) {
                        prefix = "[Only audio] ";
                    }
                    else if (ONLY_VIDEO) {
                        prefix = "[Only video] ";
                    }
                    else {
                        prefix = "";
                    }

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
                    printColor("[Merger] Merging full video into \"" + path + "\"", CYAN);
                    cout << endl;
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
                }
                else if (line.find("ERROR") != string::npos || line.find("WARNING") != string::npos) {
                    if (progressActive) { cout << endl; progressActive = false; }

                    if (line.find("Sign in to confirm you're not a bot") != string::npos ||
                        line.find("Login required") != string::npos ||
                        line.find("cookies") != string::npos ||
                        line.find("Cookie") != string::npos ||
                        line.find("invalid cookies") != string::npos ||
                        line.find("oauth") != string::npos ||
                        line.find("no longer valid") != string::npos) {
                        LAST_ERROR = COOKIE_ERROR;
                        _pclose(pipe);
                        remove(bat.c_str());
                        return false;
                    }
                    else if (line.find("Sign in to confirm your age") != string::npos) {
                        LAST_ERROR = AGE_ERROR;
                        printColor("\n[AGE ERROR] " + line, YELLOW);
                        printColor("[AGE ERROR] This video has age restriction.", YELLOW);
                        printColor("[AGE ERROR] Please use account cookies that can access this video.", YELLOW);
                        _pclose(pipe);
                        remove(bat.c_str());
                        return false;
                    }
                    else if (line.find("This live event will begin in a few moments") != string::npos) {
                        if (LAST_ERROR != COOKIE_ERROR && LAST_ERROR != AGE_ERROR) {
                            LAST_ERROR = LIVE_ERROR;
                            if (isPlaylist) {
                                printColor("============================================", YELLOW);
                                printColor("[INFO] Video " + to_string(curItem) + " was skipped!", YELLOW);
                                printColor("[INFO] This video is a live stream that hasn't started yet.", YELLOW);
                                printColor("============================================", YELLOW);
                            }
                        }
                    }
                    else {
                        if (LAST_ERROR == NOT_ERROR) {
                            LAST_ERROR = VIDEO_ERROR;
                        }
                        printColor(line, line.find("ERROR") != string::npos ? RED : YELLOW);
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
                }
                line.clear();
            }
        }
        else {
            line += (char)c;
        }
    }

    if (!line.empty()) {
        if ((line.find("ERROR") != string::npos || line.find("WARNING") != string::npos) && isNetworkError(line)) {
            if (!checkInternet()) {
                LAST_ERROR = VIDEO_ERROR;
                if (waitForInternetAndRetry(pipe, bat, cmd)) {
                    return execWithProgress(cmd);
                }
            }
        }
    }

    if (progressActive) cout << endl;

    int r = _pclose(pipe);
    remove(bat.c_str());

    if (mergeFailed && !mergeErrorPath.empty()) {
        printColor("\n[INFO] Merge failed. Trying to merge manually...", YELLOW);

        string dir = mergeErrorPath.substr(0, mergeErrorPath.find_last_of("\\/") + 1);
        string baseName = mergeErrorPath.substr(mergeErrorPath.find_last_of("\\/") + 1);
        size_t dot = baseName.find_last_of('.');
        if (dot != string::npos) baseName = baseName.substr(0, dot);

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
            int maxRetries = 3;
            bool mergeSuccess = false;

            for (int attempt = 1; attempt <= maxRetries; attempt++) {
                string outputFile = dir + baseName + "_merged.mp4";
                string mergeCmd = "\"" + FFMPEG_PATH + "\" -i \"" + videoFile + "\" -i \"" + audioFile + "\" -c:v copy -c:a aac \"" + outputFile + "\"";

                printColor("[INFO] Merge attempt " + to_string(attempt) + " of " + to_string(maxRetries), YELLOW);
                printColor("[INFO] Running: " + mergeCmd, CYAN);
                int mergeResult = system(mergeCmd.c_str());

                if (mergeResult == 0) {
                    printColor("[OK] Manual merge successful on attempt " + to_string(attempt) + "!", GREEN);
                    remove(videoFile.c_str());
                    remove(audioFile.c_str());
                    string finalFile = dir + baseName + ".mp4";
                    if (rename(outputFile.c_str(), finalFile.c_str()) != 0) {
                        printColor("[WARNING] Could not rename merged file, but it exists as: " + outputFile, YELLOW);
                    }
                    else {
                        printColor("[OK] Final file: " + finalFile, GREEN);
                    }
                    mergeSuccess = true;
                    break;
                }
                else {
                    printColor("[ERROR] Merge attempt " + to_string(attempt) + " failed!", RED);
                    if (attempt < maxRetries) {
                        printColor("[INFO] Waiting 2 seconds before retry...", YELLOW);
                        Sleep(2000);
                    }
                }
            }

            if (!mergeSuccess) {
                LAST_ERROR = MERGE_ERROR;
                printColor("[ERROR] All " + to_string(maxRetries) + " merge attempts failed!", RED);
                return false;
            }
            return true;
        }
        else {
            printColor("[ERROR] Could not find video/audio files for manual merge.", RED);
            LAST_ERROR = MERGE_ERROR;
            return false;
        }
    }

    return r == 0;
}
string buildCommand(const string& url, const string& start, const string& end, bool isPlaylist) {
    string cmd = "yt-dlp";

    string cookiesPath = SCRIPT_DIR + COOKIES_FILE;
    if (USE_COOKIES && fileExists(cookiesPath))
        cmd += " --cookies \"" + cookiesPath + "\"";

    if (!ARCHIVE_PATH.empty()) {
        cmd += " --download-archive \"" + ARCHIVE_PATH + "\"";
    }

    string fmt = buildFormat();
    cmd += " -f \"" + fmt + "\"";

    string ext = "mp4";
    string tag = "";

    if (ONLY_VIDEO) {
        if (VIDEO_FORMAT.find("WEBM") != string::npos) ext = "webm";
        else ext = "mp4";
        tag = " [only video]";
        cmd += " --no-audio";
    }
    else if (ONLY_AUDIO) {
        if (AUDIO_FORMAT.find("M4A") != string::npos) ext = "m4a";
        else if (AUDIO_FORMAT.find("WEBM") != string::npos) ext = "webm";
        else ext = "mp4";
        tag = " [only audio]";
    }
    else {
        ext = "mp4";
        cmd += " --merge-output-format mp4";
    }

    if (isPlaylist) {
        if (!start.empty()) cmd += " --playlist-start " + start;
        if (!end.empty()) cmd += " --playlist-end " + end;
    }

    cmd += " --encoding utf-8";

    string out = isPlaylist ?
        DOWNLOAD_PATH + "[Playlist] %%(playlist_title)s/%%(playlist_index)03d - [%%(channel)s] %%(title)s" + tag :
        DOWNLOAD_PATH + "%%(title)s" + tag;

    out += "." + ext;
    cmd += " -o \"" + out + "\"";

    string escapedUrl = url;
    size_t pos = 0;
    while ((pos = escapedUrl.find("\"", pos)) != string::npos) {
        escapedUrl.replace(pos, 1, "\\\"");
        pos += 2;
    }
    cmd += " \"" + escapedUrl + "\"";

    return cmd;
}

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
        cout << "\nEnter URL (ESC to cancel): \n> ";

        cin.clear();
        fflush(stdin);

        if (!inputLineWithEscape(u, "")) {
            printColor("\n[INFO] Cancelled", YELLOW);
            waitForKey();
            return;
        }

        if (u.empty()) {
            cin.clear();
            fflush(stdin);
            if (!inputLineWithEscape(u, "> ")) {
                printColor("\n[INFO] Cancelled", YELLOW);
                waitForKey();
                return;
            }
        }

        if (u.empty()) {
            printColor("[ERROR] URL empty!", RED);
            waitForKey();
            return;
        }

        if (u.find("&list=") != string::npos && u.find("/playlist?list=") == string::npos) {
            size_t pos = u.find("&list=");
            if (pos != string::npos) {
                u = u.substr(0, pos);
            }
        }

        isPl = u.find("/playlist?list=") != string::npos;

        if (isPl) {
            string inp;
            cout << "\nStart number video in playlist (Enter=first, ESC to skip):\nYour choice: ";
            if (!inputLineWithEscape(inp, "")) {
                cout << "[INFO] Skipped\n";
            }
            else if (!inp.empty()) {
                s = inp;
            }

            cout << "\nEnd number video in playlist (Enter=last, ESC to skip):\nYour choice: ";
            if (!inputLineWithEscape(inp, "")) {
                cout << "[INFO] Skipped\n";
            }
            else if (!inp.empty()) {
                e = inp;
            }
        }

        cookieErrorHandled = false;
        skippedVideos.clear();
        TOTAL_ITEMS_IN_PLAYLIST = 0;  // Сбрасываем общее количество
        PLAYLIST_END_REACHED = false;
    }

    if (cookieErrorHandled) {
        return;
    }

    LAST_ERROR = NOT_ERROR;

    string cmd = buildCommand(u, s, e, isPl);

    if (!retry) {
        clearScreen();
    }

    printColor("============================================", CYAN);
    printColor(retry ? " RETRYING DOWNLOAD..." : " STARTING DOWNLOAD...", CYAN);
    printColor("============================================", CYAN);
    cout << endl;

    bool ok = execWithProgress(cmd);

    if (LAST_ERROR == COOKIE_ERROR || LAST_ERROR == AGE_ERROR) {
        cookieErrorHandled = true;

        int currentIndex = 1;
        if (!s.empty()) {
            currentIndex = stoi(s);
        }

        printColor("============================================", RED);
        if (LAST_ERROR == COOKIE_ERROR) {
            printColor("[ERROR] Cookies are invalid or expired!", RED);
            printColor("[ERROR] Sign in to confirm you're not a bot.", RED);
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
        if (ch == 27) {
            cout << "ESC" << endl;
            LAST_ERROR = NOT_ERROR;
            cookieErrorHandled = false;
            return;
        }
        cout << ch << endl;
        if (ch == '1') {
            clearLastLines(3);
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
            currentIndex = s.empty() ? 1 : stoi(s);
            int nextIndex = currentIndex + 1;
            string nextStart = to_string(nextIndex);

            // Проверяем, не достигли ли мы конца плейлиста
            bool isLastVideo = false;

            // Если есть e (end index) - проверяем его
            if (!e.empty()) {
                int endIndex = stoi(e);
                if (currentIndex >= endIndex) {
                    isLastVideo = true;
                }
            }
            else if (TOTAL_ITEMS_IN_PLAYLIST > 0) {
                // Если e не задан, проверяем по общему количеству
                if (currentIndex >= TOTAL_ITEMS_IN_PLAYLIST) {
                    isLastVideo = true;
                }
            }

            if (isLastVideo) {
                printColor("============================================", GREEN);
                printColor("[OK] Download completed!", GREEN);
                printColor("============================================", GREEN);
                if (!skippedVideos.empty()) {
                    printColor("\n============================================", YELLOW);
                    printColor("[INFO] Skipped videos in this session:", YELLOW);
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
            printColor("[ERROR] Video " + to_string(currentIndex) + " was skipped!", RED);
            printColor("[ERROR] This is a live stream that hasn't started yet.", RED);
            printColor("============================================", RED);
            printColor("\n[INFO] Continuing with next video...", CYAN);
            printColor("\n[INFO] Continuing in 5 seconds...\n", CYAN);
            Sleep(5000);

            LAST_ERROR = NOT_ERROR;
            startDownload(u, nextStart, e, true, true);
            return;
        }
        else {
            // Для одиночного видео - показываем сообщение об ошибке
            printColor("============================================", RED);
            printColor("[ERROR] This video is a live stream that hasn't started yet.", RED);
            printColor("============================================", RED);
            waitForKey();
            LAST_ERROR = NOT_ERROR;
            cookieErrorHandled = false;
            return;
        }
    }

    // ========== ОБРАБОТКА ПЛЕЙЛИСТА ==========
    if (!ok && isPl) {
        int currentIndex = s.empty() ? 1 : stoi(s);
        int nextIndex = currentIndex + 1;
        string nextStart = to_string(nextIndex);

        // Проверяем, не достигли ли мы конца плейлиста
        bool isLastVideo = false;

        if (!e.empty()) {
            int endIndex = stoi(e);
            if (currentIndex >= endIndex) {
                isLastVideo = true;
            }
        }
        else if (TOTAL_ITEMS_IN_PLAYLIST > 0) {
            if (currentIndex >= TOTAL_ITEMS_IN_PLAYLIST) {
                isLastVideo = true;
            }
        }

        if (isLastVideo) {
            printColor("\n[INFO] Reached end of playlist range.", YELLOW);
            printColor("[OK] Download partially completed!", GREEN);
            if (!skippedVideos.empty()) {
                printColor("\n============================================", YELLOW);
                printColor("[INFO] Skipped videos in this session:", YELLOW);
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
            string skippedVideo = "Video " + to_string(currentIndex);
            skippedVideos.push_back(skippedVideo);
        }

        printColor("\n============================================", RED);
        printColor("[ERROR] Video " + to_string(currentIndex) + " was skipped!", RED);
        printColor("[ERROR] Possible reasons:", RED);
        printColor("  - Video is unavailable or deleted", RED);
        printColor("  - Video is private", RED);
        printColor("  - Network connection issues", RED);
        printColor("  - YouTube API changes", RED);
        printColor("============================================", RED);
        printColor("\n[INFO] Continuing with next video...", CYAN);
        printColor("\n[INFO] Continuing in 5 seconds...\n", CYAN);
        Sleep(5000);
        LAST_ERROR = NOT_ERROR;
        startDownload(u, nextStart, e, true, true);
        return;
    }

    // ========== ОДИНОЧНОЕ ВИДЕО - ДРУГИЕ ОШИБКИ (НЕ ИНТЕРНЕТ) ==========
    if (!ok) {
        printColor("\n============================================", RED);
        printColor("[ERROR] Download failed!", RED);
        printColor("============================================", RED);
        printColor("\nPossible reasons:", RED);
        printColor("  - Expired/invalid cookies", RED);
        printColor("  - Network issues", RED);
        printColor("  - YouTube API changes", RED);
        printColor("  - Video unavailable", RED);
        cout << "\n1 - Update cookies\n0 - Main menu (ESC)\nYour choice: ";
        char ch = getMenuChoice();
        if (ch == 27) {
            cout << "ESC" << endl;
            LAST_ERROR = NOT_ERROR;
            cookieErrorHandled = false;
            return;
        }
        cout << ch << endl;
        if (ch == '1' && cookieEditor(false) == "continue") {
            cout << "\n";
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
        printColor("[OK] Download completed!", GREEN);
        printColor("============================================", GREEN);
        waitForKey();
    }
}

// ========== SETTINGS MENUS ==========
void codecRecompilerMenu() {
    clearScreen();
    printColor("========================================", CYAN);
    printColor(" Coming soon...", CYAN);
    printColor("========================================", CYAN);
    cout << "\nThis feature is under development.\n";
    cout << "It will allow you to re-encode videos to different codecs.\n";
    waitForKey();
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
    default: printColor("[ERROR] Invalid!", RED); waitForKey(); return;
    }

    clearScreen();
    printColor("========================================", CYAN);
    printColor(" Select frame rate", CYAN);
    printColor("========================================", CYAN);
    cout << "\n1) 60fps\n2) 30fps\n\nYour choice: ";
    ch = getMenuChoice(); cout << ch << endl;
    switch (ch) {
    case '1': VIDEO_FPS = "60"; break;
    case '2': VIDEO_FPS = "30"; break;
    default: printColor("[ERROR] Invalid!", RED); waitForKey(); return;
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
    default: printColor("[ERROR] Invalid!", RED); waitForKey(); return;
    }
    saveConfig();
    printColor("[OK] Updated!", GREEN);
    waitForKey();
}

void selectAudioQuality() {
    clearScreen();
    printColor("========================================", CYAN);
    printColor(" Select audio format", CYAN);
    printColor("========================================", CYAN);
    cout << "\n1) M4A(AAC)\n2) WEBM(Opus)\n3) WEBM(Vorbis)\n\nYour choice: ";
    char ch = getMenuChoice(); cout << ch << endl;
    switch (ch) { case '1': AUDIO_FORMAT = "M4A(AAC)"; break; case '2': AUDIO_FORMAT = "WEBM(Opus)"; break; case '3': AUDIO_FORMAT = "WEBM(Vorbis)"; break; default: printColor("[ERROR] Invalid!", RED); waitForKey(); return; }
                          saveConfig(); printColor("[OK] Updated!", GREEN); waitForKey();
}

void toggleOnlyAudio() { ONLY_AUDIO = !ONLY_AUDIO; if (ONLY_AUDIO) ONLY_VIDEO = false; saveConfig(); }
void toggleOnlyVideo() { ONLY_VIDEO = !ONLY_VIDEO; if (ONLY_VIDEO) ONLY_AUDIO = false; saveConfig(); }

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
            << "\n0. Return (ESC)\n\nYour choice: ";
        char ch = getMenuChoice();
        if (ch == 27) {
            cout << "ESC" << endl;
            return;
        }
        cout << ch << endl;
        switch (ch) {
        case '1': { string p = openFolderDialog(); if (!p.empty()) { DOWNLOAD_PATH = p; saveConfig(); printColor("[OK] Updated!", GREEN); } else printColor("[INFO] Not changed", YELLOW); waitForKey(); break; }
        case '2': selectVideoQuality(); break;
        case '3': selectAudioQuality(); break;
        case '4': codecRecompilerMenu(); break;
        case '5': toggleOnlyAudio(); break;
        case '6': toggleOnlyVideo(); break;
        case '7': clearScreen(); cookieEditor(true); saveConfig(); break;
        case '0': return;
        default: printColor("[ERROR] Invalid!", RED); waitForKey();
        }
    }
}

// ========== CHECKS ==========
bool checkYTDLP() {
    if (fileExists(SCRIPT_DIR + "yt-dlp.exe")) { YTDLP_PATH = SCRIPT_DIR + "yt-dlp.exe"; YTDLP_FOUND = true; return true; }
    if (inPath("yt-dlp.exe", YTDLP_PATH)) { YTDLP_FOUND = true; return true; }
    if (fileExists("C:\\Program Files\\YtDLP\\yt-dlp.exe")) { YTDLP_PATH = "C:\\Program Files\\YtDLP\\yt-dlp.exe"; YTDLP_FOUND = true; return true; }

    string dir = SCRIPT_DIR;
    string cmd = "powershell -Command \"& {$url='https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe';$out='" + dir + "yt-dlp.exe';try{Invoke-WebRequest -Uri $url -OutFile $out -UserAgent 'Mozilla/5.0'}catch{exit 1}}\"";
    if (system(cmd.c_str()) != 0) { YTDLP_FOUND = false; return false; }
    YTDLP_PATH = dir + "yt-dlp.exe"; YTDLP_FOUND = true; return true;
}

bool checkFFMPEG() {
    if (fileExists(SCRIPT_DIR + "ffmpeg.exe")) { FFMPEG_PATH = SCRIPT_DIR + "ffmpeg.exe"; FFMPEG_FOUND = true; return true; }
    if (inPath("ffmpeg.exe", FFMPEG_PATH)) { FFMPEG_FOUND = true; return true; }

    cout << "\n[WARNING] FFMPEG not found! Install? (y/n): ";
    if (getMenuChoice() != 'y') { FFMPEG_FOUND = false; cout << "[WARNING] Skipped\n"; return false; }

    string dir = SCRIPT_DIR;
    string cmd = "powershell -Command \"& {$url='https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip';$zip='" + dir + "ffmpeg.zip';try{Invoke-WebRequest -Uri $url -OutFile $zip -UserAgent 'Mozilla/5.0';Add-Type -AssemblyName System.IO.Compression.FileSystem;[System.IO.Compression.ZipFile]::ExtractToDirectory($zip,'" + dir + "');Remove-Item $zip;Write-Host 'OK'}catch{exit 1}}\"";
    if (system(cmd.c_str()) != 0) { FFMPEG_FOUND = false; return false; }
    string p = findFileRecursive(dir, "ffmpeg.exe");
    if (!p.empty()) { FFMPEG_PATH = p; FFMPEG_FOUND = true; cout << "[OK] Installed!\n"; return true; }
    FFMPEG_FOUND = false; return false;
}

// ========== MAIN ==========
void displayMenu() {
    clearScreen();
    printColor("========================================", CYAN);
    printColor(" MR CLI FOR YT DLP v1.04", CYAN);
    printColor("========================================", CYAN);
    printColor("========================================", GREEN);
    printColor(" YT-DLP: " + string(YTDLP_FOUND ? "[OK] installed" : "[ERROR] not found"), GREEN);
    printColor(" FFMPEG: " + string(FFMPEG_FOUND ? "[OK] installed" : "[WARNING] not installed"), GREEN);
    printColor("========================================", GREEN);
    cout << "========================================\n1. Start download\n2. Settings\n0. Exit\n========================================\n\nYour number choice: ";
}

int main() {
    setUTF8();
    SCRIPT_DIR = getScriptDir();

    loadConfig();

    cleanupOldArchives();

    if (!checkYTDLP()) {
        cout << "\n[ERROR] YT-DLP installation failed!\n"; waitForKey(); return 1;
    }
    checkFFMPEG();

    while (true) {
        displayMenu();
        char ch = getMenuChoice(); cout << ch << "\n\n";
        switch (ch) {
        case '1': startDownload(); break;
        case '2': settingsMenu(); break;
        case '0': cout << "Exiting...\n"; return 0;
        default: printColor("[ERROR] Invalid!", RED); waitForKey();
        }
    }
    return 0;
}