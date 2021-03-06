// Copyright (c) 2020 Matthew J. Smith and Overkit contributors
// License: MIT (http://opensource.org/licenses/MIT)

#ifndef OVK_EXAMPLES_COMMON_HPP_LOADED
#define OVK_EXAMPLES_COMMON_HPP_LOADED

#include <support/CommandArgs.hpp>
#include <support/Constants.hpp>
#include <support/Decomp.hpp>
#include <support/XDMF.hpp>

#include <ovk/core/ArrayView.hpp>
#include <ovk/core/Cart.hpp>
#include <ovk/core/Comm.hpp>
#include <ovk/core/Range.hpp>
#include <ovk/core/Tuple.hpp>

#include <mpi.h>

#include <array>

namespace examples {

constexpr double PI = support::PI;

using support::command_args;
using support::command_args_parser;
using support::command_args_error;
using support::command_args_error_code;

void DecomposeDomain(ovk::array_view<const long long> NumPointsPerGrid, int NumProcs,
  ovk::array_view<int> GridProcRanges) {

  int NumGrids = NumPointsPerGrid.Count();

  support::DecomposeDomain(NumPointsPerGrid, NumProcs, {GridProcRanges.Data(), {{NumGrids,2}}});

}

std::array<int,3> CreateCartesianDecompDims(int Size, int NumDims, const std::array<int,3>
  &InputDims) {

  ovk::tuple<int> DimsTuple = support::CreateCartesianDecompDims(Size, NumDims, InputDims);

  return {{DimsTuple(0), DimsTuple(1), DimsTuple(2)}};

}

std::array<int,6> CartesianDecomp(int NumDims, const std::array<int,3> &Size, MPI_Comm CartComm) {

  ovk::range LocalRange = support::CartesianDecomp(NumDims, {Size}, CartComm);

  return {{LocalRange.Begin(0), LocalRange.Begin(1), LocalRange.Begin(2), LocalRange.End(0),
    LocalRange.End(1), LocalRange.End(2)}};

}

#ifdef OVK_HAVE_XDMF
using support::xdmf;
using support::xdmf_grid_meta;
using support::xdmf_attribute_meta;
using support::xdmf_attribute_type;
using support::xdmf_error;
using support::xdmf_error_code;
using support::CreateXDMF;
using support::OpenXDMF;
#endif

}

#endif
