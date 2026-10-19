#include "browser/networking/resource_loader.hpp"
#include "browser/core/diagnostics.hpp"

#include <fstream>
#include <iostream>

extern "C" {
    typedef void CURL;
    typedef int CURLcode;
    typedef int CURLoption;
    typedef int CURLINFO;

    #define CURLE_OK 0
    #define CURLOPT_URL 10002
    #define CURLOPT_WRITEFUNCTION 20011
    #define CURLOPT_WRITEDATA 10001
    #define CURLOPT_FOLLOWLOCATION 52
    #define CURLOPT_MAXREDIRS 68
    #define CURLOPT_TIMEOUT 13
    #define CURLOPT_CONNECTTIMEOUT 78
    #define CURLOPT_SSL_VERIFYPEER 64
    #define CURLOPT_SSL_VERIFYHOST 81
    #define CURLOPT_USERAGENT 10018
    #define CURLINFO_RESPONSE_CODE (0x200000 + 2)

    CURL* curl_easy_init(void);
    CURLcode curl_easy_setopt(CURL*, int, ...);
    CURLcode curl_easy_perform(CURL*);
    void curl_easy_cleanup(CURL*);
    CURLcode curl_easy_getinfo(CURL*, int, ...);
    const char* curl_easy_strerror(CURLcode);
    CURLcode curl_global_init(long flags);
    void curl_global_cleanup(void);
}

namespace browser::networking {

namespace {

size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buffer = static_cast<std::vector<uint8_t>*>(userdata);
    size_t total = size * nmemb;
    // Enforce 10 MB maximum resource size to protect memory bounds
    if (buffer->size() + total > 10 * 1024 * 1024) {
        return 0; // abort transfer
    }
    buffer->insert(buffer->end(), ptr, ptr + total);
    return total;
}

} // namespace

ResourceLoader::ResourceLoader() {
    if (curl_global_init(3L) == CURLE_OK) { // CURL_GLOBAL_ALL = 3
        m_curlInitialized = true;
    }
}

ResourceLoader::~ResourceLoader() {
    if (m_curlInitialized) {
        curl_global_cleanup();
    }
}

bool ResourceLoader::isCached(const Url& url) const noexcept {
    return m_cache.find(url.toString()) != m_cache.end();
}

Resource ResourceLoader::load(const Url& url, ResourceType type) {
    core::ScopedTimer timer(core::Subsystem::ResourceLoading);
    core::BrowserDiagnostics::instance().incrementCounter("ResourceRequests");

    std::string key = url.toString();
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        return it->second;
    }

    Resource res;
    if (url.isFile()) {
        res = loadLocalFile(url, type);
    } else if (url.isHttp() || url.isHttps()) {
        res = loadHttp(url, type);
    } else if (url.isAbout()) {
        res.url = url;
        res.type = ResourceType::Html;
        res.statusCode = 200;
        res.success = true;
        std::string html = "<!DOCTYPE html><html><body><h1>about:blank</h1></body></html>";
        res.data.assign(html.begin(), html.end());
    } else {
        res.url = url;
        res.success = false;
        res.errorMessage = "Unsupported URL scheme: " + url.scheme();
    }

    if (res.isSuccess()) {
        core::BrowserDiagnostics::instance().incrementCounter("BytesLoaded", res.data.size());
        m_cache[key] = res;
    }

    return res;
}

Resource ResourceLoader::loadLocalFile(const Url& url, ResourceType type) {
    Resource res;
    res.url = url;
    res.type = type;

    std::string filePath = url.path();
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file) {
        res.statusCode = 404;
        res.success = false;
        res.errorMessage = "Local file not found: " + filePath;
        return res;
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > 0) {
        res.data.resize(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(res.data.data()), size);
    }

    res.statusCode = 200;
    res.success = true;
    return res;
}

Resource ResourceLoader::loadHttp(const Url& url, ResourceType type) {
    Resource res;
    res.url = url;
    res.type = type;

    CURL* curl = curl_easy_init();
    if (!curl) {
        res.statusCode = 500;
        res.success = false;
        res.errorMessage = "Failed to initialize libcurl session";
        return res;
    }

    std::string urlStr = url.toString();
    curl_easy_setopt(curl, CURLOPT_URL, urlStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.data);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "BrowserEngine/0.1.0 Educational");

    // Strict TLS certificate verification
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode code = curl_easy_perform(curl);
    if (code == CURLE_OK) {
        long responseCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
        res.statusCode = static_cast<int>(responseCode);
        res.success = (res.statusCode >= 200 && res.statusCode < 400);
        if (!res.success) {
            res.errorMessage = "HTTP error " + std::to_string(res.statusCode);
        }
    } else {
        res.statusCode = 0;
        res.success = false;
        res.errorMessage = std::string("Network transfer error: ") + curl_easy_strerror(code);
    }

    curl_easy_cleanup(curl);
    return res;
}

} // namespace browser::networking

