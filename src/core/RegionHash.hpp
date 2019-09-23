// Copyright (c) 2019 Matthew J. Smith and Overkit contributors
// License: MIT (http://opensource.org/licenses/MIT)

#ifndef OVK_CORE_REGION_HASH_HPP_INCLUDED
#define OVK_CORE_REGION_HASH_HPP_INCLUDED

#include <ovk/core/Array.hpp>
#include <ovk/core/ArrayView.hpp>
#include <ovk/core/Box.hpp>
#include <ovk/core/Field.hpp>
#include <ovk/core/Global.hpp>
#include <ovk/core/Range.hpp>
#include <ovk/core/Tuple.hpp>

#include <cmath>
#include <memory>
#include <type_traits>
#include <utility>

namespace ovk {
namespace core {

template <typename CoordType> class region_hash {

public:

  static_assert(std::is_same<CoordType, int>::value || std::is_same<CoordType, double>::value,
    "Coord type must be int or double.");

  using coord_type = CoordType;
  using region_type = typename std::conditional<std::is_same<coord_type, double>::value, box,
    range>::type;
  using traits = region_traits<region_type>;

  explicit region_hash(int NumDims);
  region_hash(int NumDims, const tuple<int> &NumBins, array_view<const region_type> Regions);

  long long MapToBin(const tuple<coord_type> &Point) const;

  array_view<const long long> RetrieveBin(long long iBin) const;

  int Dimension() const { return NumDims_; }

  const range &BinRange() const { return BinRange_; }

  const region_type &Extents() const { return Extents_; }

private:

  int NumDims_;
  range BinRange_;
  field_indexer BinIndexer_;
  region_type Extents_;
  tuple<double> BinSize_;
  array<long long> BinRegionIndicesStarts_;
  array<long long> BinRegionIndices_;

  static tuple<int> GetBinSize_(const range &Extents, const tuple<int> &NumBins);
  static tuple<double> GetBinSize_(const box &Extents, const tuple<int> &NumBins);
  static tuple<int> MapToUniformCell_(int NumDims, const tuple<int> &Origin, const tuple<int>
    &CellSize, const tuple<int> &Point);
  static tuple<int> MapToUniformCell_(int NumDims, const tuple<double> &Origin, const tuple<double>
    &CellSize, const tuple<double> &Point);

};

}}

#include <ovk/core/RegionHash.inl>

#endif