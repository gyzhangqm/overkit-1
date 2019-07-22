// Copyright (c) 2019 Matthew J. Smith and Overkit contributors
// License: MIT (http://opensource.org/licenses/MIT)

#ifndef OVK_CORE_OVERLAP_COMPONENT_HPP_INCLUDED
#define OVK_CORE_OVERLAP_COMPONENT_HPP_INCLUDED

#include <ovk/core/ArrayView.hpp>
#include <ovk/core/Comm.hpp>
#include <ovk/core/Context.hpp>
#include <ovk/core/DomainBase.hpp>
#include <ovk/core/Event.hpp>
#include <ovk/core/FloatingRef.hpp>
#include <ovk/core/Global.hpp>
#include <ovk/core/Grid.hpp>
#include <ovk/core/IDMap.hpp>
#include <ovk/core/IDSet.hpp>
#include <ovk/core/OverlapComponent.h>
#include <ovk/core/OverlapM.hpp>
#include <ovk/core/OverlapN.hpp>
#include <ovk/core/Requires.hpp>
#include <ovk/core/StringWrapper.hpp>
#include <ovk/core/TypeTraits.hpp>

#include <mpi.h>

#include <functional>
#include <memory>
#include <string>

namespace ovk {

enum class overlap_event_flags : int {
  NONE = OVK_OVERLAP_EVENT_FLAGS_NONE,
  CREATE = OVK_OVERLAP_EVENT_FLAGS_CREATE,
  DESTROY = OVK_OVERLAP_EVENT_FLAGS_DESTROY,
  RESIZE_M = OVK_OVERLAP_EVENT_FLAGS_RESIZE_M,
  EDIT_M_CELLS = OVK_OVERLAP_EVENT_FLAGS_EDIT_M_CELLS,
  EDIT_M_COORDS = OVK_OVERLAP_EVENT_FLAGS_EDIT_M_COORDS,
  EDIT_M_DESTINATIONS = OVK_OVERLAP_EVENT_FLAGS_EDIT_M_DESTINATIONS,
  RESIZE_N = OVK_OVERLAP_EVENT_FLAGS_RESIZE_N,
  EDIT_N_POINTS = OVK_OVERLAP_EVENT_FLAGS_EDIT_N_POINTS,
  EDIT_N_SOURCES = OVK_OVERLAP_EVENT_FLAGS_EDIT_N_SOURCES,
  ALL_EDITS = OVK_OVERLAP_EVENT_FLAGS_ALL_EDITS,
  ALL = OVK_OVERLAP_EVENT_FLAGS_ALL
};

constexpr inline overlap_event_flags operator|(overlap_event_flags Left, overlap_event_flags Right)
  {
  return overlap_event_flags(int(Left) | int(Right));
}
constexpr inline overlap_event_flags operator&(overlap_event_flags Left, overlap_event_flags Right)
  {
  return overlap_event_flags(int(Left) & int(Right));
}
constexpr inline overlap_event_flags operator^(overlap_event_flags Left, overlap_event_flags Right)
  {
  return overlap_event_flags(int(Left) ^ int(Right));
}
constexpr inline overlap_event_flags operator~(overlap_event_flags EventFlags) {
  return overlap_event_flags(~int(EventFlags));
}
inline overlap_event_flags operator|=(overlap_event_flags &Left, overlap_event_flags Right) {
  return Left = Left | Right;
}
inline overlap_event_flags operator&=(overlap_event_flags &Left, overlap_event_flags Right) {
  return Left = Left & Right;
}
inline overlap_event_flags operator^=(overlap_event_flags &Left, overlap_event_flags Right) {
  return Left = Left ^ Right;
}

namespace overlap_component_internal {

// For doing stuff before creation and after destruction
class overlap_component_base {

public:

  overlap_component_base(const core::domain_base &Domain, std::string &&Name);

  overlap_component_base(const overlap_component_base &Other) = delete;
  overlap_component_base(overlap_component_base &&Other) noexcept = default;

  overlap_component_base &operator=(const overlap_component_base &Other) = delete;
  overlap_component_base &operator=(overlap_component_base &&Other) noexcept = default;

  ~overlap_component_base() noexcept;

  floating_ref<const context> Context_;
  floating_ref<const core::domain_base> Domain_;

  core::string_wrapper Name_;

};

}

class overlap_component : private overlap_component_internal::overlap_component_base {

public:

  class params {
  public:
    params() {}
    const std::string &Name() const { return Name_; }
    params &SetName(std::string Name);
  private:
    core::string_wrapper Name_ = "OverlapComponent";
    friend class overlap_component;
  };

  overlap_component(const core::domain_base &Domain, params Params={});

  overlap_component(const overlap_component &Other) = delete;
  overlap_component(overlap_component &&Other) noexcept = default;

  overlap_component &operator=(const overlap_component &Other) = delete;
  overlap_component &operator=(overlap_component &&Other) noexcept = default;

  floating_ref<const overlap_component> GetFloatingRef() const {
    return FloatingRefGenerator_.Generate(*this);
  }
  floating_ref<overlap_component> GetFloatingRef() {
    return FloatingRefGenerator_.Generate(*this);
  }

  const std::string &Name() const { return *Name_; }

  int OverlapCount() const;

  const id_set<2> &OverlapIDs() const;

  bool OverlapExists(int MGridID, int NGridID) const;

  void CreateOverlap(int MGridID, int NGridID);
  void CreateOverlaps(array_view<const int> MGridIDs, array_view<const int> NGridIDs);

  void DestroyOverlap(int MGridID, int NGridID);
  void DestroyOverlaps(array_view<const int> MGridIDs, array_view<const int> NGridIDs);

  void ClearOverlaps();

  int LocalOverlapMCount() const;
  int LocalOverlapMCountForGrid(int MGridID) const;

  const id_set<2> &LocalOverlapMIDs() const;

  const overlap_m &OverlapM(int MGridID, int NGridID) const;
  bool EditingOverlapM(int MGridID, int NGridID) const;
  edit_handle<overlap_m> EditOverlapM(int MGridID, int NGridID);
  void RestoreOverlapM(int MGridID, int NGridID);

  int LocalOverlapNCount() const;
  int LocalOverlapNCountForGrid(int NGridID) const;

  const id_set<2> &LocalOverlapNIDs() const;

  const overlap_n &OverlapN(int MGridID, int NGridID) const;
  bool EditingOverlapN(int MGridID, int NGridID) const;
  edit_handle<overlap_n> EditOverlapN(int MGridID, int NGridID);
  void RestoreOverlapN(int MGridID, int NGridID);

  void StartEdit();
  void EndEdit();

  template <typename F, OVK_FUNCTION_REQUIRES(core::IsCallableWith<F, int, int,
    overlap_event_flags, bool>())> event_listener_handle AddOverlapEventListener(F Listener) const {
    return OverlapEvent_.AddListener(std::move(Listener));
  }

private:

  struct overlap_record {};

  struct local_m {
    overlap_m Overlap;
    overlap_event_flags EventFlags;
    floating_ref_generator FloatingRefGenerator;
    event_listener_handle ResizeEventListener;
    event_listener_handle CellsEventListener;
    event_listener_handle CoordsEventListener;
    event_listener_handle DestinationsEventListener;
    event_listener_handle DestinationRanksEventListener;
    editor Editor;
    explicit local_m(overlap_m Overlap);
  };

  struct local_n {
    overlap_n Overlap;
    overlap_event_flags EventFlags;
    floating_ref_generator FloatingRefGenerator;
    event_listener_handle ResizeEventListener;
    event_listener_handle PointsEventListener;
    event_listener_handle SourcesEventListener;
    event_listener_handle SourceRanksEventListener;
    editor Editor;
    explicit local_n(overlap_n Overlap);
  };

  floating_ref_generator FloatingRefGenerator_;

  id_map<1,grid_event_flags> GridEventFlags_;
  event_listener_handle GridEventListener_;

  id_map<2,overlap_record> OverlapRecords_;
  id_map<2,local_m,false> LocalMs_;
  id_map<2,local_n,false> LocalNs_;

  mutable event<void(int, int, overlap_event_flags, bool)> OverlapEvent_;

  void OnGridEvent_();
  void DestroyOverlapsForDyingGrids_();

  void SyncEdits_();

};

}

#endif