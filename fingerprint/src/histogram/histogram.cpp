#include "histogram.h"
#include <sstream>

namespace siren::cloud
{

    HistReturnType::HistReturnType(HistStatus status)
        : m_status(status)
    {
    }
    HistReturnType::HistReturnType(HistStatus status, SongIdType id, TimestampType ts, float ws)
        : m_status(status)
        , m_songId(id)
        , m_timestamp(ts)
        , m_wassersteinDistance(ws)
    {
    }

    HistReturnType::operator bool() const
    {
        return m_status == HistStatus::OK;
    }

    HistStatus HistReturnType::getStatus() const
    {
        return m_status;
    }

    SongIdType HistReturnType::getSongId() const
    {
        return m_songId;
    }

    TimestampType HistReturnType::getTimestamp() const
    {
        return m_timestamp;
    }

    float HistReturnType::getWassersteinDistance() const
    {
        return m_wassersteinDistance;
    }

    Histogram::Histogram(const DBCommandPtr& dbReturnPtr, const FingerprintType& fingerprint)
    {
        std::string minWassDistance = siren::getenv("MIN_WASSERSTEIN_DISTANCE");
        m_minWassersteinDistance = !minWassDistance.empty() ? std::stof(minWassDistance) : 27.5;

        HashHistogram hist;
        hist.reserve(dbReturnPtr->getSize());
        m_histogram.reserve(fingerprint.get_size());

        HashType hash;
        TimestampType timestamp;
        SongIdType songId;
        while (dbReturnPtr->fetchNext())
        {
            bool isValid = dbReturnPtr->asUint64("hash", hash) && dbReturnPtr->asInt32("timestamp", timestamp) && dbReturnPtr->asUint64("song_id", songId);
            if (!isValid)
            {
                Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Could not extract necessary data from DBCommandPtr");
                continue;
            }
            hist.emplace(hash, std::make_pair(songId, timestamp));
        }

        for (auto incomingIt = fingerprint.cbegin(); incomingIt != fingerprint.cend(); incomingIt++)
        {
            const HashType& incomingHash = incomingIt->first;
            const TimestampType& incomingTs = incomingIt->second;
            auto tsIdRange = hist.equal_range(incomingHash);
            for (auto dbIt = tsIdRange.first; dbIt != tsIdRange.second; dbIt++)
            {
                const SongIdType& dbSongId = dbIt->second.first;
                const TimestampType& originalTs = dbIt->second.second;
                DeltaType delta = originalTs - incomingTs;

                m_histogram.emplace(std::move(HistogramEntry{dbSongId, delta, originalTs}));
            }
        }
    }

    Histogram::iterator Histogram::begin()
    {
        return m_histogram.begin();
    }

    Histogram::iterator Histogram::end()
    {
        return m_histogram.end();
    }

    Histogram::const_iterator Histogram::cbegin() const
    {
        return m_histogram.cbegin();
    }

    Histogram::const_iterator Histogram::cend() const
    {
        return m_histogram.cend();
    }

    Histogram::DeltaCounterForIds Histogram::groupDeltasByCount() const
    {
        const auto& deltaIdIndex = m_histogram.get<tags::IdDeltaComposite>();

        DeltaCounterForIds countedDeltas;
        for (auto first = deltaIdIndex.begin(), last = deltaIdIndex.end(); first != last;)
        {
            auto next = deltaIdIndex.upper_bound(std::make_tuple(first->songId, first->delta));
            auto dist = std::distance(first, next);

            countedDeltas.emplace(dist, std::make_pair(first->songId, first->delta));
            first = next;
        }
        return countedDeltas;
    }

    HistReturnType Histogram::findDominantPeak()
    {
        DeltaCounterForIds counterPerSongId = groupDeltasByCount();
        auto maxIt = counterPerSongId.begin();

        std::vector<float> peak{(float)maxIt->first};
        std::vector<float> noise;

        auto it = counterPerSongId.begin();

        // comparing the most significant signal to other potential candidates
        while (noise.size() <= 10 && it != counterPerSongId.end())
        {
            if (it->second.first != maxIt->second.first)
            {
                noise.push_back(it->first);
            }
            it++;
        }

        if (noise.empty())
        {
            noise.push_back(1.0f);
        }

        std::vector<float> peakWeights(peak.size(), 1);
        std::vector<float> noiseWeights(noise.size(), 1);

        float wDistance = wasserstein(peak, peakWeights, noise, noiseWeights);
        if (m_minWassersteinDistance < wDistance)
        {
            SongIdType matchId = maxIt->second.first;
            DeltaType matchDelta = maxIt->second.second;

            auto lastMatch = m_histogram.get<tags::IdDeltaComposite>().find(std::make_tuple(matchId, matchDelta));

//            std::stringstream msg;
//            msg << "Found a song with song id " << matchId << ", wasserstein distance: " << wDistance;
//            Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, msg.str());

            return HistReturnType{HistStatus::OK, matchId, lastMatch->timestamp, wDistance};
        }

        std::stringstream err;
        err << "Could not infer song id, wasserstein distance: " << wDistance;
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, err.str());
        return HistReturnType{HistStatus::Uncertain};
    }

}
