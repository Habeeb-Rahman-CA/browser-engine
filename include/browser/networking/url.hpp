#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include <iosfwd>

namespace browser::networking {

class Url {
public:
    Url() = default;
    explicit Url(std::string_view urlString);

    [[nodiscard]] static std::optional<Url> parse(std::string_view urlString);
    [[nodiscard]] static Url resolve(const Url& base, std::string_view relative);

    [[nodiscard]] const std::string& scheme() const noexcept { return m_scheme; }
    [[nodiscard]] const std::string& host() const noexcept { return m_host; }
    [[nodiscard]] uint16_t port() const noexcept { return m_port; }
    [[nodiscard]] const std::string& path() const noexcept { return m_path; }
    [[nodiscard]] const std::string& query() const noexcept { return m_query; }
    [[nodiscard]] const std::string& fragment() const noexcept { return m_fragment; }

    [[nodiscard]] bool isValid() const noexcept { return m_valid; }
    [[nodiscard]] bool isHttp() const noexcept { return m_scheme == "http"; }
    [[nodiscard]] bool isHttps() const noexcept { return m_scheme == "https"; }
    [[nodiscard]] bool isFile() const noexcept { return m_scheme == "file"; }
    [[nodiscard]] bool isAbout() const noexcept { return m_scheme == "about"; }

    [[nodiscard]] std::string toString() const;
    bool operator==(const Url& other) const = default;

private:
    std::string m_scheme;
    std::string m_host;
    uint16_t m_port{0};
    std::string m_path{"/"};
    std::string m_query;
    std::string m_fragment;
    bool m_valid{false};
};

std::ostream& operator<<(std::ostream& os, const Url& url);

} // namespace browser::networking
