#include <PCH.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCollisionMeshType, 1)
  EZ_ENUM_CONSTANT(ezCollisionMeshType::ConvexHull),
  EZ_ENUM_CONSTANT(ezCollisionMeshType::TriangleMesh),
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetProperties, 1, ezRTTIDefaultAllocator<ezCollisionMeshAssetProperties>);
  EZ_BEGIN_PROPERTIES
    EZ_ENUM_MEMBER_PROPERTY("Forward Dir", ezBasisAxis, m_ForwardDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveX)),
    EZ_ENUM_MEMBER_PROPERTY("Right Dir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_ENUM_MEMBER_PROPERTY("Up Dir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveZ)),
    EZ_ENUM_MEMBER_PROPERTY("Mesh Type", ezCollisionMeshType, m_MeshType),
    EZ_MEMBER_PROPERTY("Uniform Scaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Non-Uniform Scaling", m_vNonUniformScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f))),
    EZ_MEMBER_PROPERTY("Mesh File", m_sMeshFile),
    //EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new ezContainerAttribute(false, false, true)),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezCollisionMeshAssetProperties::ezCollisionMeshAssetProperties()
{
  m_uiVertices = 0;
  m_uiTriangles = 0;
  m_ForwardDir = ezBasisAxis::PositiveX;
  m_RightDir = ezBasisAxis::PositiveY;
  m_UpDir = ezBasisAxis::PositiveZ;
  m_fUniformScaling = 1.0f;
  m_MeshType = ezCollisionMeshType::ConvexHull;
}

//const ezString& ezCollisionMeshAssetProperties::GetResourceSlotProperty(ezUInt32 uiSlot) const
//{
//  uiSlot %= m_Slots.GetCount();
//  return m_Slots[uiSlot].m_sResource;
//}