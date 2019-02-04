#ifndef CVUTILS_CACHE_ABSTRACT_CACHE_H
#define CVUTILS_CACHE_ABSTRACT_CACHE_H

#include <cstddef>

namespace cvutils
{
namespace cache
{
template <typename Key, typename Value>
class AbstractCache
{
public:
    virtual ~AbstractCache() { }

    // can be reimplemented to resize / reserve slots for the underlying data structure
    virtual void setCapacityHint(std::size_t capacity) { };

    // not const because it might add something to the cache
    virtual Value get(const Key& key) = 0;
    

};
} // namespace cache
} // namespace cvutils


#endif //CVUTILS_CACHE_ABSTRACT_CACHE_H
