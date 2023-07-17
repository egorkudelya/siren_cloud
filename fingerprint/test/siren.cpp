#include <gtest/gtest.h>
#include "../src/common/request_manager.h"
#include "../src/common/common.h"

using namespace siren::cloud;

TEST(SirenAlgorithm, TestGeneral)
{
    std::string addr = siren::getenv("SIREN_HOST");
    std::string port = siren::getenv("SIREN_PORT");
    std::string url = "https://" + addr + ':' + port + "/records/findByFingerprint";
    std::string contentType = "application/json";

    auto requestAndCheck = [&](const std::string& path, const std::string& targetName)
    {
        std::ifstream fingerprint(path);
        std::string content((std::istreambuf_iterator<char>(fingerprint)), (std::istreambuf_iterator<char>()));

        HttpResponse response = RequestManager::Post(url, content, contentType, {});

        if (response.status_code != 200)
        {
            return false;
        }

        Json json = Json::parse(std::move(response.text));
        if (json["name"] != targetName)
        {
            if (targetName == "Doin Time" && json["name"] == "Doin' Time")
            {
                return true;
            }
            return false;
        }
        return true;
    };

    const std::filesystem::path cassettes{"../test/cassettes"};

    int hits = 0, misses = 0;
    for (const auto& cassette : std::filesystem::directory_iterator{cassettes})
    {
        const auto& path = cassette.path();
        if (path.extension() == ".json" && path.stem() != "master_params")
        {
            if (requestAndCheck(path, path.stem()))
            {
                hits++;
                continue;
            }
            misses++;
        }
    }

    ASSERT_TRUE(hits > 0);

    float acc = float(hits) / float(hits + misses);
    ASSERT_TRUE(acc > 0.65);
    std::cerr << "Est. accuracy after evaluating the algorithm on " << hits + misses << " cassettes: " << acc << std::endl;
}