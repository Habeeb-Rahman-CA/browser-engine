#pragma once

#include "browser/platform/event.hpp"
#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace browser::events {

using ListenerId = uint64_t;
using EventCallback = std::function<void(const platform::Event&)>;

class EventDispatcher {
public:
    EventDispatcher() = default;
    ~EventDispatcher() = default;

    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;
    EventDispatcher(EventDispatcher&&) noexcept = default;
    EventDispatcher& operator=(EventDispatcher&&) noexcept = default;

    // Subscribe to a specific event type
    ListenerId subscribe(platform::EventType type, EventCallback callback);

    // Subscribe to all event types
    ListenerId subscribeAll(EventCallback callback);

    // Unsubscribe by id
    void unsubscribe(ListenerId id);

    // Dispatch an event to all subscribed listeners
    void dispatch(const platform::Event& event);

    // Clear all listeners
    void clear();

    [[nodiscard]] size_t listenerCount() const noexcept;

private:
    struct ListenerEntry {
        ListenerId id;
        EventCallback callback;
    };

    ListenerId m_nextId{1};
    std::unordered_map<platform::EventType, std::vector<ListenerEntry>> m_typedListeners;
    std::vector<ListenerEntry> m_globalListeners;
};

} // namespace browser::events

