#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshResource.h>

#include <Core/ResourceManager/ResourceManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezMeshResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

void ezMeshResource::UnloadData(bool bFullUnload)
{
  m_Parts.Clear();
  m_uiMaterialCount = 0;

  m_LoadingState = ezResourceLoadState::Uninitialized;
}

void ezMeshResource::UpdateContent(ezStreamReaderBase* Stream)
{

}

void ezMeshResource::UpdateMemoryUsage()
{

}

void ezMeshResource::CreateResource(const ezMeshResourceDescriptor& descriptor)
{
  /// \todo proper implementation
  ezResourceLock<ezMeshBufferResource> pMeshBuffer(descriptor.hMeshBuffer);

  Part part;
  part.m_uiPrimitiveCount = pMeshBuffer->GetPrimitiveCount();
  part.m_uiFirstPrimitive = 0;  
  part.m_uiMaterialIndex = 0;
  part.m_hMeshBuffer = descriptor.hMeshBuffer;

  m_Parts.PushBack(part);

  m_uiMaterialCount = 1;

  m_uiMaxQualityLevel = 1;
  m_uiLoadedQualityLevel = 1;

  m_LoadingState = ezResourceLoadState::Loaded;
}
