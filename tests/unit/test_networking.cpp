#include "test_framework.hpp"
#include "browser/networking/url.hpp"
#include "browser/networking/resource_loader.hpp"

using namespace browser::networking;

TEST_CASE("Networking: URL parsing and components") {
    auto urlOpt = Url::parse("https://example.com:8443/docs/index.html?search=cpp#intro");
    REQUIRE(urlOpt.has_value());

    const auto& url = *urlOpt;
    CHECK(url.isValid());
    CHECK(url.isHttps());
    CHECK_EQ(url.scheme(), "https");
    CHECK_EQ(url.host(), "example.com");
    CHECK_EQ(url.port(), 8443);
    CHECK_EQ(url.path(), "/docs/index.html");
    CHECK_EQ(url.query(), "search=cpp");
    CHECK_EQ(url.fragment(), "intro");
}

TEST_CASE("Networking: Default ports for HTTP and HTTPS") {
    auto httpUrl = Url::parse("http://example.org/home");
    REQUIRE(httpUrl.has_value());
    CHECK_EQ(httpUrl->port(), 80);

    auto httpsUrl = Url::parse("https://example.org/home");
    REQUIRE(httpsUrl.has_value());
    CHECK_EQ(httpsUrl->port(), 443);
}

TEST_CASE("Networking: Relative URL resolution") {
    auto base = *Url::parse("https://example.com/blog/posts/index.html");

    // Relative file in same directory
    auto rel1 = Url::resolve(base, "article.html");
    CHECK_EQ(rel1.toString(), "https://example.com/blog/posts/article.html");

    // Absolute path on same host
    auto rel2 = Url::resolve(base, "/images/banner.png");
    CHECK_EQ(rel2.toString(), "https://example.com/images/banner.png");

    // Hash fragment
    auto rel3 = Url::resolve(base, "#comments");
    CHECK_EQ(rel3.toString(), "https://example.com/blog/posts/index.html#comments");

    // Full absolute URL replaces base completely
    auto rel4 = Url::resolve(base, "https://other.org/page");
    CHECK_EQ(rel4.toString(), "https://other.org/page");
}

TEST_CASE("Networking: ResourceLoader loads local files and about:blank") {
    ResourceLoader loader;

    // 1. about:blank
    auto aboutUrl = *Url::parse("about:blank");
    auto aboutRes = loader.load(aboutUrl);
    CHECK(aboutRes.isSuccess());
    CHECK_EQ(aboutRes.statusCode, 200);
    CHECK(aboutRes.asText().find("about:blank") != std::string::npos);

    // 2. Existing local file
    auto fileUrl = *Url::parse("file://examples/basic/index.html");
    auto fileRes = loader.load(fileUrl, ResourceType::Html);
    CHECK(fileRes.isSuccess());
    CHECK_EQ(fileRes.statusCode, 200);
    CHECK(fileRes.asText().find("Welcome to My Browser Engine") != std::string::npos);

    // 3. Cached lookup
    CHECK(loader.isCached(fileUrl));

    // 4. Missing file returns 404
    auto missingUrl = *Url::parse("file://nonexistent_file_12345.html");
    auto missingRes = loader.load(missingUrl);
    CHECK_FALSE(missingRes.isSuccess());
    CHECK_EQ(missingRes.statusCode, 404);
}

