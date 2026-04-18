#include "app/single_instance.h"

#include <vector>

namespace velo::app {
namespace {

std::vector<std::wstring> SplitLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    size_t start = 0;
    while (start < text.size()) {
        const size_t end = text.find(L'\n', start);
        const std::wstring line = text.substr(start, end == std::wstring::npos ? text.size() - start : end - start);
        if (!line.empty() && line != L"__quit__") {
            lines.push_back(line);
        }
        if (end == std::wstring::npos) {
            break;
        }
        start = end + 1;
    }
    return lines;
}

}  // namespace

SingleInstanceManager::SingleInstanceManager(std::wstring appId, std::function<void(std::wstring)> onMessage)
    : appId_(std::move(appId)), onMessage_(std::move(onMessage)) {}

SingleInstanceManager::~SingleInstanceManager() {
    stop_ = true;
    if (isPrimary_) {
        SendToPrimary(L"__quit__\n");
    }
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    if (mutex_ != nullptr) {
        CloseHandle(mutex_);
    }
}

bool SingleInstanceManager::AcquirePrimary() {
    mutex_ = CreateMutexW(nullptr, FALSE, MutexName().c_str());
    if (mutex_ == nullptr) {
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return false;
    }

    isPrimary_ = true;
    serverThread_ = std::thread([this]() { ServerLoop(); });
    return true;
}

bool SingleInstanceManager::SendToPrimary(const std::wstring& payload) const {
    HANDLE pipe = CreateFileW(PipeName().c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const DWORD bytes = static_cast<DWORD>(payload.size() * sizeof(wchar_t));
    const BOOL ok = WriteFile(pipe, payload.data(), bytes, &written, nullptr);
    CloseHandle(pipe);
    return ok == TRUE && written == bytes;
}

void SingleInstanceManager::ServerLoop() {
    while (!stop_) {
        HANDLE pipe = CreateNamedPipeW(PipeName().c_str(), PIPE_ACCESS_INBOUND,
                                       PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 0, 4096, 0,
                                       nullptr);
        if (pipe == INVALID_HANDLE_VALUE) {
            return;
        }

        const BOOL connected = ConnectNamedPipe(pipe, nullptr)
                                   ? TRUE
                                   : (GetLastError() == ERROR_PIPE_CONNECTED ? TRUE : FALSE);
        if (!connected) {
            CloseHandle(pipe);
            continue;
        }

        std::wstring buffer;
        std::wstring chunk(2048, L'\0');
        while (!stop_) {
            DWORD bytesRead = 0;
            const BOOL readOk = ReadFile(pipe, chunk.data(), static_cast<DWORD>(chunk.size() * sizeof(wchar_t)), &bytesRead, nullptr);
            if (bytesRead > 0) {
                buffer.append(chunk.data(), bytesRead / sizeof(wchar_t));
            }
            if (readOk == TRUE) {
                break;
            }
            if (GetLastError() != ERROR_MORE_DATA) {
                buffer.clear();
                break;
            }
        }
        if (!buffer.empty()) {
            for (auto& line : SplitLines(buffer)) {
                if (stop_) {
                    break;
                }
                onMessage_(std::move(line));
            }
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}

std::wstring SingleInstanceManager::PipeName() const {
    return L"\\\\.\\pipe\\" + appId_ + L"Pipe";
}

std::wstring SingleInstanceManager::MutexName() const {
    return L"Local\\" + appId_ + L"Mutex";
}

}  // namespace velo::app
