#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct ezPropertyMetaStateEvent;

/// Display names used for branch type entries in the editor UI.
namespace ezKrautBranchTypeNames
{
  constexpr const char* Trunk = "Trunk";
  constexpr const char* MainBranch1 = "Main Branch 1";
  constexpr const char* MainBranch2 = "Main Branch 2";
  constexpr const char* MainBranch3 = "Main Branch 3";
  constexpr const char* SubBranch1 = "Sub Branch 1";
  constexpr const char* SubBranch2 = "Sub Branch 2";
  constexpr const char* SubBranch3 = "Sub Branch 3";
  constexpr const char* Twig1 = "Twig 1";
  constexpr const char* Twig2 = "Twig 2";
  constexpr const char* Twig3 = "Twig 3";
} // namespace ezKrautBranchTypeNames

/// Maps a user-visible label to a material asset path for one slot in the tree's material list.
struct ezKrautAssetMaterial
{
  ezString m_sLabel;
  ezString m_sMaterial;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautAssetMaterial);

/// Controls the overall growth pattern of a branch type.
struct ezKrautBranchTypeMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Regular,  ///< Standard outward growth.
    Umbrella, ///< Branches arc upward and then droop outward, forming an umbrella silhouette.

    Default = Regular,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautBranchTypeMode);

/// Specifies the target growth direction for a branch type, expressed as an angle from straight up.
struct ezKrautBranchTargetDir
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Straight, ///< Along the start direction.
    Upwards,  ///< Toward positive Z (sky).
    Degree22,
    Degree45,
    Degree67,
    Degree90,
    Degree112,
    Degree135,
    Degree157,
    Downwards, ///< Toward negative Z (ground).

    Default = Straight
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautBranchTargetDir);

/// Controls the orientation of leaf geometry relative to the branch it grows from.
struct ezKrautLeafOrientation
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Upwards,            ///< Leaf plane faces upward regardless of branch direction.
    AlongBranch,        ///< Leaf plane is aligned with the branch axis.
    OrthogonalToBranch, ///< Leaf plane is perpendicular to the branch axis.

    Default = Upwards,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautLeafOrientation);

/// Determines how the contour curve is applied to frond geometry.
struct ezKrautFrondContourMode
{
  using StorageType = ezInt8;

  enum Enum
  {
    Off = -1,        ///< No contour; frond uses a flat rectangular shape.
    Full,            ///< Contour applied to the full frond width.
    Symetric,        ///< Contour mirrored symmetrically around the frond center.
    InverseSymetric, ///< Inverse of Symetric.

    Default = Full
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautFrondContourMode);

/// Controls whether and how a second target direction blends with the primary target direction.
struct ezKrautBranchTargetDir2Usage
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Off,      ///< Only the primary target direction is used.
    Relative, ///< Secondary direction is relative to the branch's start orientation.
    Absolute, ///< Secondary direction is in world space.

    Default = Off
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautBranchTargetDir2Usage);

/// Selects the geometry representation used for a LOD level.
struct ezKrautLodMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Full,      ///< Full triangle mesh for all enabled geometry types.
    FourQuads, ///< Impostor represented by four intersecting quads.
    TwoQuads,  ///< Impostor represented by two intersecting quads.
    Billboard, ///< Single camera-facing billboard quad.
    Disabled,  ///< LOD is not generated.

    Default = Full
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautLodMode);

/// Controls how the tapered tip of a branch segment is tessellated.
struct ezKrautBranchSpikeTipMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    FullDetail,     ///< Full vertex ring at the tip.
    SingleTriangle, ///< Tip collapsed to a single triangle fan, reducing vertex count.
    Hole,           ///< Tip is left open (no cap geometry).

    Default = FullDetail
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautBranchSpikeTipMode);

/// Authoring data for a single Kraut branch type (trunk, main branch, sub-branch, or twig).
///
/// Each field group maps to a section in the Kraut tree editor's property panel.
/// Instances of this struct are embedded in ezKrautTreeAssetProperties, one per active branch type slot.
struct ezKrautAssetBranchType
{
  // === Administrative ===

  // bool m_bVisible = true;

  bool m_bGrowSubBranchType1 = false;
  bool m_bGrowSubBranchType2 = false;
  bool m_bGrowSubBranchType3 = false;

  // === Branch Type ===

  // General

  ezUInt8 m_uiSegmentLengthCM = 5;
  ezEnum<ezKrautBranchTypeMode> m_BranchTypeMode = ezKrautBranchTypeMode::Regular;
  float m_fBranchlessPartABS = 0.0f;
  float m_fBranchlessPartEndABS = 0.0f;
  ezUInt8 m_uiLowerBound = 0;
  ezUInt8 m_uiUpperBound = 100;
  ezUInt16 m_uiMinBranchThicknessInCM = 20;
  ezUInt16 m_uiMaxBranchThicknessInCM = 20;

  // Spawn Nodes

  ezUInt8 m_uiMinBranches = 4;
  ezUInt8 m_uiMaxBranches = 4;
  float m_fNodeSpacingBefore = 0.5f;
  float m_fNodeSpacingAfter = 0.0f;
  float m_fNodeHeight = 0.0f;


  // === Growth ===

  // Start Direction

  ezAngle m_MaxRotationalDeviation = {};
  ezAngle m_BranchAngle = ezAngle::MakeFromDegree(90);
  ezAngle m_MaxBranchAngleDeviation = ezAngle::MakeFromDegree(10);

  // Target Direction

  ezEnum<ezKrautBranchTargetDir> m_TargetDirection = ezKrautBranchTargetDir::Straight;
  bool m_bTargetDirRelative = false;
  ezEnum<ezKrautBranchTargetDir2Usage> m_TargetDir2Usage = ezKrautBranchTargetDir2Usage::Off;
  float m_fTargetDir2Usage = 2.5f;
  ezEnum<ezKrautBranchTargetDir> m_TargetDirection2 = ezKrautBranchTargetDir::Upwards;
  ezAngle m_MaxTargetDirDeviation = ezAngle::MakeFromDegree(20);

  // Growth

  ezUInt16 m_uiMinBranchLengthInCM = 100;
  ezUInt16 m_uiMaxBranchLengthInCM = 100;
  ezSingleCurveData m_MaxBranchLengthParentScale;
  ezAngle m_GrowMaxTargetDirDeviation = {};
  ezAngle m_GrowMaxDirChangePerSegment = ezAngle::MakeFromDegree(5);
  bool m_bRestrictGrowthToFrondPlane = false;

  // Obstacles

  // bool m_bActAsObstacle = false;
  // bool m_bDoPhysicalSimulation = false;
  // float m_fPhysicsLookAhead = 1.5f;
  // ezAngle m_PhysicsEvasionAngle = ezAngle::MakeFromDegree(30);


  // === Appearance ===

  // Branch Mesh

  bool m_bEnableMesh = true;
  ezString m_sBranchMaterial;
  ezSingleCurveData m_BranchContour;
  float m_fRoundnessFactor = 0.5f;
  ezUInt8 m_uiFlares = 0;
  float m_fFlareWidth = 2.0f;
  ezSingleCurveData m_FlareWidthCurve;
  ezAngle m_FlareRotation = {};
  bool m_bRotateTexCoords = true;

  // Fronds

  bool m_bEnableFronds = true;
  ezString m_sFrondMaterial;
  float m_fTextureRepeat = 0.0f;
  ezEnum<ezKrautLeafOrientation> m_FrondUpOrientation = ezKrautLeafOrientation::Upwards;
  ezAngle m_MaxFrondOrientationDeviation = {};
  ezUInt8 m_uiNumFronds = 1;
  bool m_bAlignFrondsOnSurface = false;
  ezUInt8 m_uiFrondDetail = 1;
  ezSingleCurveData m_FrondContour;
  ezEnum<ezKrautFrondContourMode> m_FrondContourMode = ezKrautFrondContourMode::Full;
  float m_fFrondHeight = 0.5f;
  ezSingleCurveData m_FrondHeight;
  float m_fFrondWidth = 0.5f;
  ezSingleCurveData m_FrondWidth;
  // ezColor m_FrondVariationColor;// currently done through the material

  // Leaves

  bool m_bEnableLeaves = true;
  ezString m_sLeafMaterial;
  bool m_bBillboardLeaves = true;
  float m_fLeafSize = 0.25f;
  ezSingleCurveData m_LeafScale;
  float m_fLeafInterval = 0;
  // ezColor m_LeafVariationColor;// currently done through the material

  // Shared

  // ezUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  // ezUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautAssetBranchType);


/// Authoring data for a single LOD level in a Kraut tree asset.
///
/// Controls which geometry types are included in the LOD (via the Allow* bitfields)
/// and how much mesh detail is generated. m_uiLodDistance sets the camera distance
/// at which the engine switches to this LOD.
struct ezKrautAssetLod
{
  ezEnum<ezKrautLodMode> m_Mode = ezKrautLodMode::Full;
  float m_fTipDetail = 0.04f;
  float m_fCurvatureThreshold = 5.0f;
  float m_fThicknessThreshold = 0.2f;
  float m_fVertexRingDetail = 0.2f;
  ezBitflags<ezKrautTreeTypeBits> m_AllowBranch = ezKrautTreeTypeBits::Default; ///< Branch types for which branch mesh geometry is generated in this LOD.
  ezBitflags<ezKrautTreeTypeBits> m_AllowFrond = ezKrautTreeTypeBits::Default;  ///< Branch types for which frond geometry is generated in this LOD.
  ezBitflags<ezKrautTreeTypeBits> m_AllowLeaf = ezKrautTreeTypeBits::Default;   ///< Branch types for which leaf geometry is generated in this LOD.
  ezInt8 m_iMaxFrondDetail = 32;
  ezInt8 m_iFrondDetailReduction = 0;
  ezUInt32 m_uiLodDistance = 0;                                                 ///< Camera distance in meters at which the engine transitions to this LOD.
  ezEnum<ezKrautBranchSpikeTipMode> m_BranchSpikeTipMode = ezKrautBranchSpikeTipMode::FullDetail;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautAssetLod);

/// The editable property object for a Kraut tree asset document.
///
/// Aggregates all branch type descriptors (one per active slot), up to five LOD descriptors,
/// the material list, and global tree properties. The document serializes this object and
/// uses it to drive ezKrautGeneratorResourceDescriptor creation at transform time.
class ezKrautTreeAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeAssetProperties, ezReflectedClass);

public:
  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezKrautTreeAssetProperties();
  ~ezKrautTreeAssetProperties();

  // float m_fUniformScaling = 1.0f;
  // float m_fLodDistanceScale = 1.0f;
  float m_fStaticColliderRadius = 0.4f;
  float m_fTreeStiffness = 10.0f;
  float m_fMinAmbientOcclusion = 0.7f;
  ezString m_sSurface;

  ezHybridArray<ezKrautAssetMaterial, 8> m_Materials;
  ezKrautAssetBranchType m_BT_Trunk1;
  // ezKrautAssetBranchType m_BT_Trunk2;
  // ezKrautAssetBranchType m_BT_Trunk3;
  ezKrautAssetBranchType m_BT_MainBranch1;
  ezKrautAssetBranchType m_BT_MainBranch2;
  ezKrautAssetBranchType m_BT_MainBranch3;
  ezKrautAssetBranchType m_BT_SubBranch1;
  ezKrautAssetBranchType m_BT_SubBranch2;
  ezKrautAssetBranchType m_BT_SubBranch3;
  ezKrautAssetBranchType m_BT_Twig1;
  ezKrautAssetBranchType m_BT_Twig2;
  ezKrautAssetBranchType m_BT_Twig3;

  ezKrautAssetLod m_Lod0;
  ezKrautAssetLod m_Lod1;
  ezKrautAssetLod m_Lod2;
  ezKrautAssetLod m_Lod3;
  ezKrautAssetLod m_Lod4;

  /// Seed used to generate the tree shown in the asset editor preview.
  ezUInt16 m_uiRandomSeedForDisplay = 0;

  /// List of seeds that produce good-looking results for this tree type.
  /// ezKrautTreeComponent uses this list when a variation index is specified.
  ezHybridArray<ezUInt16, 16> m_GoodRandomSeeds;
};
