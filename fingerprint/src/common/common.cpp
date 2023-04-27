#include "common.h"

namespace siren::cloud
{
    bool generateUniqueFilePath(std::string path, std::string& res)
    {
        path += "/XXXXXXXX";
        path.push_back('\0');
        int fd = mkstemp(&path[0]);
        if (fd != -1)
        {
            path.pop_back();
            res = path;
            return true;
        }
        return false;
    }
}