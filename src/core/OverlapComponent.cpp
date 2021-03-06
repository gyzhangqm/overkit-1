// Copyright (c) 2020 Matthew J. Smith and Overkit contributors
// License: MIT (http://opensource.org/licenses/MIT)

#include "ovk/core/OverlapComponent.hpp"

#include "ovk/core/Array.hpp"
#include "ovk/core/ArrayView.hpp"
#include "ovk/core/Comm.hpp"
#include "ovk/core/DataType.hpp"
#include "ovk/core/Debug.hpp"
#include "ovk/core/DomainBase.hpp"
#include "ovk/core/Elem.hpp"
#include "ovk/core/ElemMap.hpp"
#include "ovk/core/ElemSet.hpp"
#include "ovk/core/Editor.hpp"
#include "ovk/core/FloatingRef.hpp"
#include "ovk/core/Global.hpp"
#include "ovk/core/Grid.hpp"
#include "ovk/core/Map.hpp"
#include "ovk/core/OverlapM.hpp"
#include "ovk/core/OverlapN.hpp"
#include "ovk/core/Set.hpp"
#include "ovk/core/TextProcessing.hpp"

#include <mpi.h>

#include <algorithm>
#include <string>
#include <utility>

namespace ovk {

namespace overlap_component_internal {

overlap_component_base::overlap_component_base(const core::domain_base &Domain, std::string &&Name):
  Context_(Domain.Context().GetFloatingRef()),
  Domain_(Domain.GetFloatingRef()),
  Name_(std::move(Name))
{
  MPI_Barrier(Domain.Comm());
}

overlap_component_base::~overlap_component_base() noexcept {

  if (Context_) {
    const core::domain_base &Domain = *Domain_;
    MPI_Barrier(Domain.Comm());
    core::logger &Logger = Context_->core_Logger();
    Logger.LogStatus(Domain.Comm().Rank() == 0, "Destroyed overlap component %s.%s.", Domain.Name(),
      *Name_);
  }

}

}

overlap_component::overlap_component(const core::domain_base &Domain, params Params):
  overlap_component_base(Domain, std::move(*Params.Name_))
{

  floating_ref<overlap_component> FloatingRef = FloatingRefGenerator_.Generate(*this);

  GridEventListener_ = Domain.AddGridEventListener([FloatingRef](int GridID, grid_event_flags Flags,
    bool LastInSequence) {
    overlap_component &OverlapComponent = *FloatingRef;
    grid_event_flags &AccumulatedFlags = OverlapComponent.GridEventFlags_.Fetch(GridID,
      grid_event_flags::NONE);
    AccumulatedFlags |= Flags;
    if (LastInSequence) {
      OverlapComponent.OnGridEvent_();
    }
  });

  core::logger &Logger = Context_->core_Logger();
  Logger.LogStatus(Domain.Comm().Rank() == 0, "Created overlap component %s.%s.", Domain.Name(),
    *Name_);

}

void overlap_component::OnGridEvent_() {

  DestroyOverlapsForDyingGrids_();

  GridEventFlags_.Clear();

}

void overlap_component::DestroyOverlapsForDyingGrids_() {

  set<int> DyingGridIDs;

  for (auto &EventEntry : GridEventFlags_) {
    int GridID = EventEntry.Key();
    grid_event_flags EventFlags = EventEntry.Value();
    if ((EventFlags & grid_event_flags::DESTROY) != grid_event_flags::NONE) {
      DyingGridIDs.Insert(GridID);
    }
  }

  elem_set<int,2> DyingOverlapIDs;

  for (auto &OverlapID : OverlapRecords_.Keys()) {
    int MGridID = OverlapID(0);
    int NGridID = OverlapID(1);
    if (DyingGridIDs.Contains(MGridID) || DyingGridIDs.Contains(NGridID)) {
      DyingOverlapIDs.Insert(OverlapID);
    }
  }

  DestroyOverlaps(DyingOverlapIDs);

}

void overlap_component::StartEdit() {

  // Nothing to do here

}

void overlap_component::EndEdit() {

  SyncEdits_();

}

void overlap_component::SyncEdits_() {

  const core::domain_base &Domain = *Domain_;

  int HasEdits = false;
  for (auto &LocalMEntry : LocalMs_) {
    local_m &LocalM = LocalMEntry.Value();
    if (LocalM.EventFlags != overlap_event_flags::NONE) {
      HasEdits = true;
      goto done_looping;
    }
  }
  for (auto &LocalNEntry : LocalNs_) {
    local_n &LocalN = LocalNEntry.Value();
    if (LocalN.EventFlags != overlap_event_flags::NONE) {
      HasEdits = true;
      goto done_looping;
    }
  }
  done_looping:;
  MPI_Allreduce(MPI_IN_PLACE, &HasEdits, 1, MPI_INT, MPI_MAX, Domain.Comm());

  if (!HasEdits) return;

  int NumOverlaps = OverlapRecords_.Count();

  elem_map<int,2,int> GridIDsToIndex;

  int NextIndex = 0;
  for (auto &OverlapID : OverlapRecords_.Keys()) {
    GridIDsToIndex.Insert({OverlapID,NextIndex});
    ++NextIndex;
  }

  array<overlap_event_flags> AllOverlapEventFlags({NumOverlaps},
    overlap_event_flags::NONE);

  for (auto &LocalMEntry : LocalMs_) {
    local_m &LocalM = LocalMEntry.Value();
    int iOverlap = GridIDsToIndex(LocalMEntry.Key());
    AllOverlapEventFlags(iOverlap) |= LocalM.EventFlags;
  }

  for (auto &LocalNEntry : LocalNs_) {
    local_n &LocalN = LocalNEntry.Value();
    int iOverlap = GridIDsToIndex(LocalNEntry.Key());
    AllOverlapEventFlags(iOverlap) |= LocalN.EventFlags;
  }

  MPI_Allreduce(MPI_IN_PLACE, AllOverlapEventFlags.Data(), NumOverlaps, MPI_INT, MPI_BOR,
    Domain.Comm());

  int NumTriggers = 0;
  for (auto EventFlags : AllOverlapEventFlags) {
    if (EventFlags != overlap_event_flags::NONE) ++NumTriggers;
  }
  for (auto &OverlapID : OverlapRecords_.Keys()) {
    int iOverlap = GridIDsToIndex(OverlapID);
    overlap_event_flags EventFlags = AllOverlapEventFlags(iOverlap);
    if (EventFlags != overlap_event_flags::NONE) {
      OverlapEvent_.Trigger(OverlapID, EventFlags, --NumTriggers == 0);
    }
  }

  MPI_Barrier(Domain.Comm());

  for (auto &LocalMEntry : LocalMs_) {
    local_m &LocalM = LocalMEntry.Value();
    LocalM.EventFlags = overlap_event_flags::NONE;
  }

  for (auto &LocalNEntry : LocalNs_) {
    local_n &LocalN = LocalNEntry.Value();
    LocalN.EventFlags = overlap_event_flags::NONE;
  }

}

bool overlap_component::OverlapExists(const elem<int,2> &OverlapID) const {

  const core::domain_base &Domain = *Domain_;

  if (OVK_DEBUG) {
    int MGridID = OverlapID(0);
    int NGridID = OverlapID(1);
    OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
    OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
    OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
    OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
  }

  return OverlapRecords_.Contains(OverlapID);

}

void overlap_component::CreateOverlap(const elem<int,2> &OverlapID) {

  const core::domain_base &Domain = *Domain_;

  MPI_Barrier(Domain.Comm());

  int MGridID = OverlapID(0);
  int NGridID = OverlapID(1);

  OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
  OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
  OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
  OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
  OVK_DEBUG_ASSERT(!OverlapExists(OverlapID), "Overlap (%i,%i) already exists.", MGridID, NGridID);

  SyncEdits_();

  core::logger &Logger = Context_->core_Logger();

  const grid_info &MGridInfo = Domain.GridInfo(MGridID);
  const grid_info &NGridInfo = Domain.GridInfo(NGridID);

  Logger.LogStatus(Domain.Comm().Rank() == 0, "Creating overlap %s.(%s,%s)...", Domain.Name(),
    MGridInfo.Name(), NGridInfo.Name());
  auto Level1 = Logger.IncreaseStatusLevelAndIndent();

  const std::shared_ptr<context> &SharedContext = Domain.SharedContext();

  if (MGridInfo.IsLocal()) {
    const grid &MGrid = Domain.Grid(MGridID);
    overlap_m OverlapM = core::CreateOverlapM(SharedContext, MGrid, NGridInfo);
    LocalMs_.Insert(OverlapID, std::move(OverlapM));
  }

  if (NGridInfo.IsLocal()) {
    const grid &NGrid = Domain.Grid(NGridID);
    overlap_n OverlapN = core::CreateOverlapN(SharedContext, NGrid, MGridInfo);
    LocalNs_.Insert(OverlapID, std::move(OverlapN));
  }

  OverlapRecords_.Insert(OverlapID);

  MPI_Barrier(Domain.Comm());

  Level1.Reset();
  Logger.LogStatus(Domain.Comm().Rank() == 0, "Done creating overlap %s.(%s,%s).", Domain.Name(),
    MGridInfo.Name(), NGridInfo.Name());

  OverlapEvent_.Trigger(OverlapID, overlap_event_flags::CREATE, true);

  MPI_Barrier(Domain.Comm());

}

void overlap_component::CreateOverlaps(array_view<const elem<int,2>> OverlapIDs) {

  const core::domain_base &Domain = *Domain_;

  MPI_Barrier(Domain.Comm());

  if (OVK_DEBUG) {
    for (auto &OverlapID : OverlapIDs) {
      int MGridID = OverlapID(0);
      int NGridID = OverlapID(1);
      OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
      OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
      OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
      OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
      OVK_DEBUG_ASSERT(!OverlapExists(OverlapID), "Overlap (%i,%i) already exists.", MGridID,
        NGridID);
    }
  }

  SyncEdits_();

  core::logger &Logger = Context_->core_Logger();

  if (Logger.LoggingStatus()) {
    for (auto &OverlapID : OverlapIDs) {
      int MGridID = OverlapID(0);
      int NGridID = OverlapID(1);
      const grid_info &MGridInfo = Domain.GridInfo(MGridID);
      const grid_info &NGridInfo = Domain.GridInfo(NGridID);
      Logger.LogStatus(Domain.Comm().Rank() == 0, "Creating overlap %s.(%s,%s)...", Domain.Name(),
        MGridInfo.Name(), NGridInfo.Name());
    }
  }
  auto Level1 = Logger.IncreaseStatusLevelAndIndent();

  const std::shared_ptr<context> &SharedContext = Domain.SharedContext();

  for (auto &OverlapID : OverlapIDs) {
    int MGridID = OverlapID(0);
    int NGridID = OverlapID(1);
    const grid_info &MGridInfo = Domain.GridInfo(MGridID);
    const grid_info &NGridInfo = Domain.GridInfo(NGridID);
    if (MGridInfo.IsLocal()) {
      const grid &MGrid = Domain.Grid(MGridID);
      overlap_m OverlapM = core::CreateOverlapM(SharedContext, MGrid, NGridInfo);
      LocalMs_.Insert(OverlapID, std::move(OverlapM));
    }
    if (NGridInfo.IsLocal()) {
      const grid &NGrid = Domain.Grid(NGridID);
      overlap_n OverlapN = core::CreateOverlapN(SharedContext, NGrid, MGridInfo);
      LocalNs_.Insert(OverlapID, std::move(OverlapN));
    }
  }

  for (auto &OverlapID : OverlapIDs) {
    OverlapRecords_.Insert(OverlapID);
  }

  MPI_Barrier(Domain.Comm());

  Level1.Reset();
  if (Logger.LoggingStatus()) {
    for (auto &OverlapID : OverlapIDs) {
      int MGridID = OverlapID(0);
      int NGridID = OverlapID(1);
      const grid_info &MGridInfo = Domain.GridInfo(MGridID);
      const grid_info &NGridInfo = Domain.GridInfo(NGridID);
      Logger.LogStatus(Domain.Comm().Rank() == 0, "Done creating overlap %s.(%s,%s).",
        Domain.Name(), MGridInfo.Name(), NGridInfo.Name());
    }
  }

  int NumRemaining = OverlapIDs.Count();
  for (auto &OverlapID : OverlapIDs) {
    OverlapEvent_.Trigger(OverlapID, overlap_event_flags::CREATE, --NumRemaining == 0);
  }

  MPI_Barrier(Domain.Comm());

}

void overlap_component::DestroyOverlap(const elem<int,2> &OverlapID) {

  const core::domain_base &Domain = *Domain_;

  MPI_Barrier(Domain.Comm());

  int MGridID = OverlapID(0);
  int NGridID = OverlapID(1);

  OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
  OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
  OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
  OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
  OVK_DEBUG_ASSERT(OverlapExists(OverlapID), "Overlap (%i,%i) does not exist.", MGridID, NGridID);

  if (OVK_DEBUG) {
    const grid_info &MGridInfo = Domain.GridInfo(MGridID);
    const grid_info &NGridInfo = Domain.GridInfo(NGridID);
    int Editing = 0;
    if (MGridInfo.IsLocal()) {
      Editing = Editing || LocalMs_(OverlapID).Editor.Active();
    }
    if (NGridInfo.IsLocal()) {
      Editing = Editing || LocalNs_(OverlapID).Editor.Active();
    }
    MPI_Allreduce(MPI_IN_PLACE, &Editing, 1, MPI_INT, MPI_LOR, Domain.Comm());
    OVK_DEBUG_ASSERT(!Editing, "Cannot destroy overlap %s.(%s,%s); still being edited.",
      Domain.Name(), MGridInfo.Name(), NGridInfo.Name());
  }

  SyncEdits_();

  OverlapEvent_.Trigger(OverlapID, overlap_event_flags::DESTROY, true);

  MPI_Barrier(Domain.Comm());

  core::logger &Logger = Context_->core_Logger();

  const grid_info &MGridInfo = Domain.GridInfo(MGridID);
  const grid_info &NGridInfo = Domain.GridInfo(NGridID);

  Logger.LogStatus(Domain.Comm().Rank() == 0, "Destroying overlap %s.(%s,%s)...", Domain.Name(),
    MGridInfo.Name(), NGridInfo.Name());
  auto Level1 = Logger.IncreaseStatusLevelAndIndent();

  LocalMs_.Erase(OverlapID);
  LocalNs_.Erase(OverlapID);

  OverlapRecords_.Erase(OverlapID);

  MPI_Barrier(Domain.Comm());

  Level1.Reset();
  Logger.LogStatus(Domain.Comm().Rank() == 0, "Done destroying overlap %s.(%s,%s).", Domain.Name(),
    MGridInfo.Name(), NGridInfo.Name());

}

void overlap_component::DestroyOverlaps(array_view<const elem<int,2>> OverlapIDs) {

  const core::domain_base &Domain = *Domain_;

  MPI_Barrier(Domain.Comm());

  int NumDestroys = OverlapIDs.Count();

  if (OVK_DEBUG) {
    for (auto &OverlapID : OverlapIDs) {
      int MGridID = OverlapID(0);
      int NGridID = OverlapID(1);
      OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
      OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
      OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
      OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
      OVK_DEBUG_ASSERT(OverlapExists(OverlapID), "Overlap (%i,%i) does not exist.", MGridID,
        NGridID);
    }
  }

  if (OVK_DEBUG) {
    array<int> Editing({NumDestroys}, 0);
    for (int iDestroy = 0; iDestroy < NumDestroys; ++iDestroy) {
      auto &OverlapID = OverlapIDs(iDestroy);
      int MGridID = OverlapID(0);
      int NGridID = OverlapID(1);
      if (Domain.GridIsLocal(MGridID)) {
        Editing(iDestroy) = Editing(iDestroy) || LocalMs_(OverlapID).Editor.Active();
      }
      if (Domain.GridIsLocal(NGridID)) {
        Editing(iDestroy) = Editing(iDestroy) || LocalNs_(OverlapID).Editor.Active();
      }
    }
    MPI_Allreduce(MPI_IN_PLACE, Editing.Data(), NumDestroys, MPI_INT, MPI_LOR, Domain.Comm());
    for (int iDestroy = 0; iDestroy < NumDestroys; ++iDestroy) {
      auto &OverlapID = OverlapIDs(iDestroy);
      int MGridID = OverlapID(0);
      int NGridID = OverlapID(1);
      const grid_info &MGridInfo = Domain.GridInfo(MGridID);
      const grid_info &NGridInfo = Domain.GridInfo(NGridID);
      OVK_DEBUG_ASSERT(!Editing(iDestroy), "Cannot destroy overlap %s.(%s,%s); still being edited.",
        Domain.Name(), MGridInfo.Name(), NGridInfo.Name());
    }
  }

  SyncEdits_();

  int NumRemaining = NumDestroys;
  for (auto &OverlapID : OverlapIDs) {
    OverlapEvent_.Trigger(OverlapID, overlap_event_flags::DESTROY, --NumRemaining == 0);
  }

  MPI_Barrier(Domain.Comm());

  core::logger &Logger = Context_->core_Logger();

  if (Logger.LoggingStatus()) {
    for (auto &OverlapID : OverlapIDs) {
      int MGridID = OverlapID(0);
      int NGridID = OverlapID(1);
      const grid_info &MGridInfo = Domain.GridInfo(MGridID);
      const grid_info &NGridInfo = Domain.GridInfo(NGridID);
      Logger.LogStatus(Domain.Comm().Rank() == 0, "Destroying overlap %s.(%s,%s)...", Domain.Name(),
        MGridInfo.Name(), NGridInfo.Name());
    }
  }
  auto Level1 = Logger.IncreaseStatusLevelAndIndent();

  for (auto &OverlapID : OverlapIDs) {
    LocalMs_.Erase(OverlapID);
    LocalNs_.Erase(OverlapID);
  }

  for (auto &OverlapID : OverlapIDs) {
    OverlapRecords_.Erase(OverlapID);
  }

  MPI_Barrier(Domain.Comm());

  Level1.Reset();
  if (Logger.LoggingStatus()) {
    for (auto &OverlapID : OverlapIDs) {
      int MGridID = OverlapID(0);
      int NGridID = OverlapID(1);
      const grid_info &MGridInfo = Domain.GridInfo(MGridID);
      const grid_info &NGridInfo = Domain.GridInfo(NGridID);
      Logger.LogStatus(Domain.Comm().Rank() == 0, "Done destroying overlap %s.(%s,%s).",
        Domain.Name(), MGridInfo.Name(), NGridInfo.Name());
    }
  }

}

void overlap_component::ClearOverlaps() {

  DestroyOverlaps(OverlapRecords_.Keys());

}

const overlap_m &overlap_component::OverlapM(const elem<int,2> &OverlapID) const {

  const core::domain_base &Domain = *Domain_;

  if (OVK_DEBUG) {
    int MGridID = OverlapID(0);
    int NGridID = OverlapID(1);
    OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
    OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
    OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
    OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
    OVK_DEBUG_ASSERT(OverlapExists(OverlapID), "Overlap (%i,%i) does not exist.", MGridID,
      NGridID);
    const grid_info &GridInfo = Domain.GridInfo(MGridID);
    OVK_DEBUG_ASSERT(GridInfo.IsLocal(), "M grid %s is not local to rank @rank@.", GridInfo.Name());
  }

  return LocalMs_(OverlapID).Overlap;

}

bool overlap_component::EditingOverlapM(const elem<int,2> &OverlapID) const {

  const core::domain_base &Domain = *Domain_;

  if (OVK_DEBUG) {
    int MGridID = OverlapID(0);
    int NGridID = OverlapID(1);
    OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
    OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
    OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
    OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
    OVK_DEBUG_ASSERT(OverlapExists(OverlapID), "Overlap (%i,%i) does not exist.", MGridID,
      NGridID);
    const grid_info &GridInfo = Domain.GridInfo(MGridID);
    OVK_DEBUG_ASSERT(GridInfo.IsLocal(), "M grid %s is not local to rank @rank@.", GridInfo.Name());
  }

  const local_m &LocalM = LocalMs_(OverlapID);
  const editor &Editor = LocalM.Editor;

  return Editor.Active();

}

edit_handle<overlap_m> overlap_component::EditOverlapM(const elem<int,2> &OverlapID) {

  const core::domain_base &Domain = *Domain_;

  int MGridID = OverlapID(0);
  int NGridID = OverlapID(1);

  OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
  OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
  OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
  OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
  OVK_DEBUG_ASSERT(OverlapExists(OverlapID), "Overlap (%i,%i) does not exist.", MGridID, NGridID);
  if (OVK_DEBUG) {
    const grid_info &GridInfo = Domain.GridInfo(MGridID);
    OVK_DEBUG_ASSERT(GridInfo.IsLocal(), "M grid %s is not local to rank @rank@.", GridInfo.Name());
  }

  local_m &LocalM = LocalMs_(OverlapID);
  overlap_m &OverlapM = LocalM.Overlap;
  editor &Editor = LocalM.Editor;

  if (!Editor.Active()) {
    floating_ref<const grid> GridRef = Domain.Grid(MGridID).GetFloatingRef();
    MPI_Barrier(GridRef->Comm());
    auto DeactivateFunc = [GridRef] { MPI_Barrier(GridRef->Comm()); };
    Editor.Activate(std::move(DeactivateFunc));
  }

  return Editor.Edit(OverlapM);

}

void overlap_component::RestoreOverlapM(const elem<int,2> &OverlapID) {

  const core::domain_base &Domain = *Domain_;

  int MGridID = OverlapID(0);
  int NGridID = OverlapID(1);

  OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
  OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
  OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
  OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
  OVK_DEBUG_ASSERT(OverlapExists(OverlapID), "Overlap (%i,%i) does not exist.", MGridID, NGridID);
  if (OVK_DEBUG) {
    const grid_info &GridInfo = Domain.GridInfo(MGridID);
    OVK_DEBUG_ASSERT(GridInfo.IsLocal(), "M grid %s is not local to rank @rank@.", GridInfo.Name());
  }

  local_m &LocalM = LocalMs_(OverlapID);
  editor &Editor = LocalM.Editor;

  if (OVK_DEBUG) {
    const grid_info &MGridInfo = Domain.GridInfo(MGridID);
    const grid_info &NGridInfo = Domain.GridInfo(NGridID);
    OVK_DEBUG_ASSERT(Editor.Active(), "Unable to restore overlap M %s.(%s,%s); not currently "
      "being edited.", Domain.Name(), MGridInfo.Name(), NGridInfo.Name());
  }

  Editor.Restore();

}

const overlap_n &overlap_component::OverlapN(const elem<int,2> &OverlapID) const {

  const core::domain_base &Domain = *Domain_;

  if (OVK_DEBUG) {
    int MGridID = OverlapID(0);
    int NGridID = OverlapID(1);
    OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
    OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
    OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
    OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
    OVK_DEBUG_ASSERT(OverlapExists(OverlapID), "Overlap (%i,%i) does not exist.", MGridID,
      NGridID);
    const grid_info &GridInfo = Domain.GridInfo(NGridID);
    OVK_DEBUG_ASSERT(GridInfo.IsLocal(), "N grid %s is not local to rank @rank@.", GridInfo.Name());
  }

  return LocalNs_(OverlapID).Overlap;

}

bool overlap_component::EditingOverlapN(const elem<int,2> &OverlapID) const {

  const core::domain_base &Domain = *Domain_;

  if (OVK_DEBUG) {
    int MGridID = OverlapID(0);
    int NGridID = OverlapID(1);
    OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
    OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
    OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
    OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
    OVK_DEBUG_ASSERT(OverlapExists(OverlapID), "Overlap (%i,%i) does not exist.", MGridID,
      NGridID);
    const grid_info &GridInfo = Domain.GridInfo(NGridID);
    OVK_DEBUG_ASSERT(GridInfo.IsLocal(), "N grid %s is not local to rank @rank@.", GridInfo.Name());
  }

  const local_n &LocalN = LocalNs_(OverlapID);
  const editor &Editor = LocalN.Editor;

  return Editor.Active();

}

edit_handle<overlap_n> overlap_component::EditOverlapN(const elem<int,2> &OverlapID) {

  const core::domain_base &Domain = *Domain_;

  int MGridID = OverlapID(0);
  int NGridID = OverlapID(1);

  OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
  OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
  OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
  OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
  OVK_DEBUG_ASSERT(OverlapExists(OverlapID), "Overlap (%i,%i) does not exist.", MGridID, NGridID);
  if (OVK_DEBUG) {
    const grid_info &GridInfo = Domain.GridInfo(NGridID);
    OVK_DEBUG_ASSERT(GridInfo.IsLocal(), "N grid %s is not local to rank @rank@.", GridInfo.Name());
  }

  local_n &LocalN = LocalNs_(OverlapID);
  overlap_n &OverlapN = LocalN.Overlap;
  editor &Editor = LocalN.Editor;

  if (!Editor.Active()) {
    floating_ref<const grid> GridRef = Domain.Grid(NGridID).GetFloatingRef();
    MPI_Barrier(GridRef->Comm());
    auto DeactivateFunc = [GridRef] { MPI_Barrier(GridRef->Comm()); };
    Editor.Activate(std::move(DeactivateFunc));
  }

  return Editor.Edit(OverlapN);

}

void overlap_component::RestoreOverlapN(const elem<int,2> &OverlapID) {

  const core::domain_base &Domain = *Domain_;

  int MGridID = OverlapID(0);
  int NGridID = OverlapID(1);

  OVK_DEBUG_ASSERT(MGridID >= 0, "Invalid M grid ID.");
  OVK_DEBUG_ASSERT(NGridID >= 0, "Invalid N grid ID.");
  OVK_DEBUG_ASSERT(Domain.GridExists(MGridID), "Grid %i does not exist.", MGridID);
  OVK_DEBUG_ASSERT(Domain.GridExists(NGridID), "Grid %i does not exist.", NGridID);
  OVK_DEBUG_ASSERT(OverlapExists(OverlapID), "Overlap (%i,%i) does not exist.", MGridID, NGridID);
  if (OVK_DEBUG) {
    const grid_info &GridInfo = Domain.GridInfo(NGridID);
    OVK_DEBUG_ASSERT(GridInfo.IsLocal(), "N grid %s is not local to rank @rank@.", GridInfo.Name());
  }

  local_n &LocalN = LocalNs_(OverlapID);
  editor &Editor = LocalN.Editor;

  if (OVK_DEBUG) {
    const grid_info &MGridInfo = Domain.GridInfo(MGridID);
    const grid_info &NGridInfo = Domain.GridInfo(NGridID);
    OVK_DEBUG_ASSERT(Editor.Active(), "Unable to restore overlap N %s.(%s,%s); not currently "
      "being edited.", Domain.Name(), MGridInfo.Name(), NGridInfo.Name());
  }

  Editor.Restore();

}

overlap_component::local_m::local_m(overlap_m Overlap_):
  Overlap(std::move(Overlap_)),
  EventFlags(overlap_event_flags::NONE)
{

  floating_ref<overlap_event_flags> EventFlagsRef = FloatingRefGenerator.Generate(EventFlags);

  ResizeEventListener = Overlap.AddResizeEventListener([EventFlagsRef] {
    *EventFlagsRef |= overlap_event_flags::RESIZE_M;
  });

  CellsEventListener = Overlap.AddCellsEventListener([EventFlagsRef] {
    *EventFlagsRef |= overlap_event_flags::EDIT_M_CELLS;
  });

  CoordsEventListener = Overlap.AddCoordsEventListener([EventFlagsRef] {
    *EventFlagsRef |= overlap_event_flags::EDIT_M_COORDS;
  });

  DestinationsEventListener = Overlap.AddDestinationsEventListener([EventFlagsRef] {
    *EventFlagsRef |= overlap_event_flags::EDIT_M_DESTINATIONS;
  });

  DestinationRanksEventListener = Overlap.AddDestinationRanksEventListener([EventFlagsRef] {
    *EventFlagsRef |= overlap_event_flags::EDIT_M_DESTINATIONS;
  });

}

overlap_component::local_n::local_n(overlap_n Overlap_):
  Overlap(std::move(Overlap_)),
  EventFlags(overlap_event_flags::NONE)
{

  floating_ref<overlap_event_flags> EventFlagsRef = FloatingRefGenerator.Generate(EventFlags);

  ResizeEventListener = Overlap.AddResizeEventListener([EventFlagsRef] {
    *EventFlagsRef |= overlap_event_flags::RESIZE_N;
  });

  PointsEventListener = Overlap.AddPointsEventListener([EventFlagsRef] {
    *EventFlagsRef |= overlap_event_flags::EDIT_N_POINTS;
  });

  SourcesEventListener = Overlap.AddSourcesEventListener([EventFlagsRef] {
    *EventFlagsRef |= overlap_event_flags::EDIT_N_SOURCES;
  });

  SourceRanksEventListener = Overlap.AddSourceRanksEventListener([EventFlagsRef] {
    *EventFlagsRef |= overlap_event_flags::EDIT_N_SOURCES;
  });

}

overlap_component::params &overlap_component::params::SetName(std::string Name) {

  Name_ = std::move(Name);

  return *this;

}

}
