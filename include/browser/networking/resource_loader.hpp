#pragma once

#include "browser/networking/resource.hpp"
#include <unordered_map>
#include <memory>
#include <string>

namespace browser::networking {

class ResourceLoader {
public:
    ResourceLoader();
    ~ResourceLoader();

    ResourceLoader(const ResourceLoader&) = delete;
    ResourceLoader& operator=(const ResourceLoader&) = delete;

    // Synchronously fetch a resource from a URL (supports file://, http://, https://, about:)
    [[nodiscard]] Resource load(const Url& url, ResourceType type = ResourceType::Other);

    // Cache management
    void clearCache() noexcept { m_cache.clear(); }
    [[nodiscard]] bool isCached(const Url& url) const noexcept;

private:
    [[nodiscard]] Resource loadLocalFile(const Url& url, ResourceType type);
    [[nodiscard]] Resource loadHttp(const Url& url, ResourceType type);

    std::unordered_map<std::string, Resource> m_cache;
    bool m_curlInitialized{false};
};

} // namespace browser::networking

