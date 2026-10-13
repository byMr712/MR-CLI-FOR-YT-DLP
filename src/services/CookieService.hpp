#pragma once
#include "../domain/CookieData.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <memory>

namespace mrcli::services {

class TempCookieScope {
public:
    explicit TempCookieScope(std::filesystem::path tempFilePath);
    ~TempCookieScope();

    TempCookieScope(const TempCookieScope&) = delete;
    TempCookieScope& operator=(const TempCookieScope&) = delete;

    [[nodiscard]] std::filesystem::path getPath() const noexcept { return m_tempFilePath; }

private:
    std::filesystem::path m_tempFilePath;
};

class CookieService {
public:
    CookieService(std::filesystem::path encryptedDatPath, std::filesystem::path legacyTxtPath);

    [[nodiscard]] bool hasCookies() const noexcept;
    [[nodiscard]] domain::CookieStoreInfo getStoreInfo() const noexcept;

    [[nodiscard]] std::string loadDecryptedCookies() const;
    bool saveEncryptedCookies(std::string_view netscapeContent);

    [[nodiscard]] std::unique_ptr<TempCookieScope> exportToTempFile() const;

    static bool validateNetscapeFormat(std::string_view content, size_t* outValidLines = nullptr);
    static bool isNetscapeLine(std::string_view line);

    bool importFromFile(const std::filesystem::path& sourceFile);

private:
    std::filesystem::path m_encryptedDatPath;
    std::filesystem::path m_legacyTxtPath;
};

} // namespace mrcli::services
