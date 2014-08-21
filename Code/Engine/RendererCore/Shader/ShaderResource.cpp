#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Shader/Helper.h>
#include <Foundation/Logging/Log.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezShaderResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezShaderResource::ezShaderResource()
{
}

void ezShaderResource::UnloadData(bool bFullUnload)
{
  m_uiLoadedQualityLevel = 0;
  m_PermutationVarsUsed.Clear();
  m_LoadingState = ezResourceLoadState::Uninitialized;
}

void ezShaderResource::UpdateContent(ezStreamReaderBase& Stream)
{
  ezString sContent;
  sContent.ReadAll(Stream);

  ezTextSectionizer Sections;
  GetShaderSections(sContent.GetData(), Sections);

  m_PermutationVarsUsed = Sections.GetSectionContent(ezShaderSections::PERMUTATIONS);

  m_LoadingState = ezResourceLoadState::Loaded;
  m_uiLoadedQualityLevel = 1;
  m_uiMaxQualityLevel = 1;
}

void ezShaderResource::UpdateMemoryUsage()
{
  SetMemoryUsageCPU(m_PermutationVarsUsed.GetElementCount());
  SetMemoryUsageGPU(0);
}

void ezShaderResource::CreateResource(const ezShaderResourceDescriptor& descriptor)
{

}

//ezResourceManager::SetResourceTypeLoader<ColorResource>(&g_ColorLoader);

//ezShaderResourceLoader::ezShaderResourceLoader()
//{
//}
//
//ezResourceLoadData ezShaderResourceLoader::OpenDataStream(const ezResourceBase* pResource)
//{
//}
//
//void ezShaderResourceLoader::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
//{
//}
