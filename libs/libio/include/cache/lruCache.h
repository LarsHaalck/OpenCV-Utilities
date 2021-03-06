#ifndef CVUTILS_LRU_CACHE_H
#define CVUTILS_LRU_CACHE_H

#include "cache/abstractCache.h"
#include "fetch/abstractFetcher.h"

#include <list>
#include <memory>
#include <unordered_map>
#include <thread>

namespace cvutils
{
namespace detail
{
template <typename Key, typename Value>
class LRUCache : public AbstractCache<Key, Value>
{
private:
    using listNode = std::pair<Key, Value>;
    int mCapacity;
    std::list<listNode> mList;
    std::unordered_map<Key, typename std::list<listNode>::iterator> mMap;
    std::shared_ptr<AbstractFetcher<Key, Value>> mReader;

    std::mutex mMutex;
public:
    LRUCache(int capacity, std::shared_ptr<AbstractFetcher<Key, Value>> reader)
        : mCapacity(capacity)
        , mList()
        , mMap(capacity)
        , mReader(std::move(reader))
    {

    }

    Value get(const Key& key)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        // not in cache
        if (!mMap.count(key))
        {
            // cache is full
            if (static_cast<int>(mMap.size()) == mCapacity)
            {
                auto node = mList.back();
                mList.pop_back();
                mMap.erase(node.first);
            }

            // let reader get a new value
            mList.push_front(std::make_pair(key, mReader->get(key)));
        }
        else
        {
            // element is in cache, retrieve and move to new position to
            // avoid re-reading it from file
            auto pairIt = mMap[key];
            auto keyValuePair = std::move(*pairIt);
            mList.erase(pairIt);
            mList.push_front(keyValuePair);
        }

        // update iterator
        mMap[key] = mList.begin();
        return mList.front().second;
    }

};
} // namespace detail
} // namespace cvutils

#endif // CVUTILS_LRU_CACHE_H
