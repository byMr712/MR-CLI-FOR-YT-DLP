#include "Application.hpp"
#include "src/core/Console.hpp"
#include "src/core/FileSystem.hpp"
#include "src/core/Handle.hpp"
#include "src/services/Logger.hpp"
#include "src/ui/Menu.hpp"
#include "src/ui/InputReader.hpp"
#include "src/ui/screens/MainMenuScreen.hpp"
#include <iostream>

namespace mrcli::app {

Application::Application() = default;
Application::~Application() = default;

bool Application::initialize() {
    core::Console::initUtf8();

    m_ctx.paths = std::make_shared<services::PathProvider>();
    m_ctx.logger = std::make_shared<services::ConsoleLogger>();
    m_ctx.loc = std::make_shared<services::LocalizationService>();

    core::FileSystem::createDirectories(m_ctx.paths->getConfigDir());
    core::FileSystem::createDirectories(m_ctx.paths->getDownloadsDir());
    core::FileSystem::createDirectories(m_ctx.paths->getPluginsDir());

    m_ctx.configService = std::make_shared<services::ConfigService>(m_ctx.paths->getConfigFile());
    m_ctx.config = m_ctx.configService->load();

    if (m_ctx.config.downloadPath.empty()) {
        m_ctx.config.downloadPath = m_ctx.paths->getDownloadsDir();
        m_ctx.configService->save(m_ctx.config);
    } else {
        core::FileSystem::createDirectories(m_ctx.config.downloadPath);
    }

    m_ctx.loc->setLanguage(m_ctx.config.language);
    ui::Menu::setLocalization(m_ctx.loc.get());
    if (auto cl = std::dynamic_pointer_cast<services::ConsoleLogger>(m_ctx.logger)) {
        cl->setLocalization(m_ctx.loc.get());
    }

    m_ctx.archiveService = std::make_shared<services::ArchiveService>(m_ctx.paths->getArchiveFile());
    m_ctx.cookieService = std::make_shared<services::CookieService>(
        m_ctx.paths->getCookiesFile(),
        m_ctx.paths->getEncryptedCookiesFile(),
        m_ctx.logger
    );
    m_ctx.dependencyManager = std::make_shared<services::DependencyManager>(m_ctx.paths, m_ctx.logger, m_ctx.loc);
    m_ctx.ytDlpService = std::make_shared<services::YtDlpService>(
        m_ctx.paths,
        m_ctx.archiveService,
        m_ctx.cookieService,
        m_ctx.logger,
        m_ctx.loc
    );

    return true;
}

bool Application::checkDependencies() {
    auto& dm = *m_ctx.dependencyManager;
    dm.refreshStatus();

    auto status = dm.getStatus();
    if (!status.ffmpegInstalled) {
        if (dm.checkAndCopyFromPeerFFmpeg()) {
            dm.refreshStatus();
            status = dm.getStatus();
        }
    }

    if (!status.ytDlpInstalled || !status.ffmpegInstalled || !status.quickJsInstalled) {
        core::Console::print("============================================", core::ConsoleColor::Red);
        core::Console::print(m_ctx.loc->tr("[ERROR] Missing recommended components on your computer",
                                          "[ОШИБКА] На вашем компьютере отсутствуют рекомендуемые компоненты"), core::ConsoleColor::Red);
        core::Console::print("============================================", core::ConsoleColor::Red);

        std::string missing = "";
        if (!status.ytDlpInstalled) missing += "yt-dlp ";
        if (!status.ffmpegInstalled) missing += "FFmpeg ";
        if (!status.quickJsInstalled) missing += "QuickJS ";

        std::vector<std::string> instOpts = {
            m_ctx.loc->tr("Install automatically (Recommended)", "Установить автоматически (Рекомендуется)"),
            m_ctx.loc->tr("Skip / Manual installation", "Пропустить / Ручная установка")
        };
        int selInst = ui::Menu::arrowSelect(
            m_ctx.loc->tr("AUTO INSTALLER", "АВТОУСТАНОВЩИК"),
            m_ctx.loc->tr("Missing recommended components: ", "Отсутствуют рекомендуемые компоненты: ") + missing + "\n" +
            m_ctx.loc->tr("Would you like to install them automatically?", "Установить их автоматически?"),
            instOpts, 0
        );

        if (selInst != 0) {
            if (!status.ytDlpInstalled) {
                core::Console::clear();
                core::Console::print("============================================", core::ConsoleColor::Yellow);
                core::Console::print(m_ctx.loc->tr(" [ERROR] yt-dlp is required to run the application!", " [ОШИБКА] yt-dlp необходим для работы программы!"), core::ConsoleColor::Red);
                core::Console::print(m_ctx.loc->tr(" [INFO] Download from: https://github.com/yt-dlp/yt-dlp/releases", " [ИНФО] Скачайте с: https://github.com/yt-dlp/yt-dlp/releases"), core::ConsoleColor::Yellow);
                core::Console::print(m_ctx.loc->tr(" [INFO] Place 'yt-dlp.exe' in: ", " [ИНФО] Поместите 'yt-dlp.exe' в: ") + m_ctx.paths->getConfigDir(), core::ConsoleColor::Yellow);
                core::Console::print("============================================", core::ConsoleColor::Yellow);
                ui::InputReader::waitForKey();
                return false;
            }
            m_ctx.logger->warn(m_ctx.loc->tr("[WARNING] Proceeding without optional components. Some features may be limited.",
                                             "[ПРЕДУПРЕЖДЕНИЕ] Продолжение без опциональных компонентов. Некоторые функции могут быть ограничены."));
            Sleep(1500);
            return true;
        }

        core::FileSystem::createDirectories(m_ctx.paths->getConfigDir());
        core::FileSystem::createDirectories(m_ctx.paths->getPluginsDir());

        if (!status.ytDlpInstalled) {
            dm.installYtDlp();
        }
        if (!status.ffmpegInstalled) {
            dm.installFFmpeg();
        }
        if (!status.quickJsInstalled) {
            dm.installQuickJs();
        }

        dm.refreshStatus();
        status = dm.getStatus();

        if (status.ytDlpInstalled) {
            m_ctx.logger->info(m_ctx.loc->tr("[OK] Dependencies check completed!", "[OK] Проверка зависимостей завершена!"));
            Sleep(1000);
            return true;
        } else {
            m_ctx.logger->error(m_ctx.loc->tr("[ERROR] yt-dlp is missing and could not be installed!", "[ОШИБКА] yt-dlp отсутствует и не может быть установлен!"));
            ui::InputReader::waitForKey();
            return false;
        }
    }

    return true;
}

int Application::run() {
    core::ScopedCoInit coInit;

    if (!initialize()) {
        return 1;
    }

    if (!checkDependencies()) {
        return 1;
    }

    ui::screens::MainMenuScreen mainScreen(m_ctx);
    mainScreen.show();

    return 0;
}

} // namespace mrcli::app
