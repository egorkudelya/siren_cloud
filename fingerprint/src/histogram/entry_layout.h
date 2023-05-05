#pragma once
#include "../common/common.h"

namespace siren::cloud
{
    struct HistogramEntry
    {
        HistogramEntry(SongIdType id, DeltaType delta, TimestampType ts)
            : song_id(id), delta(delta), timestamp(ts), counter(0)
        {
        }

        HistogramEntry& operator=(const HistogramEntry& rhs)
        {
            if (this != &rhs)
            {
                timestamp = rhs.timestamp;
                song_id = rhs.song_id;
                counter = rhs.counter;
                delta = rhs.delta;
            }
            return *this;
        }

        SongIdType song_id;
        DeltaType delta;
        CounterType counter;
        TimestampType timestamp;
    };
}
