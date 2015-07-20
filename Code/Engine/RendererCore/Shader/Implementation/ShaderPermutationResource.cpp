#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/MemoryStream.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderPermutationResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezShaderPermutationResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

static ezShaderPermutationResourceLoader g_PermutationResourceLoader;

ezShaderPermutationResource::ezShaderPermutationResource() : ezResource<ezShaderPermutationResource, ezShaderPermutationResourceDescriptor>(DoUpdate::OnMainThread, 1)
{
  m_bShaderPermutationValid = false;

  for (ezUInt32 e = ezGALShaderStage::VertexShader; e < ezGALShaderStage::ENUM_COUNT; ++e)
    m_pShaderStageBinaries[e] = nullptr;
}

ezResourceLoadDesc ezShaderPermutationResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderPermutationValid = false;

  auto pDevice = ezGALDevice::GetDefaultDevice();

  if (!m_hShader.IsInvalidated())
  {
    pDevice->DestroyShader(m_hShader);
    m_hShader.Invalidate();
  }

  if (!m_hBlendState.IsInvalidated())
  {
    pDevice->DestroyBlendState(m_hBlendState);
    m_hBlendState.Invalidate();
  }

  if (!m_hDepthStencilState.IsInvalidated())
  {
    pDevice->DestroyDepthStencilState(m_hDepthStencilState);
    m_hDepthStencilState.Invalidate();
  }

  if (!m_hRasterizerState.IsInvalidated())
  {
    pDevice->DestroyRasterizerState(m_hRasterizerState);
    m_hRasterizerState.Invalidate();
  }


  ezResourceLoadDesc res;
  res.m_State = ezResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}

ezResourceLoadDesc ezShaderPermutationResource::UpdateContent(ezStreamReaderBase* Stream)
{
  ezUInt32 uiGPUMem = 0;
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  m_bShaderPermutationValid = false;

  ezResourceLoadDesc res;
  res.m_State = ezResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    ezLog::Error("Shader Permutation '%s': Data is not available", GetResourceID().GetData());
    return res;
  }

  ezShaderPermutationBinary PermutationBinary;

  if (PermutationBinary.Read(*Stream).Failed())
  {
    ezLog::Error("Shader Permutation '%s': Could not read shader permutation binary", GetResourceID().GetData());
    return res;
  }

  auto pDevice = ezGALDevice::GetDefaultDevice();

  // get the shader render state object
  {
    m_hBlendState = pDevice->CreateBlendState(PermutationBinary.m_StateDescriptor.m_BlendDesc);
    m_hDepthStencilState = pDevice->CreateDepthStencilState(PermutationBinary.m_StateDescriptor.m_DepthStencilDesc);
    m_hRasterizerState = pDevice->CreateRasterizerState(PermutationBinary.m_StateDescriptor.m_RasterizerDesc);
  }

  ezGALShaderCreationDescription ShaderDesc;

  // iterate over all shader stages, add them to the descriptor
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    const ezUInt32 uiStageHash = PermutationBinary.m_uiShaderStageHashes[stage];

    if (uiStageHash == 0) // not used
      continue;

    ezShaderStageBinary* pStageBin = ezShaderStageBinary::LoadStageBinary((ezGALShaderStage::Enum) stage, uiStageHash);

    if (pStageBin == nullptr)
    {
      ezLog::Error("Shader Permutation '%s': Stage '%s' could not be loaded", GetResourceID().GetData(), ezGALShaderStage::Names[stage]);
      return res;
    }

    // store not only the hash but also the pointer to the stage binary
    // since it contains other useful information (resource bindings), that we need for shader binding
    m_pShaderStageBinaries[stage] = pStageBin;

    EZ_ASSERT_DEV(pStageBin->m_Stage == stage, "Invalid shader stage! Expected stage '%s', but loaded data is for stage '%s'", ezGALShaderStage::Names[stage], ezGALShaderStage::Names[pStageBin->m_Stage]);

    ShaderDesc.m_ByteCodes[stage] = pStageBin->m_pGALByteCode;

    uiGPUMem += pStageBin->m_ByteCode.GetCount();
  }

  m_hShader = pDevice->CreateShader(ShaderDesc);

  if (m_hShader.IsInvalidated())
  {
    ezLog::Error("Shader Permutation '%s': Shader program creation failed", GetResourceID().GetData());
    return res;
  }

  m_bShaderPermutationValid = true;

  ModifyMemoryUsage().m_uiMemoryGPU = uiGPUMem;

  return res;
}

void ezShaderPermutationResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezShaderPermutationResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

ezResourceTypeLoader* ezShaderPermutationResource::GetDefaultResourceTypeLoader() const
{
  return &g_PermutationResourceLoader;
}

struct ShaderPermutationResourceLoadData
{
  ShaderPermutationResourceLoadData() : m_Reader(&m_Storage)
  {
  }

  ezMemoryStreamStorage m_Storage;
  ezMemoryStreamReader m_Reader;
};

ezResult ezShaderPermutationResourceLoader::RunCompiler(const ezResourceBase* pResource, ezShaderPermutationBinary& BinaryInfo, bool bForce)
{
  if (ezRenderContext::IsRuntimeShaderCompilationEnabled())
  {
    if (!bForce)
    {
      // check whether any dependent file has changed, and trigger a recompilation if necessary
      if (BinaryInfo.m_DependencyFile.HasAnyFileChanged())
      {
        bForce = true;
      }
    }

    if (!bForce) // no recompilation necessary
      return EZ_SUCCESS;

    ezStringBuilder sPermutationFile = pResource->GetResourceID();

    sPermutationFile.ChangeFileExtension("");
    sPermutationFile.Shrink(ezRenderContext::GetShaderCacheDirectory().GetCharacterCount() + ezRenderContext::GetActiveShaderPlatform().GetCharacterCount() + 2, 1);

    auto itEnd = end(sPermutationFile);

    // move the start of the iterator 8 characters from the back to the front
    itEnd -= 8;

    ezString sHash = ezStringView(itEnd.GetData(), end(sPermutationFile).GetData()); // copy the hash (last 8 characters)
    ezUInt32 uiPermutationHash = ezConversionUtils::ConvertHexStringToUInt32(sHash.GetData());

    sPermutationFile.Shrink(0, 8); // remove the hash at the end
    sPermutationFile.Append(".ezShader");

    const ezPermutationGenerator* pGenerator = ezRenderContext::GetGeneratorForShaderPermutation(uiPermutationHash);

    EZ_ASSERT_DEV(pGenerator != nullptr, "The permutation generator for permutation '%s' is unknown", sHash.GetData());

    ezShaderCompiler sc;
    return sc.CompileShaderPermutationsForPlatforms(sPermutationFile.GetData(), *pGenerator, ezRenderContext::GetActiveShaderPlatform().GetData());
  }
  else
  {
    if (bForce)
    {
      ezLog::Error("Shader was forced to be compiled, but runtime shader compilation is not available");
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

bool ezShaderPermutationResourceLoader::IsResourceOutdated(const ezResourceBase* pResource) const
{
  ezDependencyFile dep;
  if (dep.ReadDependencyFile(pResource->GetResourceID()).Failed())
    return true;

  return dep.HasAnyFileChanged();
}

ezResourceLoadData ezShaderPermutationResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  ezResourceLoadData res;

  ezShaderPermutationBinary permutationBinary;

  bool bNeedsCompilation = true;

  {
    ezFileReader File;
    if (File.Open(pResource->GetResourceID().GetData()).Failed())
    {
      ezLog::Debug("Shader Permutation '%s' does not exist, triggering recompile.", pResource->GetResourceID().GetData());

      bNeedsCompilation = false;
      if (RunCompiler(pResource, permutationBinary, true).Failed())
        return res;

      // try again
      if (File.Open(pResource->GetResourceID().GetData()).Failed())
      {
        ezLog::Debug("Shader Permutation '%s' still does not exist after recompile.", pResource->GetResourceID().GetData());
        return res;
      }

      res.m_sResourceDescription = File.GetFilePathRelative().GetData();
    }

    if (permutationBinary.Read(File).Failed())
    {
      ezLog::Error("Shader Permutation '%s': Could not read shader permutation binary", pResource->GetResourceID().GetData());

      bNeedsCompilation = true;
    }
  }

  if (bNeedsCompilation)
  {
    if (RunCompiler(pResource, permutationBinary, false).Failed())
      return res;

    ezFileReader File;

    if (File.Open(pResource->GetResourceID().GetData()).Failed())
    {
      ezLog::Error("Shader Permutation '%s': Failed to open the file", pResource->GetResourceID().GetData());
      return res;
    }

    if (permutationBinary.Read(File).Failed())
    {
      ezLog::Error("Shader Permutation '%s': Binary data could not be read", pResource->GetResourceID().GetData());
      return res;
    }

    File.Close();
  }



  ShaderPermutationResourceLoadData* pData = EZ_DEFAULT_NEW(ShaderPermutationResourceLoadData);

  ezMemoryStreamWriter w(&pData->m_Storage);

  // preload the files that are referenced in the .ezPermutation file
  {
    // write the permutation file info back to the output stream, so that the resource can read it as well
    permutationBinary.Write(w);

    for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      const ezUInt32 uiStageHash = permutationBinary.m_uiShaderStageHashes[stage];

      if (uiStageHash == 0) // not used
        continue;

      // this is where the preloading happens
      ezShaderStageBinary* pStageBin = ezShaderStageBinary::LoadStageBinary((ezGALShaderStage::Enum) stage, uiStageHash);
    }
  }

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void ezShaderPermutationResourceLoader::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
{
  ShaderPermutationResourceLoadData* pData = static_cast<ShaderPermutationResourceLoadData*>(LoaderData.m_pCustomLoaderData);

  EZ_DEFAULT_DELETE(pData);
}




EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_ShaderPermutationResource);

