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
        m_minWassersteinDistance = !minWassDistance.empty() ? std::stof(minWassDistance) : 45;

        std::unordered_multimap<HashType, std::pair<SongIdType, TimestampType>> dist;
        while (dbReturnPtr->fetchNext())
        {
            HashType hash;
            TimestampType timestamp;
            SongIdType songId;
            bool isValid = dbReturnPtr->asUint64("hash", hash)
                        && dbReturnPtr->asInt32("timestamp", timestamp)
                        && dbReturnPtr->asUint64("song_id", songId);

            if (!isValid)
            {
                Logger::log(LogLevel::ERROR, __FILE__, __FUNCTION__, __LINE__, "Could not extract necessary data from DBCommandPtr");
                continue;
            }
            dist.emplace(hash, std::make_pair(songId, timestamp));
        }

        for (auto incomingIt = fingerprint.cbegin(); incomingIt != fingerprint.cend(); incomingIt++)
        {
            HashType incomingHash = incomingIt->first;
            TimestampType incomingTs = incomingIt->second;

            auto tsIdRange = dist.equal_range(incomingHash);
            for (auto dbIt = tsIdRange.first; dbIt != tsIdRange.second; dbIt++)
            {
                SongIdType dbSongId = dbIt->second.first;
                TimestampType originalTs = dbIt->second.second;
                DeltaType delta = originalTs - incomingTs;

                m_histogram.insert(HistogramEntry{dbSongId, delta, originalTs});
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
        const auto& deltaIdIndex = m_histogram.get<tags::IdDeltaTsComposite>();

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
        auto counterPerSongId = groupDeltasByCount();
        auto maxIt = counterPerSongId.begin();

        std::vector<float> peak{(float)maxIt->first};
        std::vector<float> noise;

        auto it = counterPerSongId.begin();
        while (noise.size() <= 10 || it != counterPerSongId.end())
        {
            if (it->second != maxIt->second)
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

            auto lastMatch = m_histogram.get<tags::IdDeltaTsComposite>().find(std::make_tuple(matchId, matchDelta));
            return HistReturnType{HistStatus::OK, matchId, lastMatch->timestamp, wDistance};
        }

        std::stringstream err;
        err << "Could not infer song id, wasserstein distance: " << wDistance;
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, err.str());
        return HistReturnType{HistStatus::Uncertain};
    }

}
