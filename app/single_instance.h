#pragma once

#include <Windows.h>

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace velo::app {

class SingleInstanceManager {
public:
    SingleInstanceManager(std::wstring appId, std::function<void(std::wstring)> onMessage);
    ~SingleInstanceManager();

    bool AcquirePrimary();
    bool SendToPrimary(const std::wstring& payload) const;

private:
    void ServerLoop();
    std::wstring PipeName() const;
    std::wstring MutexName() const;

    std::wstring appId_;
    std::function<void(std::wstring)> onMessage_;
    HANDLE mutex_ = nullptr;
    bool isPrimary_ = false;
    std::atomic<bool> stop_ = false;
    std::thread serverThread_;
};

}  // namespace velo::app