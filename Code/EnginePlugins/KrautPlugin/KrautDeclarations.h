#pragma once

#include <KrautPlugin/KrautPluginDLL.h>

#include <Foundation/Reflection/Reflection.h>

struct ezResourceEvent;

class ezKrautLodInfo;
class ezKrautRenderData;

class ezKrautTreeComponent;
class ezKrautTreeComponentManager;

/// Tracks the async generation state of a single LOD slot inside an ezKrautTreeResource.
enum class ezKrautLodState : ezUInt8
{
  NotGenerated = 0,
  Generating = 1,
  Ready = 2,
};

/// Describes the rendering technique used for a LOD.
enum class ezKrautLodType : ezUInt8
{
  None = 0xFF,
  Mesh = 0,              ///< Standard triangle mesh.
  StaticImpostor = 1,    ///< Pre-baked static impostor quad(s).
  BillboardImpostor = 2, ///< Camera-facing billboard impostor.
};

/// Identifies the material category of a sub-mesh within a tree LOD.
enum class ezKrautMaterialType : ezUInt8
{
  None = 0xFF,
  Branch = 0, ///< Cylindrical branch/trunk geometry.
  Frond = 1,  ///< Flat frond geometry along branches.
  Leaf = 2,   ///< Individual leaf geometry or billboards.
};

/// Identifies one of the named branch types that the Kraut generator supports.
///
/// Each branch type maps to one slot in the tree's hierarchical structure
/// (trunk, main branches, sub-branches, twigs), with up to three parallel
/// variants per level. The value 0xFF (None) is used as a sentinel.
enum class ezKrautBranchType : ezUInt8
{
  None = 0xFF,
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
};

/// Bitflag set that selects which branch types should be visible.
///
/// Used by the editor preview to show or hide individual branch types while
/// inspecting a LOD. The Default value enables all bits so that every branch
/// type is visible unless explicitly masked out.
struct ezKrautTreeTypeBits
{
  using StorageType = ezUInt32;

  enum Enum : ezUInt32
  {
    Trunk1 = EZ_BIT(0),
    Trunk2 = EZ_BIT(1),
    Trunk3 = EZ_BIT(2),
    MainBranches1 = EZ_BIT(3),
    MainBranches2 = EZ_BIT(4),
    MainBranches3 = EZ_BIT(5),
    SubBranches1 = EZ_BIT(6),
    SubBranches2 = EZ_BIT(7),
    SubBranches3 = EZ_BIT(8),
    Twigs1 = EZ_BIT(9),
    Twigs2 = EZ_BIT(10),
    Twigs3 = EZ_BIT(11),

    Default = 0xFFFFFFFF ///< All branch types enabled.
  };

  struct Bits
  {
    StorageType Trunk1 : 1;
    StorageType Trunk2 : 1;
    StorageType Trunk3 : 1;
    StorageType MainBranches1 : 1;
    StorageType MainBranches2 : 1;
    StorageType MainBranches3 : 1;
    StorageType SubBranches1 : 1;
    StorageType SubBranches2 : 1;
    StorageType SubBranches3 : 1;
    StorageType Twigs1 : 1;
    StorageType Twigs2 : 1;
    StorageType Twigs3 : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezKrautTreeTypeBits);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_KRAUTPLUGIN_DLL, ezKrautTreeTypeBits);
