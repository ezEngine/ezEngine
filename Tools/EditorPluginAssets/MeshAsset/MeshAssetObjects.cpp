#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezResourceSlot, ezNoBase, 1, ezRTTIDefaultAllocator<ezResourceSlot>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new ezReadOnlyAttribute()),
EZ_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new ezAssetBrowserAttribute("Material")),
EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetProperties, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezMeshAssetProperties>);
  EZ_BEGIN_PROPERTIES
    EZ_ENUM_MEMBER_PROPERTY("Primitive Type", ezMeshPrimitive, m_PrimitiveType),
    EZ_ENUM_MEMBER_PROPERTY("Forward Dir", ezBasisAxis, m_ForwardDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveX)),
    EZ_ENUM_MEMBER_PROPERTY("Right Dir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_ENUM_MEMBER_PROPERTY("Up Dir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveZ)),
    EZ_MEMBER_PROPERTY("Uniform Scaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Non-Uniform Scaling", m_vNonUniformScaling)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f))),
    EZ_MEMBER_PROPERTY("Mesh File", m_sMeshFile),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius),
    EZ_MEMBER_PROPERTY("Radius 2", m_fRadius2),
    EZ_MEMBER_PROPERTY("Height", m_fHeight),
    EZ_MEMBER_PROPERTY("Detail", m_uiDetail),
    EZ_MEMBER_PROPERTY("Detail 2", m_uiDetail2),
    EZ_MEMBER_PROPERTY("Cap", m_bCap),
    EZ_MEMBER_PROPERTY("Cap 2", m_bCap2),
    EZ_MEMBER_PROPERTY("Angle", m_fAngle)->AddAttributes(new ezDefaultValueAttribute(360.0f)),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new ezContainerAttribute(false, false, true)),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezMeshPrimitive, 1)
  EZ_ENUM_CONSTANT(ezMeshPrimitive::File),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Box),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Rect),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Cylinder),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Cone),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Pyramid),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Sphere),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::HalfSphere),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::GeodesicSphere),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Capsule),
  EZ_ENUM_CONSTANT(ezMeshPrimitive::Torus),
EZ_END_STATIC_REFLECTED_ENUM();

ezMeshAssetProperties::ezMeshAssetProperties()
{
  m_uiVertices = 0;
  m_uiTriangles = 0;
  m_ForwardDir = ezBasisAxis::PositiveX;
  m_RightDir = ezBasisAxis::PositiveY;
  m_UpDir = ezBasisAxis::PositiveZ;
  m_fUniformScaling = 1.0f;
  m_fRadius = 0.5f;
  m_fHeight = 1.0f;
}

const ezString& ezMeshAssetProperties::GetResourceSlotProperty(ezUInt32 uiSlot) const
{
  uiSlot %= m_Slots.GetCount();
  return m_Slots[uiSlot].m_sResource;
}