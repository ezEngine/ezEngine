#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezJoltSurfaceResourceSlot, ezNoBase, 1, ezRTTIDefaultAllocator<ezJoltSurfaceResourceSlot>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new ezReadOnlyAttribute()),
    EZ_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("Exclude", m_bExclude),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltCollisionMeshType, 2)
  EZ_ENUM_CONSTANT(ezJoltCollisionMeshType::ConvexHull),
  EZ_ENUM_CONSTANT(ezJoltCollisionMeshType::TriangleMesh),
  EZ_ENUM_CONSTANT(ezJoltCollisionMeshType::Cylinder),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltConvexCollisionMeshType, 1)
  EZ_ENUM_CONSTANT(ezJoltConvexCollisionMeshType::ConvexHull),
  EZ_ENUM_CONSTANT(ezJoltConvexCollisionMeshType::Cylinder),
  EZ_ENUM_CONSTANT(ezJoltConvexCollisionMeshType::ConvexDecomposition),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltCollisionMeshAssetProperties, 2, ezRTTIDefaultAllocator<ezJoltCollisionMeshAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("ImportTransform", ezMeshImportTransform, m_ImportTransform),
    EZ_ENUM_MEMBER_PROPERTY("RightDir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::NegativeX)),
    EZ_ENUM_MEMBER_PROPERTY("UpDir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("IsConvexMesh", m_bIsConvexMesh)->AddAttributes(new ezHiddenAttribute()),
    EZ_ENUM_MEMBER_PROPERTY("ConvexMeshType", ezJoltConvexCollisionMeshType, m_ConvexMeshType),
    EZ_MEMBER_PROPERTY("MaxConvexPieces", m_uiMaxConvexPieces)->AddAttributes(new ezDefaultValueAttribute(5)),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Radius2", m_fRadius2)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 32)),
    EZ_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", ezFileBrowserAttribute::Meshes)),
    EZ_ARRAY_MEMBER_PROPERTY("Surfaces", m_Slots)->AddAttributes(new ezContainerAttribute(false, false, true)),
    EZ_MEMBER_PROPERTY("Surface", m_sConvexMeshSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),

    EZ_MEMBER_PROPERTY("SimplifyMesh", m_bSimplifyMesh),
    EZ_MEMBER_PROPERTY("MeshSimplification", m_uiMeshSimplification)->AddAttributes(new ezDefaultValueAttribute(50), new ezClampValueAttribute(1, 100)),
    EZ_MEMBER_PROPERTY("MaxSimplificationError", m_uiMaxSimplificationError)->AddAttributes(new ezDefaultValueAttribute(20), new ezClampValueAttribute(1, 100)),
    EZ_MEMBER_PROPERTY("AggressiveSimplification", m_bAggressiveSimplification),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltCollisionMeshAssetProperties::ezJoltCollisionMeshAssetProperties() = default;
ezJoltCollisionMeshAssetProperties::~ezJoltCollisionMeshAssetProperties() = default;

void ezJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezJoltCollisionMeshAssetProperties>())
    return;

  const bool bSimplify = e.m_pObject->GetTypeAccessor().GetValue("SimplifyMesh").ConvertTo<bool>();
  const bool isConvex = e.m_pObject->GetTypeAccessor().GetValue("IsConvexMesh").ConvertTo<bool>();
  const ezInt64 meshType = e.m_pObject->GetTypeAccessor().GetValue("ConvexMeshType").ConvertTo<ezInt64>();

  auto& props = *e.m_pPropertyStates;

  props["Radius"].m_Visibility = ezPropertyUiState::Invisible;
  props["Radius2"].m_Visibility = ezPropertyUiState::Invisible;
  props["Height"].m_Visibility = ezPropertyUiState::Invisible;
  props["Detail"].m_Visibility = ezPropertyUiState::Invisible;
  props["MeshFile"].m_Visibility = ezPropertyUiState::Invisible;
  props["ConvexMeshType"].m_Visibility = ezPropertyUiState::Invisible;
  props["MaxConvexPieces"].m_Visibility = ezPropertyUiState::Invisible;
  props["Surfaces"].m_Visibility = isConvex ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
  props["Surface"].m_Visibility = isConvex ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

  props["MeshSimplification"].m_Visibility = bSimplify ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  props["MaxSimplificationError"].m_Visibility = bSimplify ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  props["AggressiveSimplification"].m_Visibility = bSimplify ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

  const ezInt64 importTransform = e.m_pObject->GetTypeAccessor().GetValue("ImportTransform").ConvertTo<ezInt64>();
  const bool bCustomTransform = importTransform == 127;
  props["RightDir"].m_Visibility = bCustomTransform ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  props["UpDir"].m_Visibility = bCustomTransform ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  props["FlipForwardDir"].m_Visibility = bCustomTransform ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

  if (!isConvex)
  {
    props["MeshFile"].m_Visibility = ezPropertyUiState::Default;
  }
  else
  {
    props["ConvexMeshType"].m_Visibility = ezPropertyUiState::Default;

    switch (meshType)
    {
      case ezJoltConvexCollisionMeshType::ConvexDecomposition:
        props["MaxConvexPieces"].m_Visibility = ezPropertyUiState::Default;
        [[fallthrough]];

      case ezJoltConvexCollisionMeshType::ConvexHull:
        props["MeshFile"].m_Visibility = ezPropertyUiState::Default;
        break;

      case ezJoltConvexCollisionMeshType::Cylinder:
        props["Radius"].m_Visibility = ezPropertyUiState::Default;
        props["Radius2"].m_Visibility = ezPropertyUiState::Default;
        props["Height"].m_Visibility = ezPropertyUiState::Default;
        props["Detail"].m_Visibility = ezPropertyUiState::Default;
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezJoltCollisionMeshAssetProperties_1_2 : public ezGraphPatch
{
public:
  ezJoltCollisionMeshAssetProperties_1_2()
    : ezGraphPatch("ezJoltCollisionMeshAssetProperties", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("ImportTransform", 127);
  }
};

ezJoltCollisionMeshAssetProperties_1_2 g_ezJoltCollisionMeshAssetProperties_1_2;
