#pragma once
#include <cpr/cpr.h>
#include <fstream>

namespace siren::cloud
{
    using HttpResponse = cpr::Response;

    struct Auth
    {
        std::string user{};
        std::string password{};
    };

    class RequestManager
    {
    public:
        RequestManager(const RequestManager& other) = delete;
        RequestManager(RequestManager&& other) = delete;
        RequestManager& operator=(const RequestManager& other) = delete;
        RequestManager& operator=(RequestManager&& other) = delete;

        static HttpResponse Get(const std::string& url, std::string_view body, const std::string& contentType, const Auth& auth, bool isVerifying=true);
        static HttpResponse Post(const std::string& url, std::string_view body, const std::string& contentType, const Auth& auth, bool isVerifying=true);
        static HttpResponse Put(const std::string& url, std::string_view body, const std::string& contentType, const Auth& auth, bool isVerifying=true);
        static HttpResponse Delete(const std::string& url, std::string_view body, const std::string& contentType, const Auth& auth, bool isVerifying=true);
        static HttpResponse DownloadFile(const std::string& url, std::ofstream& stream, int timeout, bool isVerifying=true);

    private:
        RequestManager() = default;
    };
}