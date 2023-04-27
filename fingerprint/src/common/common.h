#pragma once
#include "siren_core/src/common/common.h"
#include "siren_core/src/entities/fingerprint.h"

namespace siren::cloud
{
    using TimestampType     =  int32_t;
    using CounterType       =  size_t;
    using DeltaType         =  int32_t;
    using SongIdType        =  uint64_t;
    using HashType          =  uint64_t;
    using FingerprintType   =  Fingerprint<>;

    bool generateUniqueFilePath(std::string path, std::string& res);
}