#pragma once

#include <functional>
#include <unordered_set>

#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
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
        struct Id {};
        struct Delta {};
        struct Timestamp {};
        struct IdDeltaComposite {};
    }

    enum class HistStatus
    {
        OK = 1,
        Uncertain = 2,
    };

    struct HistReturnType
    {
        HistReturnType(HistStatus status);
        HistReturnType(HistStatus status, SongIdType id, TimestampType ts, float ws);
        explicit operator bool() const;

        HistStatus getStatus() const;
        SongIdType getSongId() const;
        TimestampType getTimestamp() const;
        float getWassersteinDistance() const;

    private:
        HistStatus m_status;
        SongIdType m_songId;
        TimestampType m_timestamp;
        float m_wassersteinDistance;
    };


    class Histogram
    {
        using DeltaCounterForIds = std::multimap<CounterType, std::pair<SongIdType, DeltaType>, std::greater<>>;
        using HashHistogram = std::unordered_multimap<HashType, std::pair<SongIdType, TimestampType>>;
        using HistogramContainer = boost::multi_index::multi_index_container<
            HistogramEntry,
            boost::multi_index::indexed_by<
                boost::multi_index::hashed_non_unique<
                    boost::multi_index::tag<tags::Id>,
                    boost::multi_index::key<&HistogramEntry::songId>
                    >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<tags::Timestamp>,
                    boost::multi_index::key<&HistogramEntry::timestamp>,
                    std::greater<>
                    >,
                boost::multi_index::hashed_non_unique<
                    boost::multi_index::tag<tags::Delta>,
                    boost::multi_index::key<&HistogramEntry::delta>
                    >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<tags::IdDeltaComposite>,
                    boost::multi_index::key<&HistogramEntry::songId, &HistogramEntry::delta>
                    >
                >
            >;

        HistogramContainer m_histogram;
        float m_minWassersteinDistance;

    public:
        using iterator = HistogramContainer::iterator;
        using const_iterator = HistogramContainer::const_iterator;

        Histogram(const DBCommandPtr& dbReturnPtr, const FingerprintType& fingerprint);
        iterator begin();
        iterator end();
        const_iterator cbegin() const;
        const_iterator cend() const;
        HistReturnType findDominantPeak();

    private:
        DeltaCounterForIds groupDeltasByCount() const;
    };

}// namespace siren::service