#pragma once

#include <functional>
#include <unordered_set>

#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <siren_core/src/entities/fingerprint.h>
#include <wasserstein/wasserstein.h>

#include "../storage/abstract_command.h"
#include "../logger/logger.h"
#include "entry_layout.h"

namespace siren::cloud
{

    namespace tags
    {
        struct DeltaCounterComposite {};
        struct Id {};
        struct Counter {};
        struct Delta {};
        struct Timestamp {};
    }

    enum class HistStatus
    {
        OK = 1,
        Uncertain = 2,
    };

    struct HistReturnType
    {
        HistReturnType(HistStatus s);
        HistReturnType(HistStatus s, SongIdType id, TimestampType ts);
        explicit operator bool() const;

        HistStatus getStatus() const;
        SongIdType getSongId() const;
        TimestampType getTimestamp() const;

    private:
        HistStatus m_status;
        SongIdType m_song_id{};
        TimestampType m_timestamp{};
    };


    class Histogram
    {
        template<typename Ts, typename Counter>
        using HistogramContainer = boost::multi_index::multi_index_container<
            HistogramEntry,
            boost::multi_index::indexed_by<
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<tags::DeltaCounterComposite>,
                    boost::multi_index::key<&HistogramEntry::counter,
                        &HistogramEntry::delta>,
                    boost::multi_index::composite_key_compare<
                        std::greater<Ts>,
                        std::less<Counter>
                        >
                    >,
                boost::multi_index::ordered_unique<
                    boost::multi_index::tag<tags::Id>,
                    boost::multi_index::key<&HistogramEntry::song_id>
                    >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<tags::Timestamp>,
                    boost::multi_index::key<&HistogramEntry::timestamp>
                    >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<tags::Delta>,
                    boost::multi_index::key<&HistogramEntry::delta>
                    >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<tags::Counter>,
                    boost::multi_index::key<&HistogramEntry::counter>
                    >
                >
            >;

        HistogramContainer<TimestampType, CounterType> m_histogram;
        std::unordered_set<SongIdType> m_ids;
        float m_peakPct;
        float m_minWassDistance;
        float m_minZscore;

    public:
        using iterator = HistogramContainer<TimestampType, CounterType>::iterator;
        using const_iterator = HistogramContainer<TimestampType, CounterType>::const_iterator;

        Histogram(const DBCommandPtr& dbReturnPtr, const FingerprintType& fingerprint);
        HistReturnType findDominantPeak();
        iterator begin();
        iterator end();
        const_iterator cbegin() const;
        const_iterator cend() const;

    private:
        template<typename Index, typename Class, typename PropertyType>
        std::vector<PropertyType> flattenIndex(const Index& index, PropertyType Class::*member)
        {
            std::vector<PropertyType> dest;
            std::transform(index.begin(), index.end(), std::inserter(dest, dest.end()),
            [&member](auto&& entry)
            {
                 return entry.*(member);
            });
            return dest;
        }

        template<typename T>
        double getMedian(std::vector<T> dist)
        {
            if (dist.empty())
            {
                return 0;
            }
            std::sort(dist.begin(), dist.end());
            return dist[dist.size()/2];
        }

        template<typename Index, typename Class, typename PropertyType>
        double getMedian(const Index& index, PropertyType Class::*member)
        {
            std::vector<PropertyType> dist = flattenIndex(index, member);
            return getMedian(std::move(dist));
        }

        template<typename T>
        double getMad(std::vector<T> dist)
        {
            if (dist.empty())
            {
                return 0;
            }

            double median = getMedian(dist);
            size_t size = dist.size();

            std::vector<double> deviations(size);
            std::transform(dist.begin(), dist.end(), deviations.begin(),
            [&](double x)
            {
                return std::abs(x - median);
            });

            std::sort(deviations.begin(), deviations.end());
            return deviations[size/2];
        }

        template<typename Index, typename Class, typename PropertyType>
        double getMad(const Index& index, PropertyType Class::*member)
        {
            std::vector<PropertyType> dist = flattenIndex(index, member);
            return getMad(std::move(dist));
        }

        template<typename T>
        double getMZScoreOfPoint(const std::vector<T>& dist, T point)
        {
            if (dist.empty())
            {
                return 0;
            }

            double median = getMedian(dist);
            double mad = getMad(dist);

            auto it = std::find(dist.begin(), dist.end(), point);
            if (it == dist.end())
            {
                return 0;
            }

            double zMScore = 0.6745 * ((*it - median) / mad);
            if (isnan(zMScore))
            {
                zMScore = 0;
            }
            return zMScore;
        }

        template<typename T>
        auto getMZScoreOfPoints(const std::vector<T>& dist)
        {
            std::vector<std::pair<double, T>> res;
            for (T point: dist)
            {
                double zMScore = getMZScoreOfPoint(dist, point);
                res.emplace_back(zMScore, point);
            }
            return res;
        }

        template<typename Index, typename Class, typename PropertyType>
        auto getMZScoreOfPoints(const Index& index, PropertyType Class::*member)
        {
            std::vector<PropertyType> dist = flattenIndex(index, member);
            return getMZScoreOfPoints<PropertyType>(dist);
        }

    };

}// namespace siren::service