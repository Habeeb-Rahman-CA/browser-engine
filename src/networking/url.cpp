#include "browser/networking/url.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>

namespace browser::networking {

namespace {

std::string toLower(std::string_view str) {
    std::string res;
    res.reserve(str.size());
    for (char c : str) res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return res;
}

} // namespace

Url::Url(std::string_view urlString) {
    auto parsed = parse(urlString);
    if (parsed) {
        *this = *parsed;
    }
}

std::optional<Url> Url::parse(std::string_view str) {
    if (str.empty()) return std::nullopt;

    Url url;

    // 1. Check for scheme delimiter ':'
    size_t schemeEnd = str.find(':');
    if (schemeEnd == std::string_view::npos) {
        // Assume file:// or relative path
        if (str.starts_with("/")) {
            url.m_scheme = "file";
            url.m_path = std::string(str);
            url.m_valid = true;
            return url;
        }
        return std::nullopt;
    }

    url.m_scheme = toLower(str.substr(0, schemeEnd));
    std::string_view remainder = str.substr(schemeEnd + 1);

    if (url.m_scheme == "about") {
        url.m_path = std::string(remainder);
        url.m_valid = true;
        return url;
    }

    if (url.m_scheme == "file") {
        if (remainder.starts_with("//")) {
            remainder = remainder.substr(2);
        }
        url.m_path = std::string(remainder);
        url.m_valid = true;
        return url;
    }

    if (!remainder.starts_with("//")) {
        return std::nullopt;
    }
    remainder = remainder.substr(2); // Skip "//"

    // 2. Extract fragment '#'
    size_t fragPos = remainder.find('#');
    if (fragPos != std::string_view::npos) {
        url.m_fragment = std::string(remainder.substr(fragPos + 1));
        remainder = remainder.substr(0, fragPos);
    }

    // 3. Extract query '?'
    size_t queryPos = remainder.find('?');
    if (queryPos != std::string_view::npos) {
        url.m_query = std::string(remainder.substr(queryPos + 1));
        remainder = remainder.substr(0, queryPos);
    }

    // 4. Extract path '/'
    size_t pathPos = remainder.find('/');
    std::string_view authority;
    if (pathPos != std::string_view::npos) {
        authority = remainder.substr(0, pathPos);
        url.m_path = std::string(remainder.substr(pathPos));
    } else {
        authority = remainder;
        url.m_path = "/";
    }

    // 5. Extract host & port from authority
    size_t colonPos = authority.find(':');
    if (colonPos != std::string_view::npos) {
        url.m_host = toLower(authority.substr(0, colonPos));
        auto portStr = authority.substr(colonPos + 1);
        uint16_t p = 0;
        auto res = std::from_chars(portStr.data(), portStr.data() + portStr.size(), p);
        if (res.ec == std::errc()) {
            url.m_port = p;
        }
    } else {
        url.m_host = toLower(authority);
        if (url.m_scheme == "http") url.m_port = 80;
        else if (url.m_scheme == "https") url.m_port = 443;
    }

    url.m_valid = !url.m_host.empty();
    return url;
}

Url Url::resolve(const Url& base, std::string_view relative) {
    if (relative.empty()) return base;

    // If relative has an explicit scheme (e.g. "https://other.com/page"), parse it directly
    if (relative.find(':') != std::string_view::npos) {
        if (auto direct = parse(relative)) {
            return *direct;
        }
    }

    if (!base.isValid()) {
        return Url(relative);
    }

    Url resolved = base;

    // Fragment only
    if (relative.starts_with("#")) {
        resolved.m_fragment = std::string(relative.substr(1));
        return resolved;
    }

    // Query only
    if (relative.starts_with("?")) {
        resolved.m_query = std::string(relative.substr(1));
        return resolved;
    }

    // Absolute path on same host
    if (relative.starts_with("/")) {
        resolved.m_path = std::string(relative);
        resolved.m_query.clear();
        resolved.m_fragment.clear();
        return resolved;
    }

    // Relative path against base directory
    std::string basePath = base.path();
    size_t lastSlash = basePath.rfind('/');
    if (lastSlash != std::string::npos) {
        basePath = basePath.substr(0, lastSlash + 1);
    } else {
        basePath = "/";
    }

    resolved.m_path = basePath + std::string(relative);
    resolved.m_query.clear();
    resolved.m_fragment.clear();
    return resolved;
}

std::string Url::toString() const {
    if (!m_valid) return "";
    if (m_scheme == "about") {
        return "about:" + m_path;
    }
    if (m_scheme == "file") {
        return "file://" + m_path;
    }

    std::string s = m_scheme + "://" + m_host;
    if ((m_scheme == "http" && m_port != 80 && m_port != 0) ||
        (m_scheme == "https" && m_port != 443 && m_port != 0)) {
        s += ":" + std::to_string(m_port);
    }
    s += m_path;
    if (!m_query.empty()) {
        s += "?" + m_query;
    }
    if (!m_fragment.empty()) {
        s += "#" + m_fragment;
    }
    return s;
}

std::ostream& operator<<(std::ostream& os, const Url& url) {
    return os << url.toString();
}

} // namespace browser::networking
