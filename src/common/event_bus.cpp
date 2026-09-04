#include "event_bus.h"

void EventBus::Unsubscribe(SubscriptionId id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = subscriptionMap_.find(id);
    if (it == subscriptionMap_.end()) return;

    EventType type = it->second;
    auto handlersIt = handlers_.find(type);
    if (handlersIt != handlers_.end()) {
        auto& list = handlersIt->second;
        list.erase(
            std::remove_if(list.begin(), list.end(),
                [id](const auto& pair) { return pair.first == id; }),
            list.end()
        );
    }
    subscriptionMap_.erase(it);
}

void EventBus::Clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    handlers_.clear();
    subscriptionMap_.clear();
    nextId_ = 1;
}
