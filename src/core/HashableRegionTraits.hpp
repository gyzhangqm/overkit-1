// Copyright (c) 2020 Matthew J. Smith and Overkit contributors
// License: MIT (http://opensource.org/licenses/MIT)

#ifndef OVK_CORE_HASHABLE_REGION_TRAITS_HPP_INCLUDED
#define OVK_CORE_HASHABLE_REGION_TRAITS_HPP_INCLUDED

#include <ovk/core/Global.hpp>
#include <ovk/core/Interval.hpp>
#include <ovk/core/Set.hpp>
#include <ovk/core/Tuple.hpp>

#include <type_traits>

namespace ovk {
namespace core {

enum class hashable_region_maps_to {
  RANGE,
  SET
};

template <typename T, typename=void> struct hashable_region_traits {
/*
  using coord_type = ???;
  static constexpr hashable_region_maps_to MapsTo() { return ???; }
  static interval<coord_type,MAX_DIMS> ComputeExtents(int NumDims, const T &Region) { return ???; }
  // If maps to range
  static range MapToBins(int NumDims, const range &BinRange, const tuple<coord_type> &LowerCorner,
    const tuple<coord_type> &BinSize, const T &Region) {
    return ???;
  }
  // If maps to set
  template <typename IndexerType> static set<typename IndexerType::index_type> MapToBins(int
    NumDims, const range &BinRange, const IndexerType &BinIndexer, const tuple<coord_type>
    &LowerCorner, const tuple<coord_type> &BinSize, const T &Region) {
    return ???;
  }
*/
};

namespace is_hashable_region_internal {
template <typename T> using maybe_int = int;
template <typename T> constexpr std::true_type Test(maybe_int<typename
  hashable_region_traits<T>::coord_type>) {
  return {};
}
template <typename T> constexpr std::false_type Test(...) { return {}; }
}
template <typename T> constexpr bool IsHashableRegion() {
  return decltype(is_hashable_region_internal::Test<T>(0))::value;
}

}}

#endif
