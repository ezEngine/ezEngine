#include <RendererCore/PCH.h>
#include <RendererCore/Material/MaterialResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezMaterialResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMaterialResource::ezMaterialResource()
{
}

void ezMaterialResource::UnloadData(bool bFullUnload)
{

}

void ezMaterialResource::UpdateContent(ezStreamReaderBase& Stream)
{

}

void ezMaterialResource::UpdateMemoryUsage()
{

}

void ezMaterialResource::CreateResource(const ezMaterialResourceDescriptor& descriptor)
{
  m_hShader = descriptor.m_hShader;

  m_uiMaxQualityLevel = 1;
  m_uiLoadedQualityLevel = 1;

  m_LoadingState = ezResourceLoadState::Loaded;
}
