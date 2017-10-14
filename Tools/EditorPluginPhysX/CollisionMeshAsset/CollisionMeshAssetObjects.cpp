#include <PCH.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezSurfaceResourceSlot, ezNoBase, 1, ezRTTIDefaultAllocator<ezSurfaceResourceSlot>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new ezReadOnlyAttribute()),
    EZ_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCollisionMeshType, 2)
EZ_ENUM_CONSTANT(ezCollisionMeshType::ConvexHull),
EZ_ENUM_CONSTANT(ezCollisionMeshType::TriangleMesh),
EZ_ENUM_CONSTANT(ezCollisionMeshType::Cylinder),
//EZ_ENUM_CONSTANT(ezCollisionMeshType::Tetraeder),
//EZ_ENUM_CONSTANT(ezCollisionMeshType::Oktaeder),
//EZ_ENUM_CONSTANT(ezCollisionMeshType::Pyramid),
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetProperties, 1, ezRTTIDefaultAllocator<ezCollisionMeshAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("ForwardDir", ezBasisAxis, m_ForwardDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::NegativeZ)),
    EZ_ENUM_MEMBER_PROPERTY("RightDir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveX)),
    EZ_ENUM_MEMBER_PROPERTY("UpDir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("NonUniformScaling", m_vNonUniformScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f))),
    EZ_ENUM_MEMBER_PROPERTY("MeshType", ezCollisionMeshType, m_MeshType),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Radius2", m_fRadius2)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 32)),
    EZ_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", "*.obj;*.fbx;*.ply")),
    EZ_MEMBER_PROPERTY("SubmeshName", m_sSubMeshName),
    EZ_ARRAY_MEMBER_PROPERTY("Surfaces", m_Slots)->AddAttributes(new ezContainerAttribute(false, false, true)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezCollisionMeshAssetProperties::ezCollisionMeshAssetProperties()
{
  m_uiVertices = 0;
  m_uiTriangles = 0;
  m_ForwardDir = ezBasisAxis::NegativeZ;
  m_RightDir = ezBasisAxis::PositiveX;
  m_UpDir = ezBasisAxis::PositiveY;
  m_fUniformScaling = 1.0f;
  m_vNonUniformScaling.Set(1.0f);
  m_fRadius = 0.5f;
  m_fRadius2 = 0.5f;
  m_fHeight = 1.0f;
  m_uiDetail = 1;
}

const ezString ezCollisionMeshAssetProperties::GetResourceSlotProperty(ezUInt32 uiSlot) const
{
  if (m_Slots.IsEmpty())
    return "";

  uiSlot %= m_Slots.GetCount();
  return m_Slots[uiSlot].m_sResource;
}

void ezCollisionMeshAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() != ezRTTI::FindTypeByName("ezCollisionMeshAssetProperties"))
    return;

  ezInt64 meshType = e.m_pObject->GetTypeAccessor().GetValue("MeshType").ConvertTo<ezInt64>();

  auto& props = *e.m_pPropertyStates;

  props["Radius"].m_Visibility = ezPropertyUiState::Invisible;
  props["Radius2"].m_Visibility = ezPropertyUiState::Invisible;
  props["Height"].m_Visibility = ezPropertyUiState::Invisible;
  props["Detail"].m_Visibility = ezPropertyUiState::Invisible;
  props["MeshFile"].m_Visibility = ezPropertyUiState::Invisible;
  props["SubmeshName"].m_Visibility = ezPropertyUiState::Invisible;

  switch (meshType)
  {
  case ezCollisionMeshType::ConvexHull:
  case ezCollisionMeshType::TriangleMesh:
    props["MeshFile"].m_Visibility = ezPropertyUiState::Default;
    props["SubmeshName"].m_Visibility = ezPropertyUiState::Default;
    break;

  case ezCollisionMeshType::Cylinder:
    props["Radius"].m_Visibility = ezPropertyUiState::Default;
    props["Radius2"].m_Visibility = ezPropertyUiState::Default;
    props["Height"].m_Visibility = ezPropertyUiState::Default;
    props["Detail"].m_Visibility = ezPropertyUiState::Default;
    break;

  //case ezCollisionMeshType::Oktaeder:
  //case ezCollisionMeshType::Pyramid:
  //case ezCollisionMeshType::Tetraeder:
  //  break;
  }
}



