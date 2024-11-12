/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2024-07-20
    License: MIT
*/

#pragma once
#include "Containers/Inlinedvector.hpp"
#include "Containers/Bytebuffer.hpp"
#include "Containers/Protobuffer.hpp"
#include "Containers/Ringbuffer.hpp"
#include <unordered_map>
#include <unordered_set>
#include <map>

// Select our desired implementation for the containers.
#if __has_include(<absl/container/inlined_vector.h>)
#include <absl/container/inlined_vector.h>

template <typename T, size_t N>
using Inlinedvector_t = absl::InlinedVector<T, N>;
#else
template <typename T, size_t N>
using Inlinedvector_t = Inlinedvector<T, N>;
#endif

#if __has_include(<absl/container/flat_hash_map.h>)
#include <absl/container/flat_hash_map.h>

template <class K, class V,
          class Hash = absl::container_internal::hash_default_hash<K>,
          class Eq = absl::container_internal::hash_default_eq<K>>
using Hashmap_t = absl::flat_hash_map<K, V, Hash, Eq>;
#else
template <class K, class V,
          class Hash = std::hash<K>,
          class Eq = std::equal_to<K>>
using Hashmap_t = std::unordered_map<K, V, Hash, Eq>;
#endif

#if __has_include(<absl/container/btree_map.h>)
#include <absl/container/btree_map.h>

template <typename Key, typename Value,
          typename Compare = std::less<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>>
using Btree_t = absl::btree_map<Key, Value, Compare, Alloc>;
#else
template <class Key, class Value,
          class Compare = std::less<Key>,
          class Alloc = std::allocator<std::pair<const Key, Value>>>
using Btree_t = std::map<Key, Value, Compare, Alloc>;
#endif

#if __has_include(<absl/container/flat_hash_set.h>)
#include <absl/container/flat_hash_set.h>

template <class T, class Hash = absl::container_internal::hash_default_hash<T>,
          class Eq = absl::container_internal::hash_default_eq<T>>
using Hashset_t = absl::flat_hash_set<T, Hash, Eq>;
#else
template <class T, class Hash = std::hash<T>,
          class Eq = std::equal_to<T>>
using Hashset_t = std::unordered_set<T, Hash, Eq>;
#endif
