#pragma once
#include "find_track.h"
#include "load_track.h"
#include "delete_track.h"
#include "../grpc/server.h"

namespace siren::cloud
{
    using ServerImpl = SirenServer<
                                SirenFingerprint,
                                LoadTrackByUrlCallData,
                                FindTrackByFingerprintCallData,
                                DeleteTrackByIdCallData
                                >;
}