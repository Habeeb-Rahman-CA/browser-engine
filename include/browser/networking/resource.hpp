#pragma once

#include "browser/networking/url.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace browser::networking {

enum class ResourceType {
    Html,
    Css,
    Image,
    Script,
    Other
};

struct Resource {
    Url url;
    ResourceType type{ResourceType::Other};
    int statusCode{200};
    std::string contentType;
    std::vector<uint8_t> data;
    bool success{false};
    std::string errorMessage;

    [[nodiscard]] std::string asText() const {
        return std::string(reinterpret_cast<const char*>(data.data()), data.size());
    }

    [[nodiscard]] bool isSuccess() const noexcept {
        return success && statusCode >= 200 && statusCode < 300;
    }
};

} // namespace browser::networking

