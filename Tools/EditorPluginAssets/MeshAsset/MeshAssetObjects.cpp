#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetProperties, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Mesh File", m_sMeshFile),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMeshAssetProperties::ezMeshAssetProperties()
{
}

