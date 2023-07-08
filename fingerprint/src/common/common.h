#pragma once
#include "siren_core/src/common/common.h"
#include "siren_core/src/entities/fingerprint.h"
#include <nlohmann/json.hpp>
#include <condition_variable>

namespace siren::cloud
{
    using TimestampType     =  int32_t;
    using CounterType       =  size_t;
    using DeltaType         =  int32_t;
    using SongIdType        =  uint64_t;
    using HashType          =  uint64_t;
    using FingerprintType   =  Fingerprint<>;
    using Json              =  nlohmann::json;

    bool generateUniqueFilePath(std::string path, std::string& res);
}