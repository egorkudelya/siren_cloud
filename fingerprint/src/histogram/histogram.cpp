#include "histogram.h"
#include <sstream>

namespace siren::cloud
{

    HistReturnType::HistReturnType(HistStatus s)
        : m_status(s)
    {
    }
    HistReturnType::HistReturnType(HistStatus s, SongIdType id, TimestampType ts)
        : m_status(s)
        , m_song_id(id)
        , m_timestamp(ts)
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
        return m_song_id;
    }

    TimestampType HistReturnType::getTimestamp() const
    {
        return m_timestamp;
    }

    Histogram::Histogram(const DBCommandPtr& dbReturnPtr, const FingerprintType& fingerprint)
    {
        std::string peakPercent = siren::getenv("MINIMUM_PEAK_PERCENT");
        std::string minWassDistance = siren::getenv("MINIMUM_WASSERSTEIN_DISTANCE");
        std::string minZscore = siren::getenv("MINIMUM_ZSCORE");

        m_peakPct = !peakPercent.empty() ? std::stoi(peakPercent)/100 : 0.33f;
        m_minWassDistance = !minWassDistance.empty() ? std::stof(minWassDistance) : 250;
        m_minZscore = !minZscore.empty() ? std::stof(minZscore) : 3.25;

        std::unordered_multimap<HashType, std::pair<TimestampType, SongIdType>> dist;
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
            dist.emplace(hash, std::make_pair(timestamp, songId));
        }

        for (auto fptIt = fingerprint.cbegin(); fptIt != fingerprint.cend(); fptIt++)
        {
            auto tsIdRange = dist.equal_range(fptIt->first);
            for (auto dbIt = tsIdRange.first; dbIt != tsIdRange.second; dbIt++)
            {
                SongIdType dbSongId = dbIt->second.second;
                TimestampType originalTs = dbIt->second.first;
                TimestampType incomingTs = fptIt->second;
                DeltaType delta = originalTs - incomingTs;

                m_ids.insert(dbSongId);
                auto histogramIt = m_histogram.insert(HistogramEntry{dbSongId, delta, originalTs}).first;

                m_histogram.modify(histogramIt, (&boost::lambda::_1)->*& HistogramEntry::counter += 1);
                if (histogramIt->timestamp < originalTs)
                {
                    m_histogram.modify(histogramIt, (&boost::lambda::_1)->*& HistogramEntry::timestamp = originalTs);
                }
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

    HistReturnType Histogram::findDominantPeak()
    {
        auto flattenCounterMap = [](const auto& map, auto& vec)
        {
            for (auto [counter, _]: map)
            {
                vec.push_back(counter);
            }
        };

        auto composeOutlierMaps = [this](auto&& scores, float threshold)
        {
            std::multimap<CounterType, SongIdType, std::greater<CounterType>> peaksMap, noiseMap;
            for (const auto& [zscore, counter]: scores)
            {
                auto it = m_histogram.get<tags::Counter>().find(counter);
                if (scores.size() == 1 || zscore >= threshold)
                {
                    peaksMap.emplace(counter, it->song_id);
                }
                else
                {
                    noiseMap.emplace(counter, it->song_id);
                }
            }
            return std::make_pair(peaksMap, noiseMap);
        };

        auto getOutliersOfFlatIndex = [&](const std::vector<float>& flatIndex, float threshold)
        {
            auto scores = getMZScoreOfPoints(flatIndex);
            return composeOutlierMaps(std::move(scores), threshold);
        };

        auto getOutliersOfIndex = [&](const auto& index, const auto& dim, float threshold)
        {
            auto scores = getMZScoreOfPoints(index, dim);
            return composeOutlierMaps(std::move(scores), threshold);
        };

        auto& deltaCounterIdx = m_histogram.get<tags::Counter>();

        size_t prevPeaksCount = 0;
        while (!deltaCounterIdx.empty())
        {
            auto [peaksMap, noiseMap] = getOutliersOfIndex(deltaCounterIdx, &HistogramEntry::counter, m_minZscore);
            if (peaksMap.empty())
            {
                return HistReturnType{HistStatus::Uncertain};
            }

            std::vector<float> peaks, noise;

            flattenCounterMap(peaksMap, peaks);
            flattenCounterMap(noiseMap, noise);

            if (noise.empty())
            {
                noise.push_back(0.0f);
            }

            std::vector<float> peakWeights(peaks.size(), 1);
            std::vector<float> noiseWeights(noise.size(), 1);

            float wassDistance = wasserstein(peaks, peakWeights, noise, noiseWeights);
            if (wassDistance > m_minWassDistance)
            {
                auto it = peaksMap.begin();
                size_t idCount = std::count_if(peaksMap.begin(), peaksMap.end(), [&it](auto&& entry) {
                    return entry.second == it->second;
                });

                std::set<SongIdType> uniqueIdDist;
                for (const auto& [counter, songId]: peaksMap)
                {
                    uniqueIdDist.insert(songId);
                }

                // check if dominant peak is significantly larger than peers
                auto [subPeaksMap, _] = getOutliersOfFlatIndex(peaks, 3);
                if (peaksMap.size() == 1 || subPeaksMap.size() == 1 || uniqueIdDist.size() * m_peakPct <= idCount)
                {
                    auto entry = deltaCounterIdx.find(it->first);
                    TimestampType ts{0};
                    if (entry != deltaCounterIdx.end())
                    {
                        ts = entry->timestamp;
                    }
                    std::stringstream stream;
                    stream << " Found a song with song id: " << it->second <<
                              " peak: " << it->first <<
                              " w. distance: " << wassDistance <<
                              " timestamp: " << ts << std::endl;
                    Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, stream.str());
                    return HistReturnType{HistStatus::OK, it->second, ts};
                }
            }

            if (noiseMap.empty() || prevPeaksCount == peaks.size())
            {
                Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "Could not infer song id");
                return HistReturnType{HistStatus::Uncertain};
            }
            prevPeaksCount = peaks.size();

            // purge noise from index
            for (auto [counter, _]: noiseMap)
            {
                auto it = deltaCounterIdx.find(counter);
                if (it != deltaCounterIdx.end())
                {
                    deltaCounterIdx.erase(it);
                }
            }
        }
        Logger::log(LogLevel::INFO, __FILE__, __FUNCTION__, __LINE__, "deltaCounterIdx is empty; could not infer song id");
        return HistReturnType{HistStatus::Uncertain};
    }

}
