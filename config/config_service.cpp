#include "config/config_service_internal.h"

namespace velo::config {

ConfigService::ConfigService(std::filesystem::path rootDirectory)
    : rootDirectory_(std::move(rootDirectory)),
      configPath_(rootDirectory_ / kConfigFileName),
      backupPath_(rootDirectory_ / kBackupFileName),
      current_(DefaultAppConfig()) {
    worker_ = std::thread([this]() { WorkerLoop(); });
}

ConfigService::~ConfigService() {
    {
        std::scoped_lock lock(mutex_);
        stopWorker_ = true;
    }
    if (worker_.joinable()) {
        worker_.join();
    }
    Flush();
}

bool ConfigService::Load() {
    EnsureRootDirectory();
    AppConfig loaded = DefaultAppConfig();
    bool loadedFromPrimary = false;

    if (std::filesystem::exists(configPath_)) {
        loadedFromPrimary = ParseConfigFile(configPath_, loaded);
        if (!loadedFromPrimary) {
            std::error_code error;
            std::filesystem::copy_file(configPath_, CorruptConfigCopyPath(configPath_), std::filesystem::copy_options::overwrite_existing,
                                       error);
        }
    }

    if (!loadedFromPrimary && std::filesystem::exists(backupPath_) && ParseConfigFile(backupPath_, loaded)) {
        WriteConfigFile(loaded);
    } else if (!loadedFromPrimary && !std::filesystem::exists(configPath_)) {
        return false;
    } else if (!loadedFromPrimary) {
        loaded = DefaultAppConfig();
        WriteConfigFile(loaded);
    }

    std::scoped_lock lock(mutex_);
    current_ = loaded;
    return loadedFromPrimary;
}

void ConfigService::ScheduleSave(const AppConfig& config) {
    std::scoped_lock lock(mutex_);
    current_ = config;
    pending_ = config;
}

void ConfigService::Flush() {
    std::optional<AppConfig> snapshot;
    {
        std::scoped_lock lock(mutex_);
        snapshot = pending_.has_value() ? pending_ : std::optional<AppConfig>(current_);
        pending_.reset();
    }
    if (snapshot.has_value()) {
        WriteConfigFile(*snapshot);
    }
}

const AppConfig& ConfigService::Current() const noexcept {
    return current_;
}

const std::filesystem::path& ConfigService::ConfigPath() const noexcept {
    return configPath_;
}

const std::filesystem::path& ConfigService::BackupPath() const noexcept {
    return backupPath_;
}

void ConfigService::WorkerLoop() {
    using namespace std::chrono_literals;
    while (true) {
        bool shouldStop = false;
        {
            std::scoped_lock lock(mutex_);
            shouldStop = stopWorker_;
        }
        if (shouldStop) {
            return;
        }
        std::this_thread::sleep_for(1500ms);

        std::optional<AppConfig> snapshot;
        {
            std::scoped_lock lock(mutex_);
            if (pending_.has_value()) {
                snapshot = pending_;
                pending_.reset();
            }
        }
        if (snapshot.has_value()) {
            WriteConfigFile(*snapshot);
        }
    }
}

bool ConfigService::WriteConfigFile(const AppConfig& config) {
    EnsureRootDirectory();
    if (!WriteConfigFileToPath(config, configPath_)) {
        return false;
    }
    WriteConfigFileToPath(config, backupPath_);
    return true;
}

void ConfigService::EnsureRootDirectory() {
    std::error_code error;
    std::filesystem::create_directories(rootDirectory_, error);
}

bool ExportConfigSnapshot(const AppConfig& config, const std::filesystem::path& destinationPath) {
    std::error_code error;
    std::filesystem::create_directories(destinationPath.parent_path(), error);
    AppConfig normalized = config;
    NormalizeConfig(normalized);
    return WriteConfigFileToPath(normalized, destinationPath);
}

bool ImportConfigSnapshot(const std::filesystem::path& sourcePath, AppConfig& loadedConfig) {
    return ParseConfigFile(sourcePath, loadedConfig);
}

}  // namespace velo::config
