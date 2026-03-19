#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <KrautGenerator/Description/SpawnNodeDesc.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Material/MaterialResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezKrautAssetMaterial, ezNoBase, 1, ezRTTIDefaultAllocator<ezKrautAssetMaterial>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new ezReadOnlyAttribute()),
    EZ_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "Kraut")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezKrautBranchTypeMode, 1)
  EZ_ENUM_CONSTANTS(ezKrautBranchTypeMode::Regular, ezKrautBranchTypeMode::Umbrella)
EZ_END_STATIC_REFLECTED_ENUM

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezKrautBranchTargetDir, 1)
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir::Straight),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir::Upwards),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir::Degree22),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir::Degree45),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir::Degree67),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir::Degree90),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir::Degree112),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir::Degree135),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir::Degree157),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir::Downwards),
EZ_END_STATIC_REFLECTED_ENUM

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezKrautLeafOrientation, 1)
  EZ_ENUM_CONSTANT(ezKrautLeafOrientation::Upwards),
  EZ_ENUM_CONSTANT(ezKrautLeafOrientation::AlongBranch),
  EZ_ENUM_CONSTANT(ezKrautLeafOrientation::OrthogonalToBranch),
EZ_END_STATIC_REFLECTED_ENUM

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezKrautFrondContourMode, 1)
  EZ_ENUM_CONSTANT(ezKrautFrondContourMode::Off),
  EZ_ENUM_CONSTANT(ezKrautFrondContourMode::Full),
  EZ_ENUM_CONSTANT(ezKrautFrondContourMode::Symetric),
  EZ_ENUM_CONSTANT(ezKrautFrondContourMode::InverseSymetric),
EZ_END_STATIC_REFLECTED_ENUM

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezKrautBranchTargetDir2Usage, 1)
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir2Usage::Off),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir2Usage::Relative),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir2Usage::Absolute),
EZ_END_STATIC_REFLECTED_ENUM

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezKrautLodMode, 1)
  EZ_ENUM_CONSTANT(ezKrautLodMode::Full),
  //EZ_ENUM_CONSTANT(ezKrautLodMode::FourQuads),
  //EZ_ENUM_CONSTANT(ezKrautLodMode::TwoQuads),
  //EZ_ENUM_CONSTANT(ezKrautLodMode::Billboard),
  EZ_ENUM_CONSTANT(ezKrautLodMode::Disabled),
EZ_END_STATIC_REFLECTED_ENUM

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezKrautBranchSpikeTipMode, 1)
  EZ_ENUM_CONSTANT(ezKrautBranchSpikeTipMode::FullDetail),
  EZ_ENUM_CONSTANT(ezKrautBranchSpikeTipMode::SingleTriangle),
  EZ_ENUM_CONSTANT(ezKrautBranchSpikeTipMode::Hole),
EZ_END_STATIC_REFLECTED_ENUM


EZ_BEGIN_STATIC_REFLECTED_TYPE(ezKrautAssetBranchType, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    // === Administrative ===

    EZ_MEMBER_PROPERTY("GrowSubBranchType1", m_bGrowSubBranchType1),
    EZ_MEMBER_PROPERTY("GrowSubBranchType2", m_bGrowSubBranchType2),
    EZ_MEMBER_PROPERTY("GrowSubBranchType3", m_bGrowSubBranchType3),

    // === Branch Type ===

    // General

    EZ_MEMBER_PROPERTY("SegmentLength", m_uiSegmentLengthCM)->AddAttributes(new ezDefaultValueAttribute(5), new ezClampValueAttribute(1, 50), new ezSuffixAttribute("cm"), new ezGroupAttribute("Branch Type: General Settings")),
    EZ_ENUM_MEMBER_PROPERTY("BranchType", ezKrautBranchTypeMode, m_BranchTypeMode)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("BranchlessPartABS", m_fBranchlessPartABS)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0, 10.0f), new ezSuffixAttribute("m")),
    EZ_MEMBER_PROPERTY("BranchlessPartEndABS", m_fBranchlessPartEndABS)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0, 10.0f), new ezSuffixAttribute("m")),
    EZ_MEMBER_PROPERTY("LowerBound", m_uiLowerBound)->AddAttributes(new ezDefaultValueAttribute(0), new ezClampValueAttribute(0, 100), new ezSuffixAttribute("%")),
    EZ_MEMBER_PROPERTY("UpperBound", m_uiUpperBound)->AddAttributes(new ezDefaultValueAttribute(100), new ezClampValueAttribute(0, 100), new ezSuffixAttribute("%")),
    EZ_MEMBER_PROPERTY("MinBranchThickness", m_uiMinBranchThicknessInCM)->AddAttributes(new ezDefaultValueAttribute(20), new ezClampValueAttribute(1, 100), new ezSuffixAttribute("cm")),
    EZ_MEMBER_PROPERTY("MaxBranchThickness", m_uiMaxBranchThicknessInCM)->AddAttributes(new ezDefaultValueAttribute(20), new ezClampValueAttribute(1, 100), new ezSuffixAttribute("cm")),

    // Spawn Nodes

    EZ_MEMBER_PROPERTY("MinBranchesPerNode", m_uiMinBranches)->AddAttributes(new ezDefaultValueAttribute(4), new ezClampValueAttribute(0, 32), new ezGroupAttribute("Branch Type: Spawn Nodes")),
    EZ_MEMBER_PROPERTY("MaxBranchesPerNode", m_uiMaxBranches)->AddAttributes(new ezDefaultValueAttribute(4), new ezClampValueAttribute(0, 32)),
    EZ_MEMBER_PROPERTY("NodeSpacingBefore", m_fNodeSpacingBefore)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0, 5.0f), new ezSuffixAttribute("m")),
    EZ_MEMBER_PROPERTY("NodeSpacingAfter", m_fNodeSpacingAfter)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0, 5.0f), new ezSuffixAttribute("m")),
    EZ_MEMBER_PROPERTY("NodeHeight", m_fNodeHeight)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0, 5.0f), new ezSuffixAttribute("m")),

    // === Growth ===

    // Start Direction
    EZ_MEMBER_PROPERTY("MaxRotationalDeviation", m_MaxRotationalDeviation)->AddAttributes(new ezDefaultValueAttribute(ezAngle()), new ezClampValueAttribute(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(180)), new ezGroupAttribute("Growth: Start Direction")),
    EZ_MEMBER_PROPERTY("BranchAngle", m_BranchAngle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(90)), new ezClampValueAttribute(ezAngle::MakeFromDegree(1), ezAngle::MakeFromDegree(179))),
    EZ_MEMBER_PROPERTY("MaxBranchAngleDeviation", m_MaxBranchAngleDeviation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(10)), new ezClampValueAttribute(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(90))),

    // Target Direction
    EZ_ENUM_MEMBER_PROPERTY("TargetDirection", ezKrautBranchTargetDir, m_TargetDirection)->AddAttributes(new ezGroupAttribute("Growth: Target Direction")),
    EZ_MEMBER_PROPERTY("TargetDirRelative", m_bTargetDirRelative),
    EZ_MEMBER_PROPERTY("MaxTargetDirDeviation", m_MaxTargetDirDeviation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(20)), new ezClampValueAttribute(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(90))),
    EZ_ENUM_MEMBER_PROPERTY("TargetDir2Usage", ezKrautBranchTargetDir2Usage, m_TargetDir2Usage),
    EZ_MEMBER_PROPERTY("TargetDir2Offset", m_fTargetDir2Usage)->AddAttributes(new ezDefaultValueAttribute(2.5f), new ezClampValueAttribute(0.01f, 5.0f), new ezSuffixAttribute("m")),
    EZ_ENUM_MEMBER_PROPERTY("TargetDirection2", ezKrautBranchTargetDir, m_TargetDirection2),

    // Growth
    EZ_MEMBER_PROPERTY("MinBranchLength", m_uiMinBranchLengthInCM)->AddAttributes(new ezDefaultValueAttribute(100), new ezClampValueAttribute(1, 10000), new ezSuffixAttribute("cm"), new ezGroupAttribute("Growth: Branch Behavior")),
    EZ_MEMBER_PROPERTY("MaxBranchLength", m_uiMaxBranchLengthInCM)->AddAttributes(new ezDefaultValueAttribute(100), new ezClampValueAttribute(1, 10000), new ezSuffixAttribute("cm")),
    EZ_MEMBER_PROPERTY("MaxBranchLengthParentScale", m_MaxBranchLengthParentScale)->AddAttributes(new ezColorAttribute(ezColor::Brown), new ezCurveExtentsAttribute(0.0, true, 1.0f, true), new ezClampValueAttribute(0.0, 1.0), new ezDefaultValueAttribute(1.0)),
    EZ_MEMBER_PROPERTY("TargetDirDeviation", m_GrowMaxTargetDirDeviation)->AddAttributes(new ezDefaultValueAttribute(ezAngle()), new ezClampValueAttribute(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(180))),
    EZ_MEMBER_PROPERTY("DirChangePerSegment", m_GrowMaxDirChangePerSegment)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(5)), new ezClampValueAttribute(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(90))),
    EZ_MEMBER_PROPERTY("OnlyGrowUpAndDown", m_bRestrictGrowthToFrondPlane),

    // Obstacles

    //bool m_bActAsObstacle;
    //bool m_bDoPhysicalSimulation;
    //float m_fPhysicsLookAhead;
    //float m_fPhysicsEvasionAngle;

    // === Appearance ===

    // Branch Mesh

    EZ_MEMBER_PROPERTY("EnableMesh", m_bEnableMesh)->AddAttributes(new ezDefaultValueAttribute(true), new ezGroupAttribute("Appearance: Branch Mesh")),
    EZ_MEMBER_PROPERTY("BranchMaterial", m_sBranchMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "Kraut-Stem")),
    EZ_MEMBER_PROPERTY("BranchContour", m_BranchContour)->AddAttributes(new ezColorAttribute(ezColor::Brown), new ezCurveExtentsAttribute(0.0f, true, 1.0f, true), new ezClampValueAttribute(0.1, 1.0), new ezDefaultValueAttribute(1.0)),
    EZ_MEMBER_PROPERTY("Roundness", m_fRoundnessFactor)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("Flares", m_uiFlares)->AddAttributes(new ezDefaultValueAttribute(0), new ezClampValueAttribute(0, 16)),
    EZ_MEMBER_PROPERTY("FlareWidth", m_fFlareWidth)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("FlareWidthCurve", m_FlareWidthCurve)->AddAttributes(new ezColorAttribute(ezColor::FloralWhite), new ezCurveExtentsAttribute(0.0, true, 1.0, true), new ezClampValueAttribute(0.0, 1.0), new ezDefaultValueAttribute(1.0)),
    EZ_MEMBER_PROPERTY("FlareRotation", m_FlareRotation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(0)), new ezClampValueAttribute(ezAngle::MakeFromDegree(-720), ezAngle::MakeFromDegree(720))),
    EZ_MEMBER_PROPERTY("RotateTexCoords", m_bRotateTexCoords)->AddAttributes(new ezDefaultValueAttribute(true)),

    // Fronds

    EZ_MEMBER_PROPERTY("EnableFronds", m_bEnableFronds)->AddAttributes(new ezGroupAttribute("Appearance: Fronds")),
    EZ_MEMBER_PROPERTY("FrondMaterial", m_sFrondMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "Kraut-Frond")),
    EZ_MEMBER_PROPERTY("NumFronds", m_uiNumFronds)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("FrondDetail", m_uiFrondDetail)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 32)),
    EZ_MEMBER_PROPERTY("FrondWidth", m_fFrondWidth)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 10.0f), new ezSuffixAttribute("m")),
    EZ_MEMBER_PROPERTY("FrondWidthScale", m_FrondWidth)->AddAttributes(new ezColorAttribute(ezColor::LawnGreen), new ezCurveExtentsAttribute(0.0, true, 1.0, true), new ezClampValueAttribute(0.0, 1.0), new ezDefaultValueAttribute(1.0)),
    EZ_MEMBER_PROPERTY("TextureRepeat", m_fTextureRepeat)->AddAttributes(new ezClampValueAttribute(0.0f, 99.0f)),
    EZ_ENUM_MEMBER_PROPERTY("FrondUpOrientation", ezKrautLeafOrientation, m_FrondUpOrientation),
    EZ_MEMBER_PROPERTY("FrondOrientationDeviation", m_MaxFrondOrientationDeviation)->AddAttributes(new ezClampValueAttribute(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(180))),
    EZ_MEMBER_PROPERTY("AlignFrondsOnSurface", m_bAlignFrondsOnSurface),
    EZ_ENUM_MEMBER_PROPERTY("FrondContourMode", ezKrautFrondContourMode, m_FrondContourMode),
    EZ_MEMBER_PROPERTY("FrondContour", m_FrondContour)->AddAttributes(new ezColorAttribute(ezColor::LawnGreen), new ezCurveExtentsAttribute(0.0, true, 1.0, true), new ezClampValueAttribute(0.0, 1.0), new ezDefaultValueAttribute(1.0)),
    EZ_MEMBER_PROPERTY("FrondHeight", m_fFrondHeight)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 10.0f), new ezSuffixAttribute("m")),
    EZ_MEMBER_PROPERTY("FrondHeightScale", m_FrondHeight)->AddAttributes(new ezColorAttribute(ezColor::LawnGreen), new ezCurveExtentsAttribute(0.0, true, 1.0, true), new ezClampValueAttribute(-1.0, 1.0)),
    //EZ_MEMBER_PROPERTY("FrondVariationColor", m_FrondVariationColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),

    // Leaves

    EZ_MEMBER_PROPERTY("EnableLeaves", m_bEnableLeaves)->AddAttributes(new ezGroupAttribute("Appearance: Leaves")),
    EZ_MEMBER_PROPERTY("LeafMaterial", m_sLeafMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "Kraut-Leaf")),
    EZ_MEMBER_PROPERTY("BillboardLeaves", m_bBillboardLeaves)->AddAttributes(new ezDefaultValueAttribute(true), new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("LeafSize", m_fLeafSize)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.01f, 10.0f), new ezSuffixAttribute("m")),
    EZ_MEMBER_PROPERTY("LeafScale", m_LeafScale)->AddAttributes(new ezColorAttribute(ezColor::LightGreen), new ezCurveExtentsAttribute(0.0, true, 1.0, true), new ezClampValueAttribute(0.0, 1.0), new ezDefaultValueAttribute(1.0)),
    EZ_MEMBER_PROPERTY("LeafInterval", m_fLeafInterval)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 10.0f), new ezSuffixAttribute("m")),
    //EZ_MEMBER_PROPERTY("LeafVariationColor", m_LeafVariationColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),

    // Shared

    //ezUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
    //ezUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezKrautAssetLod, ezNoBase, 1, ezRTTIDefaultAllocator<ezKrautAssetLod>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezKrautLodMode, m_Mode)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("TipDetail", m_fTipDetail)->AddAttributes(new ezDefaultValueAttribute(0.04f), new ezClampValueAttribute(0.01f, 1.0f), new ezSuffixAttribute("m")),
    EZ_MEMBER_PROPERTY("CurvatureThreshold", m_fCurvatureThreshold)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, 90.0f)),
    EZ_MEMBER_PROPERTY("ThicknessThreshold", m_fThicknessThreshold)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("VertexRingDetail", m_fVertexRingDetail)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.01f, 2.0f), new ezSuffixAttribute("m")),
    EZ_BITFLAGS_MEMBER_PROPERTY("AllowBranch", ezKrautTreeTypeBits, m_AllowBranch),
    EZ_BITFLAGS_MEMBER_PROPERTY("AllowFrond", ezKrautTreeTypeBits, m_AllowFrond),
    EZ_BITFLAGS_MEMBER_PROPERTY("AllowLeaf", ezKrautTreeTypeBits, m_AllowLeaf),
    EZ_MEMBER_PROPERTY("MaxFrondDetail", m_iMaxFrondDetail)->AddAttributes(new ezDefaultValueAttribute(32), new ezClampValueAttribute(0, 64)),
    EZ_MEMBER_PROPERTY("FrondDetailReduction", m_iFrondDetailReduction)->AddAttributes(new ezDefaultValueAttribute(0), new ezClampValueAttribute(0, 32)),
    EZ_MEMBER_PROPERTY("LodDistance", m_uiLodDistance)->AddAttributes(new ezDefaultValueAttribute(0), new ezClampValueAttribute(0, 1000), new ezSuffixAttribute("m")),
    EZ_ENUM_MEMBER_PROPERTY("BranchSpikeTipMode", ezKrautBranchSpikeTipMode, m_BranchSpikeTipMode),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetProperties, 1, ezRTTIDefaultAllocator<ezKrautTreeAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    //EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    //EZ_MEMBER_PROPERTY("LodDistanceScale", m_fLodDistanceScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("MinAmbientOcclusion", m_fMinAmbientOcclusion)->AddAttributes(new ezDefaultValueAttribute(0.7f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("StaticColliderRadius", m_fStaticColliderRadius)->AddAttributes(new ezDefaultValueAttribute(0.4f), new ezClampValueAttribute(0.0f, 10.0f), new ezSuffixAttribute("m")),
    EZ_MEMBER_PROPERTY("TreeStiffness", m_fTreeStiffness)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(1.0f, 10000.0f)),
    EZ_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Materials)->AddAttributes(new ezContainerAttribute(false, false, false)),
    EZ_MEMBER_PROPERTY("DisplayRandomSeed", m_uiRandomSeedForDisplay),
    EZ_ARRAY_MEMBER_PROPERTY("GoodRandomSeeds", m_GoodRandomSeeds),
    EZ_MEMBER_PROPERTY("BT_Trunk1", m_BT_Trunk1),
    //EZ_MEMBER_PROPERTY("BT_Trunk2", m_BT_Trunk2),
    //EZ_MEMBER_PROPERTY("BT_Trunk3", m_BT_Trunk3),
    EZ_MEMBER_PROPERTY("BT_MainBranch1", m_BT_MainBranch1),
    EZ_MEMBER_PROPERTY("BT_MainBranch2", m_BT_MainBranch2),
    EZ_MEMBER_PROPERTY("BT_MainBranch3", m_BT_MainBranch3),
    EZ_MEMBER_PROPERTY("BT_SubBranch1", m_BT_SubBranch1),
    EZ_MEMBER_PROPERTY("BT_SubBranch2", m_BT_SubBranch2),
    EZ_MEMBER_PROPERTY("BT_SubBranch3", m_BT_SubBranch3),
    EZ_MEMBER_PROPERTY("BT_Twig1", m_BT_Twig1),
    EZ_MEMBER_PROPERTY("BT_Twig2", m_BT_Twig2),
    EZ_MEMBER_PROPERTY("BT_Twig3", m_BT_Twig3),
    EZ_MEMBER_PROPERTY("LOD0", m_Lod0),
    EZ_MEMBER_PROPERTY("LOD1", m_Lod1),
    EZ_MEMBER_PROPERTY("LOD2", m_Lod2),
    EZ_MEMBER_PROPERTY("LOD3", m_Lod3),
    EZ_MEMBER_PROPERTY("LOD4", m_Lod4),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautTreeAssetProperties::ezKrautTreeAssetProperties()
{
  // LOD 0 is enabled by default, others are disabled
  m_Lod1.m_Mode = ezKrautLodMode::Disabled;
  m_Lod2.m_Mode = ezKrautLodMode::Disabled;
  m_Lod3.m_Mode = ezKrautLodMode::Disabled;
  m_Lod4.m_Mode = ezKrautLodMode::Disabled;
}

ezKrautTreeAssetProperties::~ezKrautTreeAssetProperties() = default;

void ezKrautTreeAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezKrautAssetLod>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool bDisabled = e.m_pObject->GetTypeAccessor().GetValue("Mode").ConvertTo<ezInt32>() == ezKrautLodMode::Disabled;
    const ezPropertyUiState::Visibility lodVis = bDisabled ? ezPropertyUiState::Disabled : ezPropertyUiState::Default;

    props["TipDetail"].m_Visibility = lodVis;
    props["CurvatureThreshold"].m_Visibility = lodVis;
    props["ThicknessThreshold"].m_Visibility = lodVis;
    props["VertexRingDetail"].m_Visibility = lodVis;
    props["AllowBranch"].m_Visibility = lodVis;
    props["AllowFrond"].m_Visibility = lodVis;
    props["AllowLeaf"].m_Visibility = lodVis;
    props["MaxFrondDetail"].m_Visibility = lodVis;
    props["FrondDetailReduction"].m_Visibility = lodVis;
    props["LodDistance"].m_Visibility = lodVis;
    props["BranchSpikeTipMode"].m_Visibility = lodVis;
  }

  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezKrautAssetBranchType>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool bEnableMesh = e.m_pObject->GetTypeAccessor().GetValue("EnableMesh").ConvertTo<bool>();
    const bool bEnableFronds = e.m_pObject->GetTypeAccessor().GetValue("EnableFronds").ConvertTo<bool>();
    const bool bEnableLeaves = e.m_pObject->GetTypeAccessor().GetValue("EnableLeaves").ConvertTo<bool>();
    const bool bHasFlares = e.m_pObject->GetTypeAccessor().GetValue("Flares").ConvertTo<ezUInt32>() > 0;
    const bool bFrondContourOff = e.m_pObject->GetTypeAccessor().GetValue("FrondContourMode").ConvertTo<ezInt32>() == ezKrautFrondContourMode::Off;
    const bool bIsTrunk = e.m_pObject->GetParentProperty().StartsWith("BT_Trunk");
    const bool bTargetDir2Off = e.m_pObject->GetTypeAccessor().GetValue("TargetDir2Usage").ConvertTo<ezInt32>() == ezKrautBranchTargetDir2Usage::Off;

    // Branch Mesh properties
    props["BranchMaterial"].m_Visibility = bEnableMesh ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["BranchContour"].m_Visibility = bEnableMesh ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["Roundness"].m_Visibility = bEnableMesh ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["Flares"].m_Visibility = bEnableMesh ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FlareWidth"].m_Visibility = (bEnableMesh && bHasFlares) ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FlareWidthCurve"].m_Visibility = (bEnableMesh && bHasFlares) ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FlareRotation"].m_Visibility = (bEnableMesh && bHasFlares) ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["RotateTexCoords"].m_Visibility = (bEnableMesh && bHasFlares) ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;

    props["MaxBranchLengthParentScale"].m_Visibility = bIsTrunk ? ezPropertyUiState::Disabled : ezPropertyUiState::Default;
    props["LowerBound"].m_Visibility = bIsTrunk ? ezPropertyUiState::Disabled : ezPropertyUiState::Default;
    props["UpperBound"].m_Visibility = bIsTrunk ? ezPropertyUiState::Disabled : ezPropertyUiState::Default;
    props["TargetDir2Offset"].m_Visibility = bTargetDir2Off ? ezPropertyUiState::Disabled : ezPropertyUiState::Default;
    props["TargetDirection2"].m_Visibility = bTargetDir2Off ? ezPropertyUiState::Disabled : ezPropertyUiState::Default;

    // Frond properties
    props["FrondMaterial"].m_Visibility = bEnableFronds ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["TextureRepeat"].m_Visibility = bEnableFronds ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FrondUpOrientation"].m_Visibility = bEnableFronds ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FrondOrientationDeviation"].m_Visibility = bEnableFronds ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["NumFronds"].m_Visibility = bEnableFronds ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["AlignFrondsOnSurface"].m_Visibility = bEnableFronds ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FrondDetail"].m_Visibility = bEnableFronds ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FrondContourMode"].m_Visibility = bEnableFronds ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FrondContour"].m_Visibility = (bEnableFronds && !bFrondContourOff) ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FrondHeight"].m_Visibility = (bEnableFronds && !bFrondContourOff) ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FrondHeightScale"].m_Visibility = (bEnableFronds && !bFrondContourOff) ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FrondWidth"].m_Visibility = bEnableFronds ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["FrondWidthScale"].m_Visibility = bEnableFronds ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;

    // Leaf properties
    props["LeafMaterial"].m_Visibility = bEnableLeaves ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["BillboardLeaves"].m_Visibility = bEnableLeaves ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["LeafSize"].m_Visibility = bEnableLeaves ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["LeafScale"].m_Visibility = bEnableLeaves ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
    props["LeafInterval"].m_Visibility = bEnableLeaves ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;

    ezStringBuilder tmp1, tmp2, tmp3;

    // Sub-branch type labels: reflect the names of the branch types that would grow from this one
    const ezStringView sParentProp = e.m_pObject->GetParentProperty();
    if (bIsTrunk)
    {
      tmp1.Set("Grow ", ezKrautBranchTypeNames::MainBranch1);
      tmp2.Set("Grow ", ezKrautBranchTypeNames::MainBranch2);
      tmp3.Set("Grow ", ezKrautBranchTypeNames::MainBranch3);
    }
    else if (sParentProp.StartsWith("BT_MainBranch"))
    {
      tmp1.Set("Grow ", ezKrautBranchTypeNames::SubBranch1);
      tmp2.Set("Grow ", ezKrautBranchTypeNames::SubBranch2);
      tmp3.Set("Grow ", ezKrautBranchTypeNames::SubBranch3);
    }
    else if (sParentProp.StartsWith("BT_SubBranch"))
    {
      tmp1.Set("Grow ", ezKrautBranchTypeNames::Twig1);
      tmp2.Set("Grow ", ezKrautBranchTypeNames::Twig2);
      tmp3.Set("Grow ", ezKrautBranchTypeNames::Twig3);
    }

    if (!tmp1.IsEmpty())
    {
      props["GrowSubBranchType1"].m_sNewLabelText = tmp1;
      props["GrowSubBranchType2"].m_sNewLabelText = tmp2;
      props["GrowSubBranchType3"].m_sNewLabelText = tmp3;
    }
  }
}

static void CopyKrautCurve(Kraut::Curve& ref_dst, const ezSingleCurveData& src, ezUInt32 uiNumSamples, float fDefaultValue)
{
  if (src.m_ControlPoints.IsEmpty())
  {
    ref_dst.Initialize(1, fDefaultValue, 0.0f, 1.0f);
  }
  else
  {
    ezCurve1D c;
    src.ConvertToRuntimeData(c);
    c.SortControlPoints();
    c.CreateLinearApproximation();

    ref_dst.Initialize(uiNumSamples, fDefaultValue, 0.0f, 1.0f);

    const double invSam = 1.0 / (uiNumSamples - 1);
    for (ezUInt32 i = 0; i < uiNumSamples; ++i)
    {
      const double val = c.Evaluate(i * invSam);
      ref_dst.m_Values[i] = (float)val;
    }
  }
}

void CopyKrautConfig(Kraut::SpawnNodeDesc& ref_node, const ezKrautAssetBranchType& bt, ezDynamicArray<ezKrautMaterialDescriptor>& ref_materials, ezKrautBranchType branchType)
{
  // === Administrative ===

  ref_node.m_bAllowSubType[0] = bt.m_bGrowSubBranchType1;
  ref_node.m_bAllowSubType[1] = bt.m_bGrowSubBranchType2;
  ref_node.m_bAllowSubType[2] = bt.m_bGrowSubBranchType3;

  ref_node.m_bEnable[Kraut::BranchGeometryType::Branch] = bt.m_bEnableMesh;
  ref_node.m_bEnable[Kraut::BranchGeometryType::Frond] = bt.m_bEnableFronds;
  ref_node.m_bEnable[Kraut::BranchGeometryType::Leaf] = bt.m_bEnableLeaves;

  if (bt.m_bEnableMesh && !bt.m_sBranchMaterial.IsEmpty())
  {
    auto& m = ref_materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = ezKrautMaterialType::Branch;
    m.m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(bt.m_sBranchMaterial);
  }

  if (bt.m_bEnableFronds && !bt.m_sFrondMaterial.IsEmpty())
  {
    auto& m = ref_materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = ezKrautMaterialType::Frond;
    m.m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(bt.m_sFrondMaterial);
    // m.m_VariationColor = bt.m_FrondVariationColor;// currently done through the material
  }

  if (bt.m_bEnableLeaves && !bt.m_sLeafMaterial.IsEmpty())
  {
    auto& m = ref_materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = ezKrautMaterialType::Leaf;
    m.m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(bt.m_sLeafMaterial);
    // m.m_VariationColor = bt.m_LeafVariationColor;// currently done through the material
  }

  // === Branch Type ===

  // General

  ref_node.m_iSegmentLengthCM = ezMath::Clamp<ezInt8>(bt.m_uiSegmentLengthCM, 1, 50);
  ref_node.m_BranchTypeMode = (Kraut::BranchTypeMode::Enum)bt.m_BranchTypeMode.GetValue();
  ref_node.m_fBranchlessPartABS = bt.m_fBranchlessPartABS;
  ref_node.m_fBranchlessPartEndABS = bt.m_fBranchlessPartEndABS;
  ref_node.m_uiLowerBound = ezMath::Clamp<ezUInt8>(bt.m_uiLowerBound, 0, 100);
  ref_node.m_uiUpperBound = ezMath::Clamp<ezUInt8>(bt.m_uiUpperBound, ref_node.m_uiLowerBound, 100);
  ref_node.m_uiMinBranchThicknessInCM = ezMath::Clamp<ezUInt16>(bt.m_uiMinBranchThicknessInCM, 1, 100);
  ref_node.m_uiMaxBranchThicknessInCM = ezMath::Clamp<ezUInt16>(bt.m_uiMaxBranchThicknessInCM, ref_node.m_uiMinBranchThicknessInCM, 100);

  // Spawn Nodes

  ref_node.m_uiMinBranches = ezMath::Clamp<ezUInt16>(bt.m_uiMinBranches, 0, 32);
  ref_node.m_uiMaxBranches = ezMath::Clamp<ezUInt16>(bt.m_uiMaxBranches, ref_node.m_uiMinBranches, 32);
  ref_node.m_fNodeSpacingBefore = bt.m_fNodeSpacingBefore;
  ref_node.m_fNodeSpacingAfter = bt.m_fNodeSpacingAfter;
  ref_node.m_fNodeHeight = bt.m_fNodeHeight;


  // === Growth ===

  // Start Direction

  ref_node.m_fMaxRotationalDeviation = bt.m_MaxRotationalDeviation.GetDegree();
  ref_node.m_fBranchAngle = bt.m_BranchAngle.GetDegree();
  ref_node.m_fMaxBranchAngleDeviation = bt.m_MaxBranchAngleDeviation.GetDegree();

  // Target Direction

  ref_node.m_TargetDirection = (Kraut::BranchTargetDir::Enum)bt.m_TargetDirection.GetValue();
  ref_node.m_bTargetDirRelative = bt.m_bTargetDirRelative;
  ref_node.m_TargetDir2Usage = (Kraut::BranchTargetDir2Usage::Enum)bt.m_TargetDir2Usage.GetValue();
  ref_node.m_fTargetDir2Usage = bt.m_fTargetDir2Usage;
  ref_node.m_TargetDirection2 = (Kraut::BranchTargetDir::Enum)bt.m_TargetDirection2.GetValue();
  ref_node.m_fMaxTargetDirDeviation = bt.m_MaxTargetDirDeviation.GetDegree();

  // Growth

  ref_node.m_uiMinBranchLengthInCM = ezMath::Clamp<ezUInt16>(bt.m_uiMinBranchLengthInCM, 1, 10000);
  ref_node.m_uiMaxBranchLengthInCM = ezMath::Clamp<ezUInt16>(bt.m_uiMaxBranchLengthInCM, ref_node.m_uiMinBranchLengthInCM, 10000);
  CopyKrautCurve(ref_node.m_MaxBranchLengthParentScale, bt.m_MaxBranchLengthParentScale, 20, 1.0f);
  ref_node.m_fGrowMaxTargetDirDeviation = bt.m_GrowMaxTargetDirDeviation.GetDegree();
  ref_node.m_fGrowMaxDirChangePerSegment = bt.m_GrowMaxDirChangePerSegment.GetDegree();
  ref_node.m_bRestrictGrowthToFrondPlane = bt.m_bRestrictGrowthToFrondPlane;

  // Obstacles

  // bool m_bActAsObstacle;
  // bool m_bDoPhysicalSimulation;
  // float m_fPhysicsLookAhead;
  // float m_fPhysicsEvasionAngle;


  // === Appearance ===

  // Branch Mesh

  CopyKrautCurve(ref_node.m_BranchContour, bt.m_BranchContour, 50, 1.0f);
  ref_node.m_fRoundnessFactor = bt.m_fRoundnessFactor;
  ref_node.m_uiFlares = bt.m_uiFlares;
  ref_node.m_fFlareWidth = bt.m_fFlareWidth;
  CopyKrautCurve(ref_node.m_FlareWidthCurve, bt.m_FlareWidthCurve, 50, 1.0f);
  ref_node.m_fFlareRotation = bt.m_FlareRotation.GetDegree();
  ref_node.m_bRotateTexCoords = bt.m_bRotateTexCoords;

  // Fronds

  ref_node.m_fTextureRepeat = bt.m_fTextureRepeat;
  ref_node.m_FrondUpOrientation = (Kraut::LeafOrientation::Enum)bt.m_FrondUpOrientation.GetValue();
  ref_node.m_uiMaxFrondOrientationDeviation = (ezUInt8)bt.m_MaxFrondOrientationDeviation.GetDegree();
  ref_node.m_uiNumFronds = bt.m_uiNumFronds;
  ref_node.m_bAlignFrondsOnSurface = bt.m_bAlignFrondsOnSurface;
  ref_node.m_uiFrondDetail = bt.m_uiFrondDetail;
  CopyKrautCurve(ref_node.m_FrondContour, bt.m_FrondContour, 40, 1.0f);
  ref_node.m_FrondContourMode = (Kraut::SpawnNodeDesc::FrondContourMode)bt.m_FrondContourMode.GetValue();
  ref_node.m_fFrondHeight = (bt.m_FrondContourMode == ezKrautFrondContourMode::Off) ? 0.0f : bt.m_fFrondHeight;
  CopyKrautCurve(ref_node.m_FrondHeight, bt.m_FrondHeight, 50, 0.0f);
  ref_node.m_fFrondWidth = bt.m_fFrondWidth;
  CopyKrautCurve(ref_node.m_FrondWidth, bt.m_FrondWidth, 50, 1.0f);

  // Leaves

  ref_node.m_bBillboardLeaves = bt.m_bBillboardLeaves;
  ref_node.m_fLeafSize = bt.m_fLeafSize;
  CopyKrautCurve(ref_node.m_LeafScale, bt.m_LeafScale, 25, 1.0f);
  ref_node.m_fLeafInterval = bt.m_fLeafInterval;

  // Shared
  // ezUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  // ezUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
}
