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
    EZ_MEMBER_PROPERTY("Mesh File", m_sMeshFile),
    EZ_MEMBER_PROPERTY("Mesh Scaling", m_fMeshScaling),
    EZ_ENUM_MEMBER_PROPERTY("Forward Dir", ezBasisAxis, m_ForwardDir),
    EZ_ENUM_MEMBER_PROPERTY("Right Dir", ezBasisAxis, m_RightDir),
    EZ_ENUM_MEMBER_PROPERTY("Up Dir", ezBasisAxis, m_UpDir),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new ezContainerAttribute(false, false, true)),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMeshAssetProperties::ezMeshAssetProperties()
{
  m_uiVertices = 0;
  m_uiTriangles = 0;
  m_ForwardDir = ezBasisAxis::PositiveX;
  m_RightDir = ezBasisAxis::PositiveY;
  m_UpDir = ezBasisAxis::PositiveZ;
  m_fMeshScaling = 1.0f;
}

const ezString& ezMeshAssetProperties::GetResourceSlotProperty(ezUInt32 uiSlot) const
{
  uiSlot %= m_Slots.GetCount();
  return m_Slots[uiSlot].m_sResource;
}