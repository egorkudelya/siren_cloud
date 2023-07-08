#pragma once
#include "../common/common.h"

namespace siren::cloud
{
    struct HistogramEntry
    {
        HistogramEntry(SongIdType id, DeltaType delta, TimestampType ts)
            : songId(id), delta(delta), timestamp(ts)
        {
        }

        HistogramEntry& operator=(const HistogramEntry& rhs)
        {
            if (this != &rhs)
            {
                timestamp = rhs.timestamp;
                songId = rhs.songId;
                delta = rhs.delta;
            }
            return *this;
        }

        SongIdType songId;
        DeltaType delta;
        TimestampType timestamp;
    };
}
