#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>

#include <algorithm>
#include <chrono>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "windowsapp.lib")

namespace fs = std::filesystem;
namespace media = winrt::Windows::Media::Control;
using namespace std::chrono_literals;

struct Config {
    std::vector<std::wstring> triggerApps{L"msedge.exe"};
    std::vector<std::wstring> mediaPatterns{L"spotify", L"msedge"};
    DWORD pollMs = 150;
    DWORD resumeDelayMs = 350;
    bool logging = true;
};

static std::wstring lower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return value;
}

static bool ends_with(const std::wstring& value, const std::wstring& suffix) {
    return value.size() >= suffix.size() &&
        value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::vector<std::wstring> split_csv(std::wstring value) {
    std::vector<std::wstring> out;
    size_t pos = 0;
    while (pos <= value.size()) {
        size_t comma = value.find(L',', pos);
        if (comma == std::wstring::npos) comma = value.size();
        auto item = value.substr(pos, comma - pos);
        auto first = item.find_first_not_of(L" \t");
        auto last = item.find_last_not_of(L" \t");
        if (first != std::wstring::npos) out.push_back(lower(item.substr(first, last - first + 1)));
        if (comma == value.size()) break;
        pos = comma + 1;
    }
    return out;
}

static fs::path exe_dir() {
    std::wstring path(32768, L'\0');
    DWORD n = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    path.resize(n);
    return fs::path(path).parent_path();
}

static std::string utf8(std::wstring_view text) {
    if (text.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
        nullptr, 0, nullptr, nullptr);
    std::string out(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
        out.data(), size, nullptr, nullptr);
    return out;
}

static Config load_config(const fs::path& path) {
    Config cfg;
    wchar_t buffer[2048];

    GetPrivateProfileStringW(L"MicMediaAutoPause", L"TriggerApps", L"msedge.exe",
        buffer, static_cast<DWORD>(std::size(buffer)), path.c_str());
    cfg.triggerApps = split_csv(buffer);

    GetPrivateProfileStringW(L"MicMediaAutoPause", L"MediaApps", L"spotify,msedge",
        buffer, static_cast<DWORD>(std::size(buffer)), path.c_str());
    cfg.mediaPatterns = split_csv(buffer);

    cfg.pollMs = std::clamp<DWORD>(
        GetPrivateProfileIntW(L"MicMediaAutoPause", L"PollMs", 150, path.c_str()), 25, 5000);
    cfg.resumeDelayMs = std::clamp<DWORD>(
        GetPrivateProfileIntW(L"MicMediaAutoPause", L"ResumeDelayMs", 350, path.c_str()), 0, 10000);
    cfg.logging = GetPrivateProfileIntW(L"MicMediaAutoPause", L"Logging", 1, path.c_str()) != 0;
    return cfg;
}

class Logger {
public:
    Logger(fs::path path, bool enabled, bool foreground)
        : path_(std::move(path)), enabled_(enabled), foreground_(foreground) {}

    void write(std::wstring_view message) const {
        SYSTEMTIME st{};
        GetLocalTime(&st);
        wchar_t stamp[64];
        swprintf_s(stamp, L"%04u-%02u-%02u %02u:%02u:%02u.%03u",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        std::wstring line = std::wstring(stamp) + L" " + std::wstring(message);
        if (foreground_) std::wcout << line << std::endl;

        if (!enabled_) return;
        std::ofstream file(path_, std::ios::binary | std::ios::app);
        if (file) file << utf8(line) << "\r\n";
    }

private:
    fs::path path_;
    bool enabled_;
    bool foreground_;
};

class MicDetector {
public:
    explicit MicDetector(std::vector<std::wstring> triggerApps)
        : triggerApps_(std::move(triggerApps)) {
        refresh();
    }

    ~MicDetector() {
        close_keys();
        if (root_) RegCloseKey(root_);
    }

    void refresh() {
        close_keys();

        if (!root_) {
            constexpr wchar_t kPath[] =
                L"Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\"
                L"ConsentStore\\microphone\\NonPackaged";
            if (RegOpenKeyExW(HKEY_CURRENT_USER, kPath, 0, KEY_READ, &root_) != ERROR_SUCCESS) {
                return;
            }
        }

        DWORD index = 0;
        for (;;) {
            wchar_t name[16384];
            DWORD nameLen = static_cast<DWORD>(std::size(name));
            LONG rc = RegEnumKeyExW(root_, index++, name, &nameLen, nullptr, nullptr, nullptr, nullptr);
            if (rc == ERROR_NO_MORE_ITEMS) break;
            if (rc != ERROR_SUCCESS) continue;

            std::wstring keyName(name, nameLen);
            auto lowered = lower(keyName);
            bool match = false;
            for (const auto& app : triggerApps_) {
                if (lowered == app || ends_with(lowered, L"#" + app)) {
                    match = true;
                    break;
                }
            }
            if (!match) continue;

            HKEY key{};
            if (RegOpenKeyExW(root_, keyName.c_str(), 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
                keys_.push_back(key);
            }
        }
        lastRefresh_ = std::chrono::steady_clock::now();
    }

    bool active() {
        if (keys_.empty() || std::chrono::steady_clock::now() - lastRefresh_ >= 60s) {
            refresh();
        }

        for (HKEY key : keys_) {
            ULONGLONG start = 0, stop = 0;
            DWORD type = 0;
            DWORD size = sizeof(ULONGLONG);
            if (RegQueryValueExW(key, L"LastUsedTimeStart", nullptr, &type,
                    reinterpret_cast<BYTE*>(&start), &size) != ERROR_SUCCESS) {
                continue;
            }

            size = sizeof(ULONGLONG);
            if (RegQueryValueExW(key, L"LastUsedTimeStop", nullptr, &type,
                    reinterpret_cast<BYTE*>(&stop), &size) != ERROR_SUCCESS) {
                continue;
            }

            if (start > 0 && stop == 0) return true;
        }
        return false;
    }

    size_t matched_key_count() const { return keys_.size(); }

private:
    void close_keys() {
        for (HKEY key : keys_) RegCloseKey(key);
        keys_.clear();
    }

    HKEY root_{};
    std::vector<HKEY> keys_;
    std::vector<std::wstring> triggerApps_;
    std::chrono::steady_clock::time_point lastRefresh_{};
};

struct PausedSession {
    media::GlobalSystemMediaTransportControlsSession session{nullptr};
    std::wstring source;
};

static bool media_matches(std::wstring source, const Config& cfg) {
    source = lower(std::move(source));
    for (const auto& pattern : cfg.mediaPatterns) {
        if (source.find(pattern) != std::wstring::npos) return true;
    }
    return false;
}

static std::wstring playback_name(media::GlobalSystemMediaTransportControlsSessionPlaybackStatus s) {
    using S = media::GlobalSystemMediaTransportControlsSessionPlaybackStatus;
    switch (s) {
        case S::Closed: return L"Closed";
        case S::Opened: return L"Opened";
        case S::Changing: return L"Changing";
        case S::Stopped: return L"Stopped";
        case S::Playing: return L"Playing";
        case S::Paused: return L"Paused";
        default: return L"Unknown";
    }
}

static std::vector<PausedSession> pause_playing(
    const media::GlobalSystemMediaTransportControlsSessionManager& manager,
    const Config& cfg,
    const Logger& log) {

    std::vector<PausedSession> paused;
    for (const auto& session : manager.GetSessions()) {
        try {
            std::wstring source = session.SourceAppUserModelId().c_str();
            if (!media_matches(source, cfg)) continue;
            if (session.GetPlaybackInfo().PlaybackStatus() !=
                media::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) continue;

            if (session.TryPauseAsync().get()) {
                paused.push_back({session, source});
                log.write(L"paused " + source);
            }
        } catch (const winrt::hresult_error& e) {
            log.write(L"pause error: " + std::wstring(e.message().c_str()));
        }
    }
    return paused;
}

static void resume_paused(std::vector<PausedSession>& paused, const Logger& log) {
    for (auto& item : paused) {
        try {
            auto status = item.session.GetPlaybackInfo().PlaybackStatus();
            if (status == media::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused) {
                if (item.session.TryPlayAsync().get()) {
                    log.write(L"resumed " + item.source);
                }
            } else {
                log.write(L"not resuming " + item.source + L" (state changed to " +
                    playback_name(status) + L")");
            }
        } catch (const winrt::hresult_error& e) {
            log.write(L"resume error: " + std::wstring(e.message().c_str()));
        }
    }
    paused.clear();
}

static int run_status(
    MicDetector& detector,
    const media::GlobalSystemMediaTransportControlsSessionManager& manager) {

    std::wcout << L"Microphone active: " << (detector.active() ? L"yes" : L"no") << L"\n";
    std::wcout << L"Matched trigger registry keys: " << detector.matched_key_count() << L"\n";
    std::wcout << L"Media sessions:\n";
    for (const auto& session : manager.GetSessions()) {
        try {
            std::wcout << L"  " << session.SourceAppUserModelId().c_str()
                       << L" - " << playback_name(session.GetPlaybackInfo().PlaybackStatus()) << L"\n";
        } catch (...) {}
    }
    return 0;
}

int wmain(int argc, wchar_t** argv) {
    bool foreground = false;
    bool statusOnly = false;
    bool noLog = false;
    for (int i = 1; i < argc; ++i) {
        std::wstring arg = lower(argv[i]);
        if (arg == L"--foreground") foreground = true;
        else if (arg == L"--status") statusOnly = true;
        else if (arg == L"--no-log") noLog = true;
        else if (arg == L"--version") {
            std::wcout << L"MicMediaAutoPause 1.0.0\n";
            return 0;
        }
    }

    const auto dir = exe_dir();
    const auto cfg = load_config(dir / L"config.ini");
    Logger log(dir / L"MicMediaAutoPause.log", cfg.logging && !noLog, foreground);

    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        MicDetector detector(cfg.triggerApps);
        auto manager = media::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();

        if (statusOnly) return run_status(detector, manager);

        HANDLE mutex = CreateMutexW(nullptr, FALSE, L"Local\\MicMediaAutoPause.Instance");
        if (!mutex) return 2;
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(mutex);
            if (foreground) std::wcout << L"MicMediaAutoPause is already running.\n";
            return 0;
        }

        HANDLE stopEvent = CreateEventW(nullptr, TRUE, FALSE, L"Local\\MicMediaAutoPause.Stop");
        if (!stopEvent) {
            CloseHandle(mutex);
            return 3;
        }
        ResetEvent(stopEvent);

        log.write(L"started");
        bool wasActive = detector.active();
        std::vector<PausedSession> paused;

        if (wasActive) {
            log.write(L"microphone already active");
            paused = pause_playing(manager, cfg, log);
        }

        auto lastRefresh = std::chrono::steady_clock::now();

        for (;;) {
            DWORD wait = WaitForSingleObject(stopEvent, cfg.pollMs);
            if (wait == WAIT_OBJECT_0) break;

            if (std::chrono::steady_clock::now() - lastRefresh >= 60s) {
                detector.refresh();
                lastRefresh = std::chrono::steady_clock::now();
            }

            bool active = detector.active();

            if (active && !wasActive) {
                log.write(L"microphone active");
                paused = pause_playing(manager, cfg, log);
            } else if (!active && wasActive) {
                log.write(L"microphone released");
                if (cfg.resumeDelayMs) Sleep(cfg.resumeDelayMs);

                if (!detector.active()) {
                    resume_paused(paused, log);
                } else {
                    log.write(L"resume canceled; microphone became active again");
                }
            }

            wasActive = active;
        }

        if (!paused.empty()) resume_paused(paused, log);
        log.write(L"stopped");
        CloseHandle(stopEvent);
        CloseHandle(mutex);
        return 0;
    } catch (const winrt::hresult_error& e) {
        log.write(L"fatal: " + std::wstring(e.message().c_str()));
        if (foreground) std::wcerr << L"Fatal: " << e.message().c_str() << L"\n";
        return 10;
    } catch (const std::exception& e) {
        std::wstring msg;
        auto narrow = std::string(e.what());
        msg.assign(narrow.begin(), narrow.end());
        log.write(L"fatal: " + msg);
        return 11;
    }
}
