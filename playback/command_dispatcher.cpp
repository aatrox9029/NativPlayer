#include "playback/command_dispatcher.h"

#include <chrono>

namespace velo::playback {
namespace {

bool IsSeekCommand(const CommandType type) {
    return type == CommandType::SeekRelative || type == CommandType::SeekAbsolute;
}

}  // namespace

void CommandDispatcher::Push(PlayerCommand command) {
    {
        std::scoped_lock lock(mutex_);
        queue_.push(std::move(command));
    }
    condition_.notify_one();
}

std::optional<PlayerCommand> CommandDispatcher::WaitAndPop(int timeoutMilliseconds) {
    std::unique_lock lock(mutex_);
    condition_.wait_for(lock, std::chrono::milliseconds(timeoutMilliseconds), [this]() { return !queue_.empty(); });
    if (queue_.empty()) {
        return std::nullopt;
    }

    PlayerCommand command = std::move(queue_.front());
    queue_.pop();
    return command;
}

std::optional<PlayerCommand> CommandDispatcher::TryPop() {
    std::scoped_lock lock(mutex_);
    if (queue_.empty()) {
        return std::nullopt;
    }
    PlayerCommand command = std::move(queue_.front());
    queue_.pop();
    return command;
}

PlayerCommand CommandDispatcher::CollapsePendingSeekCommands(PlayerCommand command) {
    std::scoped_lock lock(mutex_);
    while (!queue_.empty() && IsSeekCommand(queue_.front().type)) {
        command = std::move(queue_.front());
        queue_.pop();
    }
    return command;
}

}  // namespace velo::playback
