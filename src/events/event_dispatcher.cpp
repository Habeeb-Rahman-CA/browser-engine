#include "browser/events/event_dispatcher.hpp"
#include <algorithm>

namespace browser::events {

ListenerId EventDispatcher::subscribe(platform::EventType type, EventCallback callback) {
    ListenerId id = m_nextId++;
    m_typedListeners[type].push_back(ListenerEntry{id, std::move(callback)});
    return id;
}

ListenerId EventDispatcher::subscribeAll(EventCallback callback) {
    ListenerId id = m_nextId++;
    m_globalListeners.push_back(ListenerEntry{id, std::move(callback)});
    return id;
}

void EventDispatcher::unsubscribe(ListenerId id) {
    for (auto& [type, listeners] : m_typedListeners) {
        auto it = std::remove_if(listeners.begin(), listeners.end(),
            [id](const ListenerEntry& entry) { return entry.id == id; });
        listeners.erase(it, listeners.end());
    }

    auto it = std::remove_if(m_globalListeners.begin(), m_globalListeners.end(),
        [id](const ListenerEntry& entry) { return entry.id == id; });
    m_globalListeners.erase(it, m_globalListeners.end());
}

void EventDispatcher::dispatch(const platform::Event& event) {
    // Notify type-specific listeners first
    auto it = m_typedListeners.find(event.type);
    if (it != m_typedListeners.end()) {
        // Copy list to allow subscription/unsubscription inside callbacks safely
        auto listenersCopy = it->second;
        for (const auto& entry : listenersCopy) {
            entry.callback(event);
        }
    }

    // Notify global listeners
    auto globalCopy = m_globalListeners;
    for (const auto& entry : globalCopy) {
        entry.callback(event);
    }
}

void EventDispatcher::clear() {
    m_typedListeners.clear();
    m_globalListeners.clear();
}

size_t EventDispatcher::listenerCount() const noexcept {
    size_t count = m_globalListeners.size();
    for (const auto& [type, listeners] : m_typedListeners) {
        count += listeners.size();
    }
    return count;
}

} // namespace browser::events

