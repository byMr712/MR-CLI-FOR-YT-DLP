#pragma once
#include "../domain/Types.hpp"
#include <string>
#include <string_view>
#include <unordered_map>

namespace mrcli::services {

enum class StrId {
    AppTitle,
    MenuDownload,
    MenuSettings,
    MenuHelp,
    MenuExit,
    MenuLanguage,
    SettingsTitle,
    DownloadLocation,
    VideoQuality,
    AudioQuality,
    CodecRecompiler,
    SkipArchive,
    OnlyAudio,
    OnlyVideo,
    ConfigureDelay,
    ClearArchive,
    UpdateCookies,
    UpdateComponents,
    ReturnEsc,
    EnterUrl,
    UrlEmpty,
    PlaylistDetected,
    PlaylistChoiceEntire,
    PlaylistChoiceSingle,
    PlaylistStartIndex,
    PlaylistEndIndex,
    StoppedByUser,
    ResumeDownload,
    CancelCurrentVideo,
    CancelAllVideos,
    DelayTitle,
    DelayDesc,
    DelayState,
    DelayInterval,
    DelayDuration,
    Enabled,
    Disabled,
    PressAnyKey
};

class LocalizationService {
public:
    explicit LocalizationService(domain::Language lang = domain::Language::English);

    void setLanguage(domain::Language lang) noexcept { m_currentLang = lang; }
    [[nodiscard]] domain::Language getLanguage() const noexcept { return m_currentLang; }

    [[nodiscard]] std::string get(StrId id) const;
    [[nodiscard]] std::string tr(std::string_view en, std::string_view ru) const;

    static domain::Language detectSystemLanguage() noexcept;

private:
    void initStrings();

    domain::Language m_currentLang{domain::Language::English};
    std::unordered_map<StrId, std::pair<std::string, std::string>> m_strings;
};

} // namespace mrcli::services
