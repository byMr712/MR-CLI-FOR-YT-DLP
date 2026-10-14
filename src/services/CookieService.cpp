#include "CookieService.hpp"
#include "src/core/WinLibs.hpp"
#include "src/core/FileSystem.hpp"
#include "src/core/StringUtil.hpp"
#include <fstream>
#include <sstream>

namespace mrcli::services {

TempCookieScope::TempCookieScope(std::filesystem::path tempFilePath)
    : m_tempFilePath(std::move(tempFilePath)) {}

TempCookieScope::~TempCookieScope() {
    if (!m_tempFilePath.empty()) {
        core::FileSystem::removeFile(m_tempFilePath);
    }
}

CookieService::CookieService(std::filesystem::path legacyTxtPath, std::filesystem::path encryptedDatPath, std::shared_ptr<ILogger> logger)
    : m_legacyTxtPath(std::move(legacyTxtPath)), m_encryptedDatPath(std::move(encryptedDatPath)), m_logger(std::move(logger)) {}

bool CookieService::hasCookies() const noexcept {
    return core::FileSystem::fileExists(m_encryptedDatPath) || core::FileSystem::fileExists(m_legacyTxtPath);
}

domain::CookieStoreInfo CookieService::getStoreInfo() const noexcept {
    domain::CookieStoreInfo info;
    info.hasEncryptedDat = core::FileSystem::fileExists(m_encryptedDatPath);
    info.hasPlainTxt = core::FileSystem::fileExists(m_legacyTxtPath);
    std::string content = loadDecryptedCookies();
    validateNetscapeFormat(content, &info.validLinesCount);
    return info;
}

std::string CookieService::loadDecryptedCookies() const {
    if (core::FileSystem::fileExists(m_encryptedDatPath)) {
        std::ifstream inFile(m_encryptedDatPath, std::ios::binary);
        if (inFile.is_open()) {
            std::vector<BYTE> cipherData((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();

            if (!cipherData.empty()) {
                DATA_BLOB inBlob;
                inBlob.cbData = static_cast<DWORD>(cipherData.size());
                inBlob.pbData = cipherData.data();

                DATA_BLOB outBlob;
                if (CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob)) {
                    std::string plain(reinterpret_cast<char*>(outBlob.pbData), outBlob.cbData);
                    LocalFree(outBlob.pbData);
                    return plain;
                }
            }
        }
    }

    if (core::FileSystem::fileExists(m_legacyTxtPath)) {
        std::ifstream legacyFile(m_legacyTxtPath, std::ios::binary);
        if (legacyFile.is_open()) {
            std::string content((std::istreambuf_iterator<char>(legacyFile)), std::istreambuf_iterator<char>());
            legacyFile.close();
            return content;
        }
    }

    return {};
}

bool CookieService::saveEncryptedCookies(std::string_view netscapeContent) {
    if (netscapeContent.empty()) return false;

    DATA_BLOB inBlob;
    inBlob.cbData = static_cast<DWORD>(netscapeContent.size());
    inBlob.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(netscapeContent.data()));

    DATA_BLOB outBlob;
    if (!CryptProtectData(&inBlob, L"MR-CLI Cookie Store", nullptr, nullptr, nullptr, 0, &outBlob)) {
        return false;
    }

    core::FileSystem::createDirectories(m_encryptedDatPath.parent_path());
    std::ofstream outFile(m_encryptedDatPath, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        LocalFree(outBlob.pbData);
        return false;
    }

    outFile.write(reinterpret_cast<char*>(outBlob.pbData), outBlob.cbData);
    outFile.close();
    LocalFree(outBlob.pbData);

    return true;
}

bool CookieService::saveCookies(std::string_view netscapeContent) {
    std::string fullCookies = std::string(netscapeContent);
    if (fullCookies.find("# Netscape HTTP Cookie File") == std::string::npos && fullCookies.find("[") != 0) {
        fullCookies = "# Netscape HTTP Cookie File\n# MR CLI FOR YT DLP v1.1.4\n\n" + fullCookies;
    }

    core::FileSystem::createDirectories(m_legacyTxtPath.parent_path());
    std::ofstream txtOut(m_legacyTxtPath, std::ios::binary | std::ios::trunc);
    if (txtOut.is_open()) {
        txtOut.write(fullCookies.data(), fullCookies.size());
        txtOut.close();
    }

    saveEncryptedCookies(fullCookies);
    return core::FileSystem::fileExists(m_legacyTxtPath);
}

bool CookieService::loadFromFile(const std::filesystem::path& sourceFile) {
    return importFromFile(sourceFile);
}

void CookieService::deleteCookies() {
    core::FileSystem::removeFile(m_legacyTxtPath);
    core::FileSystem::removeFile(m_encryptedDatPath);
}

std::unique_ptr<TempCookieScope> CookieService::exportToTempFile() const {
    std::string content = loadDecryptedCookies();
    if (content.empty()) return nullptr;

    wchar_t tempDir[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tempDir)) return nullptr;

    DWORD pid = GetCurrentProcessId();
    std::filesystem::path tempFile = std::filesystem::path(tempDir) / (L"mr_cli_cookie_" + std::to_wstring(pid) + L".txt");

    std::ofstream outFile(tempFile, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) return nullptr;

    outFile.write(content.data(), content.size());
    outFile.close();

    return std::make_unique<TempCookieScope>(tempFile);
}

bool CookieService::isNetscapeLine(std::string_view line) {
    if (line.empty()) return false;
    if (line[0] == '#') {
        if (core::StringUtil::startsWith(line, "#HttpOnly_")) {
            line.remove_prefix(10);
        } else {
            return false;
        }
    }
    int tabCount = 0;
    for (char c : line) {
        if (c == '\t') tabCount++;
    }
    return tabCount >= 5;
}

bool CookieService::validateNetscapeFormat(std::string_view content, size_t* outValidLines) {
    if (content.find("# Netscape HTTP Cookie File") != std::string_view::npos) {
        if (outValidLines) *outValidLines = 1;
        return true;
    }
    if (content.find("\"domain\":") != std::string_view::npos && content.find("\"name\":") != std::string_view::npos) {
        if (outValidLines) *outValidLines = 1;
        return true;
    }

    std::istringstream stream{std::string(content)};
    std::string line;
    size_t validCount = 0;

    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (isNetscapeLine(line)) {
            validCount++;
        }
    }

    if (outValidLines) *outValidLines = validCount;
    return validCount > 0;
}

bool CookieService::importFromFile(const std::filesystem::path& sourceFile) {
    if (!core::FileSystem::fileExists(sourceFile)) return false;

    std::ifstream inFile(sourceFile, std::ios::binary);
    if (!inFile.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    if (!validateNetscapeFormat(content)) return false;
    return saveCookies(content);
}

} // namespace mrcli::services
