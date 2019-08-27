// Copyright (c) 2019 Matthew J. Smith and Overkit contributors
// License: MIT (http://opensource.org/licenses/MIT)

namespace ovk {
namespace core {

namespace overlaps_cell_internal {

inline bool OverlapsCellUniform(int NumDims, const array<field<double>> &Coords, double Tolerance,
  const tuple<int> &Cell, const tuple<double> &PointCoords) {

  tuple<int> UpperCornerOffset = MakeUniformTuple<int>(NumDims, 1, 0);

  long long iLowerCorner = Coords(0).Indexer().ToIndex(Cell);
  long long iUpperCorner = Coords(0).Indexer().ToIndex(Cell+UpperCornerOffset);

  bool Overlaps;

  switch (NumDims) {
  case 1: {
    double LowerCornerCoord = Coords(0)[iLowerCorner];
    double UpperCornerCoord = Coords(0)[iUpperCorner];
    double PointCoord = PointCoords(0);
    Overlaps = core::OverlapsLine(LowerCornerCoord, UpperCornerCoord, PointCoord, Tolerance);
    break;
  }
  case 2: {
    elem<double,2> LowerCornerCoords = {
      Coords(0)[iLowerCorner],
      Coords(1)[iLowerCorner]
    };
    elem<double,2> UpperCornerCoords = {
      Coords(0)[iUpperCorner],
      Coords(1)[iUpperCorner]
    };
    elem<double,2> PointCoords_ = {PointCoords(0), PointCoords(1)};
    Overlaps = core::OverlapsQuadUniform(LowerCornerCoords, UpperCornerCoords, PointCoords_,
      Tolerance);
    break;
  }
  default: {
    tuple<double> LowerCornerCoords = {
      Coords(0)[iLowerCorner],
      Coords(1)[iLowerCorner],
      Coords(2)[iLowerCorner]
    };
    tuple<double> UpperCornerCoords = {
      Coords(0)[iUpperCorner],
      Coords(1)[iUpperCorner],
      Coords(2)[iUpperCorner]
    };
    Overlaps = core::OverlapsHexUniform(LowerCornerCoords, UpperCornerCoords, PointCoords,
      Tolerance);
    break;
  }}

  return Overlaps;

}

inline bool OverlapsCellOrientedUniform(int NumDims, const array<field<double>> &Coords, double
  Tolerance, const tuple<int> &Cell, const tuple<double> &PointCoords) {

  bool Overlaps;

  switch (NumDims) {
  case 2: {
    elem<double,2> NodeCoords[4];
    int iNode = 0;
    for (int j = Cell(1); j <= Cell(1)+1; ++j) {
      for (int i = Cell(0); i <= Cell(0)+1; ++i) {
        long long iPoint = Coords(0).Indexer().ToIndex(i,j,0);
        NodeCoords[iNode] = {
          Coords(0)[iPoint],
          Coords(1)[iPoint]
        };
        ++iNode;
      }
    }
    elem<double,2> PointCoords_ = {PointCoords(0), PointCoords(1)};
    Overlaps = core::OverlapsQuadOrientedUniform(NodeCoords, PointCoords_, Tolerance);
    break;
  }
  default: {
    tuple<double> NodeCoords[8];
    int iNode = 0;
    for (int k = Cell(2); k <= Cell(2)+1; ++k) {
      for (int j = Cell(1); j <= Cell(1)+1; ++j) {
        for (int i = Cell(0); i <= Cell(0)+1; ++i) {
          long long iPoint = Coords(0).Indexer().ToIndex(i,j,k);
          NodeCoords[iNode] = {
            Coords(0)[iPoint],
            Coords(1)[iPoint],
            Coords(2)[iPoint]
          };
          ++iNode;
        }
      }
    }
    Overlaps = core::OverlapsHexOrientedUniform(NodeCoords, PointCoords, Tolerance);
    break;
  }}

  return Overlaps;

}

inline bool OverlapsCellNonUniform(int NumDims, const array<field<double>> &Coords, double
  Tolerance, const tuple<int> &Cell, const tuple<double> &PointCoords) {

  bool Overlaps;

  switch (NumDims) {
  case 2: {
    elem<double,2> NodeCoords[4];
    int iNode = 0;
    for (int j = Cell(1); j <= Cell(1)+1; ++j) {
      for (int i = Cell(0); i <= Cell(0)+1; ++i) {
        long long iPoint = Coords(0).Indexer().ToIndex(i,j,0);
        NodeCoords[iNode] = {
          Coords(0)[iPoint],
          Coords(1)[iPoint]
        };
        ++iNode;
      }
    }
    elem<double,2> PointCoords_ = {PointCoords(0), PointCoords(1)};
    Overlaps = core::OverlapsQuadNonUniform(NodeCoords, PointCoords_, Tolerance);
    break;
  }
  default: {
    tuple<double> NodeCoords[8];
    int iNode = 0;
    for (int k = Cell(2); k <= Cell(2)+1; ++k) {
      for (int j = Cell(1); j <= Cell(1)+1; ++j) {
        for (int i = Cell(0); i <= Cell(0)+1; ++i) {
          long long iPoint = Coords(0).Indexer().ToIndex(i,j,k);
          NodeCoords[iNode] = {
            Coords(0)[iPoint],
            Coords(1)[iPoint],
            Coords(2)[iPoint]
          };
          ++iNode;
        }
      }
    }
    Overlaps = core::OverlapsHexNonUniform(NodeCoords, PointCoords, Tolerance);
    break;
  }}

  return Overlaps;

}

}

inline bool OverlapsCell(int NumDims, const array<field<double>> &Coords, geometry_type
  GeometryType, double Tolerance, const tuple<int> &Cell, const tuple<double> &PointCoords) {

  using overlaps_cell_internal::OverlapsCellUniform;
  using overlaps_cell_internal::OverlapsCellOrientedUniform;
  using overlaps_cell_internal::OverlapsCellNonUniform;

  bool Overlaps;

  switch (GeometryType) {
  case geometry_type::UNIFORM:
  case geometry_type::RECTILINEAR:
    Overlaps = OverlapsCellUniform(NumDims, Coords, Tolerance, Cell, PointCoords);
    break;
  case geometry_type::ORIENTED_UNIFORM:
  case geometry_type::ORIENTED_RECTILINEAR:
    Overlaps = OverlapsCellOrientedUniform(NumDims, Coords, Tolerance, Cell, PointCoords);
    break;
  case geometry_type::CURVILINEAR:
    Overlaps = OverlapsCellNonUniform(NumDims, Coords, Tolerance, Cell, PointCoords);
    break;
  }

  return Overlaps;

}

namespace coords_in_cell_internal {

inline tuple<double> CoordsInCellUniform(int NumDims, const array<field<double>> &Coords, const
  tuple<int> &Cell, const tuple<double> &PointCoords) {

  tuple<int> UpperCornerOffset = MakeUniformTuple<int>(NumDims, 1, 0);

  long long iLowerCorner = Coords(0).Indexer().ToIndex(Cell);
  long long iUpperCorner = Coords(0).Indexer().ToIndex(Cell+UpperCornerOffset);

  tuple<double> LocalCoords;

  switch (NumDims) {
  case 1: {
    double LowerCornerCoord = Coords(0)[iLowerCorner];
    double UpperCornerCoord = Coords(0)[iUpperCorner];
    double PointCoord = PointCoords(0);
    double LocalCoord = core::IsoLine2NodeInverse(LowerCornerCoord, UpperCornerCoord,
      PointCoord);
    LocalCoords(0) = LocalCoord;
    LocalCoords(1) = 0.;
    LocalCoords(2) = 0.;
    break;
  }
  case 2: {
    elem<double,2> LowerCornerCoords = {
      Coords(0)[iLowerCorner],
      Coords(1)[iLowerCorner]
    };
    elem<double,2> UpperCornerCoords = {
      Coords(0)[iUpperCorner],
      Coords(1)[iUpperCorner]
    };
    elem<double,2> PointCoords_ = {PointCoords(0), PointCoords(1)};
    elem<double,2> LocalCoords_ = core::IsoQuad4NodeUniformInverse(LowerCornerCoords,
      UpperCornerCoords, PointCoords_);
    LocalCoords(0) = LocalCoords_(0);
    LocalCoords(1) = LocalCoords_(1);
    LocalCoords(2) = 0.;
    break;
  }
  default: {
    tuple<double> LowerCornerCoords = {
      Coords(0)[iLowerCorner],
      Coords(1)[iLowerCorner],
      Coords(2)[iLowerCorner]
    };
    tuple<double> UpperCornerCoords = {
      Coords(0)[iUpperCorner],
      Coords(1)[iUpperCorner],
      Coords(2)[iUpperCorner]
    };
    LocalCoords = core::IsoHex8NodeUniformInverse(LowerCornerCoords, UpperCornerCoords,
      PointCoords);
    break;
  }}

  return LocalCoords;

}

inline tuple<double> CoordsInCellOrientedUniform(int NumDims, const array<field<double>> &Coords,
  const tuple<int> &Cell, const tuple<double> &PointCoords) {

  tuple<double> LocalCoords;

  switch (NumDims) {
  case 2: {
    elem<double,2> NodeCoords[4];
    int iNode = 0;
    for (int j = Cell(1); j <= Cell(1)+1; ++j) {
      for (int i = Cell(0); i <= Cell(0)+1; ++i) {
        long long iPoint = Coords(0).Indexer().ToIndex(i,j,0);
        NodeCoords[iNode] = {
          Coords(0)[iPoint],
          Coords(1)[iPoint]
        };
        ++iNode;
      }
    }
    elem<double,2> PointCoords_ = {PointCoords(0), PointCoords(1)};
    elem<double,2> LocalCoords_ = core::IsoQuad4NodeOrientedUniformInverse(NodeCoords,
      PointCoords_);
    LocalCoords(0) = LocalCoords_(0);
    LocalCoords(1) = LocalCoords_(1);
    LocalCoords(2) = 0.;
    break;
  }
  default: {
    tuple<double> NodeCoords[8];
    int iNode = 0;
    for (int k = Cell(2); k <= Cell(2)+1; ++k) {
      for (int j = Cell(1); j <= Cell(1)+1; ++j) {
        for (int i = Cell(0); i <= Cell(0)+1; ++i) {
          long long iPoint = Coords(0).Indexer().ToIndex(i,j,k);
          NodeCoords[iNode] = {
            Coords(0)[iPoint],
            Coords(1)[iPoint],
            Coords(2)[iPoint]
          };
          ++iNode;
        }
      }
    }
    LocalCoords = core::IsoHex8NodeOrientedUniformInverse(NodeCoords, PointCoords);
    break;
  }}

  return LocalCoords;

}

inline tuple<double> CoordsInCellNonUniform(int NumDims, const array<field<double>> &Coords, const
  tuple<int> &Cell, const tuple<double> &PointCoords) {

  tuple<double> LocalCoords;

  switch (NumDims) {
  case 1: {
    double NodeCoords[4];
    int ShiftedCell = Cell(0);
    ShiftedCell = Max<int>(ShiftedCell, Coords(0).Extents().Begin(0)+1);
    ShiftedCell = Min<int>(ShiftedCell, Coords(0).Extents().End(0)-3);
    int iNode = 0;
    for (int iPoint = ShiftedCell-1; iPoint < ShiftedCell+3; ++iPoint) {
      NodeCoords[iNode] = Coords(0)[iPoint];
      ++iNode;
    }
    double PointCoord = PointCoords(0);
    double LocalCoord = core::IsoLine4NodeInverse(NodeCoords, PointCoord);
    LocalCoords(0) = LocalCoord;
    LocalCoords(1) = 0.;
    LocalCoords(2) = 0.;
    break;
  }
  case 2: {
    elem<double,2> NodeCoords[16];
    elem<int,2> ShiftedCell = {Cell(0),Cell(1)};
    for (int iDim = 0; iDim < 2; ++iDim) {
      ShiftedCell(iDim) = Max<int>(ShiftedCell(iDim), Coords(0).Extents().Begin(iDim)+1);
      ShiftedCell(iDim) = Min<int>(ShiftedCell(iDim), Coords(0).Extents().End(iDim)-3);
    }
    int iNode = 0;
    for (int j = ShiftedCell(1)-1; j <= ShiftedCell(1)+2; ++j) {
      for (int i = ShiftedCell(0)-1; i <= ShiftedCell(0)+2; ++i) {
        long long iPoint = Coords(0).Indexer().ToIndex(i,j,0);
        NodeCoords[iNode] = {
          Coords(0)[iPoint],
          Coords(1)[iPoint]
        };
        ++iNode;
      }
    }
    elem<double,2> PointCoords_ = {PointCoords(0), PointCoords(1)};
    elem<double,2> LocalCoords_ = core::IsoQuad16NodeInverse(NodeCoords, PointCoords_);
    LocalCoords(0) = LocalCoords_(0);
    LocalCoords(1) = LocalCoords_(1);
    LocalCoords(2) = 0.;
    break;
  }
  default: {
    tuple<double> NodeCoords[64];
    tuple<int> ShiftedCell = Cell;
    for (int iDim = 0; iDim < 3; ++iDim) {
      ShiftedCell(iDim) = Max<int>(ShiftedCell(iDim), Coords(0).Extents().Begin(iDim)+1);
      ShiftedCell(iDim) = Min<int>(ShiftedCell(iDim), Coords(0).Extents().End(iDim)-3);
    }
    int iNode = 0;
    for (int k = ShiftedCell(2)-1; k <= ShiftedCell(2)+2; ++k) {
      for (int j = ShiftedCell(1)-1; j <= ShiftedCell(1)+2; ++j) {
        for (int i = ShiftedCell(0)-1; i <= ShiftedCell(0)+2; ++i) {
          long long iPoint = Coords(0).Indexer().ToIndex(i,j,0);
          NodeCoords[iNode] = {
            Coords(0)[iPoint],
            Coords(1)[iPoint],
            Coords(2)[iPoint]
          };
          ++iNode;
        }
      }
    }
    LocalCoords = core::IsoHex64NodeInverse(NodeCoords, PointCoords);
    break;
  }}

  return LocalCoords;

}

}

inline tuple<double> CoordsInCell(int NumDims, const array<field<double>> &Coords, geometry_type
  GeometryType, const tuple<int> &Cell, const tuple<double> &PointCoords) {

  using coords_in_cell_internal::CoordsInCellUniform;
  using coords_in_cell_internal::CoordsInCellOrientedUniform;
  using coords_in_cell_internal::CoordsInCellNonUniform;

  tuple<double> LocalCoords;

  switch (GeometryType) {
  case geometry_type::UNIFORM:
    LocalCoords = CoordsInCellUniform(NumDims, Coords, Cell, PointCoords);
    break;
  case geometry_type::ORIENTED_UNIFORM:
    LocalCoords = CoordsInCellOrientedUniform(NumDims, Coords, Cell, PointCoords);
    break;
  case geometry_type::RECTILINEAR:
  case geometry_type::ORIENTED_RECTILINEAR:
  case geometry_type::CURVILINEAR:
    LocalCoords = CoordsInCellNonUniform(NumDims, Coords, Cell, PointCoords);
    break;
  }

  return LocalCoords;

}

}}