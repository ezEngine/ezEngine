#pragma once

#include <KrautGenerator/KrautGeneratorDLL.h>

namespace Kraut
{
  struct BranchTargetDir
  {
    enum Enum
    {
      Straight, // along the start direction
      Upwards,  // to the sky!
      Degree22,
      Degree45,
      Degree67,
      Degree90,
      Degree112,
      Degree135,
      Degree157,
      Downwards, // to the ground
      ENUM_COUNT
    };
  };

  struct BranchTargetDir2Usage
  {
    enum Enum
    {
      Off,
      Relative,
      Absolute,
    };
  };

  struct LeafOrientation
  {
    enum Enum
    {
      Upwards,
      AlongBranch,
      OrthogonalToBranch,
      ENUM_COUNT
    };
  };

  struct BranchTypeMode
  {
    enum Enum
    {
      Default,
      Umbrella,
      ENUM_COUNT
    };
  };

  struct BranchType
  {
    enum Enum
    {
      None = -1,
      Trunk1 = 0,
      Trunk2 = 1,
      Trunk3 = 2,
      MainBranches1 = 3,
      MainBranches2 = 4,
      MainBranches3 = 5,
      SubBranches1 = 6,
      SubBranches2 = 7,
      SubBranches3 = 8,
      Twigs1 = 9,
      Twigs2 = 10,
      Twigs3 = 11,

      ENUM_COUNT
    };
  };

  struct LodMode
  {
    enum Enum
    {
      Full,
      FourQuads,
      TwoQuads,
      Billboard,
      Disabled,
      ENUM_COUNT
    };

    static bool IsImpostorMode(LodMode::Enum mode)
    {
      return (mode != LodMode::Full && mode != LodMode::Disabled);
    }

    static bool IsMeshMode(LodMode::Enum mode)
    {
      return (mode == LodMode::Full);
    }
  };

  struct BranchGeometryType
  {
    enum Enum
    {
      Branch,
      Frond,
      Leaf,
      ENUM_COUNT
    };
  };

  struct BranchSpikeTipMode
  {
    enum Enum
    {
      FullDetail,
      SingleTriangle,
      Hole,
      ENUM_COUNT
    };
  };

} // namespace Kraut
