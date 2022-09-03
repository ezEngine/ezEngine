#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct ezKrautAssetMaterial
{
  ezString m_sLabel;
  ezString m_sMaterial;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautAssetMaterial);

struct ezKrautBranchTypeMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Regular,
    Umbrella,

    Default = Regular,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautBranchTypeMode);

struct ezKrautBranchTargetDir
{
  using StorageType = ezUInt8;

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

    Default = Straight
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautBranchTargetDir);

struct ezKrautLeafOrientation
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Upwards,
    AlongBranch,
    OrthogonalToBranch,

    Default = Upwards,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautLeafOrientation);

struct ezKrautFrondContourMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Full,
    Symetric,
    InverseSymetric,

    Default = Full
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautFrondContourMode);

struct ezKrautBranchTargetDir2Usage
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Off,
    Relative,
    Absolute,

    Default = Off
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautBranchTargetDir2Usage);

struct ezKrautAssetBranchType
{
  // === Administrative ===

  //Kraut::BranchType::Enum m_Type;
  //bool m_bUsed = false;
  //bool m_bVisible = true;

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
  ezAngle m_BranchAngle = ezAngle::Degree(90);
  ezAngle m_MaxBranchAngleDeviation = ezAngle::Degree(10);

  // Target Direction

  ezEnum<ezKrautBranchTargetDir> m_TargetDirection = ezKrautBranchTargetDir::Straight;
  bool m_bTargetDirRelative = false;
  ezEnum<ezKrautBranchTargetDir2Usage> m_TargetDir2Uage = ezKrautBranchTargetDir2Usage::Off;
  float m_fTargetDir2Usage = 2.5f;
  ezEnum<ezKrautBranchTargetDir> m_TargetDirection2 = ezKrautBranchTargetDir::Upwards;
  ezAngle m_MaxTargetDirDeviation = ezAngle::Degree(20);

  // Growth

  ezUInt16 m_uiMinBranchLengthInCM = 100;
  ezUInt16 m_uiMaxBranchLengthInCM = 100;
  //Kraut::Curve m_MaxBranchLengthParentScale;
  ezAngle m_GrowMaxTargetDirDeviation = {};
  ezAngle m_GrowMaxDirChangePerSegment = ezAngle::Degree(5);
  bool m_bRestrictGrowthToFrondPlane = false;

  // Obstacles

  //bool m_bActAsObstacle = false;
  //bool m_bDoPhysicalSimulation = false;
  //float m_fPhysicsLookAhead = 1.5f;
  //ezAngle m_PhysicsEvasionAngle = ezAngle::Degree(30);


  // === Appearance ===

  // Branch Mesh

  bool m_bEnableMesh = true;
  ezString m_sBranchMaterial;
  ezSingleCurveData m_BranchContour;
  //Kraut::Curve m_BranchContour;
  float m_fRoundnessFactor = 0.5f;
  ezUInt8 m_uiFlares = 0;
  float m_fFlareWidth = 2.0f;
  //Kraut::Curve m_FlareWidthCurve;
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
  //Kraut::Curve m_FrondContour;
  ezEnum<ezKrautFrondContourMode> m_FrondContourMode = ezKrautFrondContourMode::Full;
  float m_fFrondHeight = 0.5f;
  //Kraut::Curve m_FrondHeight;
  float m_fFrondWidth = 0.5f;
  //Kraut::Curve m_FrondWidth;
  //ezColor m_FrondVariationColor;// currently done through the material

  // Leaves

  bool m_bEnableLeaves = true;
  ezString m_sLeafMaterial;
  bool m_bBillboardLeaves = true;
  float m_fLeafSize = 0.25f;
  //Kraut::Curve m_LeafScale;
  float m_fLeafInterval = 0;
  //ezColor m_LeafVariationColor;// currently done through the material

  // Shared

  //ezString m_sTexture[Kraut::BranchGeometryType::ENUM_COUNT];
  //ezUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  //ezUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezKrautAssetBranchType);

class ezKrautTreeAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeAssetProperties, ezReflectedClass);

public:
  ezKrautTreeAssetProperties();
  ~ezKrautTreeAssetProperties();

  //ezString m_sKrautFile;
  //float m_fUniformScaling = 1.0f;
  //float m_fLodDistanceScale = 1.0f;
  float m_fStaticColliderRadius = 0.4f;
  float m_fTreeStiffness = 10.0f;
  ezString m_sSurface;

  ezHybridArray<ezKrautAssetMaterial, 8> m_Materials;
  ezKrautAssetBranchType m_BT_Trunk1;
  //ezKrautAssetBranchType m_BT_Trunk2;
  //ezKrautAssetBranchType m_BT_Trunk3;
  ezKrautAssetBranchType m_BT_MainBranch1;
  ezKrautAssetBranchType m_BT_MainBranch2;
  ezKrautAssetBranchType m_BT_MainBranch3;
  ezKrautAssetBranchType m_BT_SubBranch1;
  ezKrautAssetBranchType m_BT_SubBranch2;
  ezKrautAssetBranchType m_BT_SubBranch3;
  ezKrautAssetBranchType m_BT_Twig1;
  ezKrautAssetBranchType m_BT_Twig2;
  ezKrautAssetBranchType m_BT_Twig3;

  ezUInt16 m_uiRandomSeedForDisplay = 0;

  ezHybridArray<ezUInt16, 16> m_GoodRandomSeeds;
};
