// Copyright (c) 2018 Matthew J. Smith and Overkit contributors
// License: MIT (http://opensource.org/licenses/MIT)

#ifndef OVK_CORE_RANGE_BASE_H_INCLUDED
#define OVK_CORE_RANGE_BASE_H_INCLUDED

#include <ovk/core/ConstantsBase.h>
#include <ovk/core/GlobalBase.h>
#include <ovk/core/MinMax.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int NumDims;
  int Begin[OVK_MAX_DIMS];
  int End[OVK_MAX_DIMS];
} ovk_range;

static inline void ovkDefaultRange(ovk_range *Range, int NumDims);
static inline void ovkSetRange(ovk_range *Range, int NumDims, const int *Begin, const int *End);
static inline bool ovkRangeEquals(const ovk_range *LeftRange, const ovk_range *RightRange);
static inline void ovkRangeSize(const ovk_range *Range, int *Size);
static inline void ovkRangeCount(const ovk_range *Range, long long *Count);
static inline bool ovkRangeIsEmpty(const ovk_range *Range);
static inline void ovkRangeTupleToIndex(const ovk_range *Range, ovk_array_layout Layout,
  const int *Tuple, long long *Index);
// Note: Be careful with this; Tuple must have size OVK_MAX_DIMS
static inline void ovkRangeIndexToTuple(const ovk_range *Range, ovk_array_layout Layout,
  long long Index, int *Tuple);
static inline bool ovkRangeContains(const ovk_range *Range, const int *Point);
static inline bool ovkRangeIncludes(const ovk_range *LeftRange, const ovk_range *RightRange);
static inline bool ovkRangeOverlaps(const ovk_range *LeftRange, const ovk_range *RightRange);
static inline void ovkRangeUnion(const ovk_range *LeftRange, const ovk_range *RightRange,
  ovk_range *UnionRange);
static inline void ovkRangeIntersect(const ovk_range *LeftRange, const ovk_range *RightRange,
  ovk_range *IntersectRange);
static inline void ovkRangeClamp(const ovk_range *Range, int *Point);
static inline void ovkRangeExtend(const ovk_range *Range, const int *Point, ovk_range *ExtendRange);

#ifdef __cplusplus
}
#endif

#include <ovk/core/RangeBase.inl>

#endif
