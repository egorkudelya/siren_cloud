#pragma once

#include "api/api_umbrella.h"

namespace siren::cloud
{
    std::unique_ptr<ServerImpl> CreateServer();
}
