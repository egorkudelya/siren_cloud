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
        RequestManager() = delete;

        static HttpResponse Get(const std::string& url, const std::string& body, const std::string& contentType, const Auth& auth, bool isVerifying=false);
        static HttpResponse Post(const std::string& url, const std::string& body, const std::string& contentType, const Auth& auth, bool isVerifying=false);
        static HttpResponse Put(const std::string& url, const std::string& body, const std::string& contentType, const Auth& auth, bool isVerifying=false);
        static HttpResponse Delete(const std::string& url, const std::string& body, const std::string& contentType, const Auth& auth, bool isVerifying=false);
        static HttpResponse DownloadFile(const std::string& url, std::ofstream& stream);
    };
}