#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <KrautGenerator/Description/SpawnNodeDesc.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Material/MaterialResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezKrautAssetMaterial, ezNoBase, 1, ezRTTIDefaultAllocator<ezKrautAssetMaterial>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new ezReadOnlyAttribute()),
    EZ_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
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
  EZ_ENUM_CONSTANT(ezKrautFrondContourMode::Full),
  EZ_ENUM_CONSTANT(ezKrautFrondContourMode::Symetric),
  EZ_ENUM_CONSTANT(ezKrautFrondContourMode::InverseSymetric),
EZ_END_STATIC_REFLECTED_ENUM

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezKrautBranchTargetDir2Usage, 1)
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir2Usage::Off),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir2Usage::Relative),
  EZ_ENUM_CONSTANT(ezKrautBranchTargetDir2Usage::Absolute),
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

    EZ_MEMBER_PROPERTY("SegmentLength", m_uiSegmentLengthCM)->AddAttributes(new ezDefaultValueAttribute(5), new ezClampValueAttribute(1, 50), new ezSuffixAttribute("cm"), new ezGroupAttribute("General")),
    EZ_ENUM_MEMBER_PROPERTY("BranchType", ezKrautBranchTypeMode, m_BranchTypeMode),
    EZ_MEMBER_PROPERTY("BranchlessPartABS", m_fBranchlessPartABS)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0, 10.0f)),
    EZ_MEMBER_PROPERTY("BranchlessPartEndABS", m_fBranchlessPartEndABS)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0, 10.0f)),
    EZ_MEMBER_PROPERTY("LowerBound", m_uiLowerBound)->AddAttributes(new ezDefaultValueAttribute(0), new ezClampValueAttribute(0, 100)),
    EZ_MEMBER_PROPERTY("UpperBound", m_uiUpperBound)->AddAttributes(new ezDefaultValueAttribute(100), new ezClampValueAttribute(0, 100)),
    EZ_MEMBER_PROPERTY("MinBranchThickness", m_uiMinBranchThicknessInCM)->AddAttributes(new ezDefaultValueAttribute(20), new ezClampValueAttribute(1, 100), new ezSuffixAttribute("cm")),
    EZ_MEMBER_PROPERTY("MaxBranchThickness", m_uiMaxBranchThicknessInCM)->AddAttributes(new ezDefaultValueAttribute(20), new ezClampValueAttribute(1, 100), new ezSuffixAttribute("cm")),

    // Spawn Nodes

    EZ_MEMBER_PROPERTY("MinBranchesPerNode", m_uiMinBranches)->AddAttributes(new ezDefaultValueAttribute(4), new ezClampValueAttribute(0, 32), new ezGroupAttribute("Spawn Nodes")),
    EZ_MEMBER_PROPERTY("MaxBranchesPerNode", m_uiMaxBranches)->AddAttributes(new ezDefaultValueAttribute(4), new ezClampValueAttribute(0, 32)),
    EZ_MEMBER_PROPERTY("NodeSpacingBefore", m_fNodeSpacingBefore)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0, 5.0f)),
    EZ_MEMBER_PROPERTY("NodeSpacingAfter", m_fNodeSpacingAfter)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0, 5.0f)),
    EZ_MEMBER_PROPERTY("NodeHeight", m_fNodeHeight)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0, 5.0f)),

    // === Growth ===

    // Start Direction
    EZ_MEMBER_PROPERTY("MaxRotationalDeviation", m_MaxRotationalDeviation)->AddAttributes(new ezDefaultValueAttribute(ezAngle()), new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(180)), new ezGroupAttribute("Start Direction")),
    EZ_MEMBER_PROPERTY("BranchAngle", m_BranchAngle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90)), new ezClampValueAttribute(ezAngle::Degree(1), ezAngle::Degree(179))),
    EZ_MEMBER_PROPERTY("MaxBranchAngleDeviation", m_MaxBranchAngleDeviation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(10)), new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(90))),

    // Target Direction
    EZ_ENUM_MEMBER_PROPERTY("TargetDirection", ezKrautBranchTargetDir, m_TargetDirection)->AddAttributes(new ezGroupAttribute("Target Direction")),
    EZ_MEMBER_PROPERTY("TargetDirRelative", m_bTargetDirRelative),
    EZ_ENUM_MEMBER_PROPERTY("TargetDir2Uage", ezKrautBranchTargetDir2Usage, m_TargetDir2Uage),
    EZ_MEMBER_PROPERTY("TargetDir2Offset", m_fTargetDir2Usage)->AddAttributes(new ezDefaultValueAttribute(2.5f), new ezClampValueAttribute(0.01f, 5.0f)),
    EZ_ENUM_MEMBER_PROPERTY("TargetDirection2", ezKrautBranchTargetDir, m_TargetDirection2),
    EZ_MEMBER_PROPERTY("MaxTargetDirDeviation", m_MaxTargetDirDeviation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(20)), new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(90))),

    // Growth
    EZ_MEMBER_PROPERTY("MinBranchLength", m_uiMinBranchLengthInCM)->AddAttributes(new ezDefaultValueAttribute(100), new ezClampValueAttribute(1, 10000), new ezSuffixAttribute("cm"), new ezGroupAttribute("Branch Growth")),
    EZ_MEMBER_PROPERTY("MaxBranchLength", m_uiMaxBranchLengthInCM)->AddAttributes(new ezDefaultValueAttribute(100), new ezClampValueAttribute(1, 10000), new ezSuffixAttribute("cm")),
    //Kraut::Curve m_MaxBranchLengthParentScale;
    EZ_MEMBER_PROPERTY("TargetDirDeviation", m_GrowMaxTargetDirDeviation)->AddAttributes(new ezDefaultValueAttribute(ezAngle()), new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(180))),
    EZ_MEMBER_PROPERTY("DirChangePerSegment", m_GrowMaxDirChangePerSegment)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(5)), new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(90))),
    EZ_MEMBER_PROPERTY("OnlyGrowUpAndDown", m_bRestrictGrowthToFrondPlane),

    // Obstacles

    //bool m_bActAsObstacle;
    //bool m_bDoPhysicalSimulation;
    //float m_fPhysicsLookAhead;
    //float m_fPhysicsEvasionAngle;

    // === Appearance ===

    // Branch Mesh

    EZ_MEMBER_PROPERTY("EnableMesh", m_bEnableMesh)->AddAttributes(new ezDefaultValueAttribute(true), new ezGroupAttribute("Branch Mesh")),
    EZ_MEMBER_PROPERTY("BranchMaterial", m_sBranchMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    //Kraut::Curve m_BranchContour;
    EZ_MEMBER_PROPERTY("BranchContour", m_BranchContour),
    EZ_MEMBER_PROPERTY("Roundness", m_fRoundnessFactor)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("Flares", m_uiFlares)->AddAttributes(new ezDefaultValueAttribute(0), new ezClampValueAttribute(0, 16)),
    EZ_MEMBER_PROPERTY("FlareWidth", m_fFlareWidth)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    //Kraut::Curve m_FlareWidthCurve;
    EZ_MEMBER_PROPERTY("FlareRotation", m_FlareRotation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(0)), new ezClampValueAttribute(ezAngle::Degree(-720), ezAngle::Degree(720))),
    EZ_MEMBER_PROPERTY("RotateTexCoords", m_bRotateTexCoords)->AddAttributes(new ezDefaultValueAttribute(true)),

    // Fronds

    EZ_MEMBER_PROPERTY("EnableFronds", m_bEnableFronds)->AddAttributes(new ezGroupAttribute("Fronds")),
    EZ_MEMBER_PROPERTY("FrondMaterial", m_sFrondMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_MEMBER_PROPERTY("TextureRepeat", m_fTextureRepeat)->AddAttributes(new ezClampValueAttribute(0.0f, 99.0f)),
    EZ_ENUM_MEMBER_PROPERTY("FrondUpOrientation", ezKrautLeafOrientation, m_FrondUpOrientation),
    EZ_MEMBER_PROPERTY("FrondOrientationDeviation", m_MaxFrondOrientationDeviation)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(180))),
    EZ_MEMBER_PROPERTY("NumFronds", m_uiNumFronds)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("AlignFrondsOnSurface", m_bAlignFrondsOnSurface),
    EZ_MEMBER_PROPERTY("FrondDetail", m_uiFrondDetail)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 32)),
    //Kraut::Curve m_FrondContour;
    EZ_ENUM_MEMBER_PROPERTY("FrondContourMode", ezKrautFrondContourMode, m_FrondContourMode),
    EZ_MEMBER_PROPERTY("FrondHeight", m_fFrondHeight)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 10.0f)),
    //Kraut::Curve m_FrondHeight;
    EZ_MEMBER_PROPERTY("FrondWidth", m_fFrondWidth)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 10.0f)),
    //Kraut::Curve m_FrondWidth;
    //EZ_MEMBER_PROPERTY("FrondVariationColor", m_FrondVariationColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),

    // Leaves

    EZ_MEMBER_PROPERTY("EnableLeaves", m_bEnableLeaves)->AddAttributes(new ezGroupAttribute("Leaves")),
    EZ_MEMBER_PROPERTY("LeafMaterial", m_sLeafMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_MEMBER_PROPERTY("BillboardLeaves", m_bBillboardLeaves)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("LeafSize", m_fLeafSize)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.01f, 10.0f)),
    //Kraut::Curve m_LeafScale;
    EZ_MEMBER_PROPERTY("LeafInterval", m_fLeafInterval)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    //EZ_MEMBER_PROPERTY("LeafVariationColor", m_LeafVariationColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),

    // Shared

    //ezString m_sTexture[Kraut::BranchGeometryType::ENUM_COUNT];
    //ezUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
    //ezUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetProperties, 1, ezRTTIDefaultAllocator<ezKrautTreeAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    //EZ_MEMBER_PROPERTY("KrautFile", m_sKrautFile)->AddAttributes(new ezFileBrowserAttribute("Select Kraut Tree file", "*.tree")),
    //EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    //EZ_MEMBER_PROPERTY("LodDistanceScale", m_fLodDistanceScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("StaticColliderRadius", m_fStaticColliderRadius)->AddAttributes(new ezDefaultValueAttribute(0.4f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("TreeStiffness", m_fTreeStiffness)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(1.0f, 10000.0f)),
    EZ_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface")),
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
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautTreeAssetProperties::ezKrautTreeAssetProperties() = default;
ezKrautTreeAssetProperties::~ezKrautTreeAssetProperties() = default;

void CopyConfig(Kraut::SpawnNodeDesc& nd, const ezKrautAssetBranchType& bt, ezDynamicArray<ezKrautMaterialDescriptor>& materials, ezKrautBranchType branchType)
{
  // === Administrative ===

  //bool m_bVisible;

  nd.m_bAllowSubType[0] = bt.m_bGrowSubBranchType1;
  nd.m_bAllowSubType[1] = bt.m_bGrowSubBranchType2;
  nd.m_bAllowSubType[2] = bt.m_bGrowSubBranchType3;

  nd.m_bEnable[Kraut::BranchGeometryType::Branch] = bt.m_bEnableMesh;
  nd.m_bEnable[Kraut::BranchGeometryType::Frond] = bt.m_bEnableFronds;
  nd.m_bEnable[Kraut::BranchGeometryType::Leaf] = bt.m_bEnableLeaves;

  if (bt.m_bEnableMesh && !bt.m_sBranchMaterial.IsEmpty())
  {
    auto& m = materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = ezKrautMaterialType::Branch;
    m.m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(bt.m_sBranchMaterial);
  }

  if (bt.m_bEnableFronds && !bt.m_sFrondMaterial.IsEmpty())
  {
    auto& m = materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = ezKrautMaterialType::Frond;
    m.m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(bt.m_sFrondMaterial);
    //m.m_VariationColor = bt.m_FrondVariationColor;// currently done through the material
  }

  if (bt.m_bEnableLeaves && !bt.m_sLeafMaterial.IsEmpty())
  {
    auto& m = materials.ExpandAndGetRef();
    m.m_BranchType = branchType;
    m.m_MaterialType = ezKrautMaterialType::Leaf;
    m.m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(bt.m_sLeafMaterial);
    //m.m_VariationColor = bt.m_LeafVariationColor;// currently done through the material
  }

  // === Branch Type ===

  // General

  nd.m_iSegmentLengthCM = ezMath::Clamp<ezInt8>(bt.m_uiSegmentLengthCM, 1, 50);
  nd.m_BranchTypeMode = (Kraut::BranchTypeMode::Enum)bt.m_BranchTypeMode.GetValue();
  nd.m_fBranchlessPartABS = bt.m_fBranchlessPartABS;
  nd.m_fBranchlessPartEndABS = bt.m_fBranchlessPartEndABS;
  nd.m_uiLowerBound = ezMath::Clamp<ezUInt8>(bt.m_uiLowerBound, 0, 100);
  nd.m_uiUpperBound = ezMath::Clamp<ezUInt8>(bt.m_uiUpperBound, nd.m_uiLowerBound, 100);
  nd.m_uiMinBranchThicknessInCM = ezMath::Clamp<ezUInt16>(bt.m_uiMinBranchThicknessInCM, 1, 100);
  nd.m_uiMaxBranchThicknessInCM = ezMath::Clamp<ezUInt16>(bt.m_uiMaxBranchThicknessInCM, nd.m_uiMinBranchThicknessInCM, 100);

  // Spawn Nodes

  nd.m_uiMinBranches = ezMath::Clamp<ezUInt16>(bt.m_uiMinBranches, 0, 32);
  nd.m_uiMaxBranches = ezMath::Clamp<ezUInt16>(bt.m_uiMaxBranches, nd.m_uiMinBranches, 32);
  nd.m_fNodeSpacingBefore = bt.m_fNodeSpacingBefore;
  nd.m_fNodeSpacingAfter = bt.m_fNodeSpacingAfter;
  nd.m_fNodeHeight = bt.m_fNodeHeight;


  // === Growth ===

  // Start Direction

  nd.m_fMaxRotationalDeviation = bt.m_MaxRotationalDeviation.GetDegree();
  nd.m_fBranchAngle = bt.m_BranchAngle.GetDegree();
  nd.m_fMaxBranchAngleDeviation = bt.m_MaxBranchAngleDeviation.GetDegree();

  // Target Direction

  nd.m_TargetDirection = (Kraut::BranchTargetDir::Enum)bt.m_TargetDirection.GetValue();
  nd.m_bTargetDirRelative = bt.m_bTargetDirRelative;
  nd.m_TargetDir2Uage = (Kraut::BranchTargetDir2Usage::Enum)bt.m_TargetDir2Uage.GetValue();
  nd.m_fTargetDir2Usage = bt.m_fTargetDir2Usage;
  nd.m_TargetDirection2 = (Kraut::BranchTargetDir::Enum)bt.m_TargetDirection2.GetValue();
  nd.m_fMaxTargetDirDeviation = bt.m_MaxTargetDirDeviation.GetDegree();

  // Growth

  nd.m_uiMinBranchLengthInCM = ezMath::Clamp<ezUInt16>(bt.m_uiMinBranchLengthInCM, 1, 10000);
  nd.m_uiMaxBranchLengthInCM = ezMath::Clamp<ezUInt16>(bt.m_uiMaxBranchLengthInCM, nd.m_uiMinBranchLengthInCM, 10000);
  //Kraut::Curve m_MaxBranchLengthParentScale;
  nd.m_fGrowMaxTargetDirDeviation = bt.m_GrowMaxTargetDirDeviation.GetDegree();
  nd.m_fGrowMaxDirChangePerSegment = bt.m_GrowMaxDirChangePerSegment.GetDegree();
  nd.m_bRestrictGrowthToFrondPlane = bt.m_bRestrictGrowthToFrondPlane;

  // Obstacles

  //bool m_bActAsObstacle;
  //bool m_bDoPhysicalSimulation;
  //float m_fPhysicsLookAhead;
  //float m_fPhysicsEvasionAngle;


  // === Appearance ===

  // Branch Mesh

  //Kraut::Curve m_BranchContour;
  nd.m_fRoundnessFactor = bt.m_fRoundnessFactor;
  nd.m_uiFlares = bt.m_uiFlares;
  nd.m_fFlareWidth = bt.m_fFlareWidth;
  //Kraut::Curve m_FlareWidthCurve;
  nd.m_fFlareRotation = bt.m_FlareRotation.GetDegree();
  nd.m_bRotateTexCoords = bt.m_bRotateTexCoords;

  // Fronds

  nd.m_fTextureRepeat = bt.m_fTextureRepeat;
  nd.m_FrondUpOrientation = (Kraut::LeafOrientation::Enum)bt.m_FrondUpOrientation.GetValue();
  nd.m_uiMaxFrondOrientationDeviation = (ezUInt8)bt.m_MaxFrondOrientationDeviation.GetDegree();
  nd.m_uiNumFronds = bt.m_uiNumFronds;
  nd.m_bAlignFrondsOnSurface = bt.m_bAlignFrondsOnSurface;
  nd.m_uiFrondDetail = bt.m_uiFrondDetail;
  //Kraut::Curve m_FrondContour;
  nd.m_FrondContourMode = (Kraut::SpawnNodeDesc::FrondContourMode)bt.m_FrondContourMode.GetValue();
  nd.m_fFrondHeight = bt.m_fFrondHeight;
  //Kraut::Curve m_FrondHeight;
  nd.m_fFrondWidth = bt.m_fFrondWidth;
  //Kraut::Curve m_FrondWidth;
  //ezColorGammaUB col = bt.m_FrondVariationColor;
  //nd.m_uiVariationColor[Kraut::BranchGeometryType::Frond][0] = col.r;
  //nd.m_uiVariationColor[Kraut::BranchGeometryType::Frond][1] = col.g;
  //nd.m_uiVariationColor[Kraut::BranchGeometryType::Frond][2] = col.b;
  //nd.m_uiVariationColor[Kraut::BranchGeometryType::Frond][3] = col.a;

  // Leaves

  nd.m_bBillboardLeaves = bt.m_bBillboardLeaves;
  nd.m_fLeafSize = bt.m_fLeafSize;
  //Kraut::Curve m_LeafScale;
  nd.m_fLeafInterval = bt.m_fLeafInterval;
  //col = bt.m_LeafVariationColor;
  //nd.m_uiVariationColor[Kraut::BranchGeometryType::Leaf][0] = col.r;
  //nd.m_uiVariationColor[Kraut::BranchGeometryType::Leaf][1] = col.g;
  //nd.m_uiVariationColor[Kraut::BranchGeometryType::Leaf][2] = col.b;
  //nd.m_uiVariationColor[Kraut::BranchGeometryType::Leaf][3] = col.a;

  // Shared
  //ezString m_sTexture[Kraut::BranchGeometryType::ENUM_COUNT];
  //ezUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  //ezUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
}
