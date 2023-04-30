#include "request_manager.h"

namespace siren::cloud
{

    HttpResponse RequestManager::Get(const std::string& url, const std::string& body, const std::string& contentType, const Auth& auth, bool isVerifying)
    {
        return cpr::Get(cpr::Url{url}, cpr::VerifySsl(isVerifying),
                        cpr::Body{body}, cpr::Authentication{auth.user, auth.password, cpr::AuthMode::BASIC},
                        cpr::Header{{"Content-Type", contentType}});
    }

    HttpResponse RequestManager::Post(const std::string& url, const std::string& body, const std::string& contentType, const Auth& auth, bool isVerifying)
    {
        return cpr::Post(cpr::Url{url}, cpr::VerifySsl(isVerifying),
                         cpr::Body{body}, cpr::Authentication{auth.user, auth.password, cpr::AuthMode::BASIC},
                         cpr::Header{{"Content-Type", contentType}});
    }

    HttpResponse RequestManager::Put(const std::string& url, const std::string& body, const std::string& contentType, const Auth& auth, bool isVerifying)
    {
        return cpr::Put(cpr::Url{url}, cpr::VerifySsl(isVerifying),
                        cpr::Body{body}, cpr::Authentication{auth.user, auth.password, cpr::AuthMode::BASIC},
                        cpr::Header{{"Content-Type", contentType}});
    }

    HttpResponse RequestManager::Delete(const std::string& url, const std::string& body, const std::string& contentType, const Auth& auth, bool isVerifying)
    {
       return cpr::Delete(cpr::Url{url}, cpr::VerifySsl(isVerifying),
                           cpr::Body{body}, cpr::Authentication{auth.user, auth.password, cpr::AuthMode::BASIC},
                           cpr::Header{{"Content-Type", contentType}});
    }

    HttpResponse RequestManager::DownloadFile(const std::string& url, std::ofstream& ofstream)
    {
       cpr::Session session{};
       session.SetUrl(cpr::Url{url});
       return session.Download(ofstream);
    }
}