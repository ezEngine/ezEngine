#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/Shader/Helper.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/MemoryStream.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderPermutationResource, ezResourceBase, ezRTTIDefaultAllocator<ezShaderPermutationResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

static ezShaderPermutationResourceLoader g_PermutationResourceLoader;

ezShaderPermutationResource::ezShaderPermutationResource()
{
  m_bValid = false;
  m_uiMaxQualityLevel = 1;
  m_Flags.Add(ezResourceFlags::UpdateOnMainThread);
}

void ezShaderPermutationResource::UnloadData(bool bFullUnload)
{
  m_bValid = false;
  m_uiLoadedQualityLevel = 0;
  m_LoadingState = ezResourceLoadState::Uninitialized;

  if (!m_hVertexDeclaration.IsInvalidated())
  {
    ezShaderManager::GetDevice()->DestroyVertexDeclaration(m_hVertexDeclaration);
    m_hVertexDeclaration.Invalidate();
  }

  if (!m_hShader.IsInvalidated())
  {
    ezShaderManager::GetDevice()->DestroyShader(m_hShader);
    m_hShader.Invalidate();
  }
}

void ezShaderPermutationResource::UpdateContent(ezStreamReaderBase& Stream)
{
  ezUInt32 uiGPUMem = 0;
  SetMemoryUsageGPU(0);

  m_bValid = false;

  m_LoadingState = ezResourceLoadState::Loaded;
  m_uiLoadedQualityLevel = 1;
  m_uiMaxQualityLevel = 1;

  if (m_PermutationBinary.Read(Stream).Failed())
  {
    ezLog::Error("Shader Resource '%s': Could not read shader permutation binary", GetResourceID().GetData());
    return;
  }

  ezGALShaderCreationDescription ShaderDesc;

  // iterate over all shader stages, add them to the descriptor
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    const ezUInt32 uiStageHash = m_PermutationBinary.m_uiShaderStageHashes[stage];

    if (uiStageHash == 0) // not used
      continue;

    ezShaderStageBinary* pStageBin = ezShaderStageBinary::LoadStageBinary((ezGALShaderStage::Enum) stage, uiStageHash);

    if (pStageBin == nullptr)
    {
      ezLog::Error("Shader Resource '%s': Stage %u could not be loaded", GetResourceID().GetData(), stage);
      return;
    }

    EZ_ASSERT(pStageBin->m_Stage == stage, "Invalid shader stage! Expected stage %u, but loaded data is for stage %u", stage, pStageBin->m_Stage);

    ShaderDesc.m_ByteCodes[stage] = pStageBin->m_pGALByteCode;

    uiGPUMem += pStageBin->m_ByteCode.GetCount();
  }

  m_hShader = ezShaderManager::GetDevice()->CreateShader(ShaderDesc);

  if (m_hShader.IsInvalidated())
  {
    ezLog::Error("Shader Resource '%s': Shader program creation failed", GetResourceID().GetData());
    return;
  }

  m_bValid = true;

  // *** HACK ***

  ezGALVertexDeclarationCreationDescription VertDeclDesc;
  VertDeclDesc.m_hShader = m_hShader;
  VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat, 0, 0, false));
  VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat, 12, 0, false));
  VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat, 24, 0, false));

  m_hVertexDeclaration = ezShaderManager::GetDevice()->CreateVertexDeclaration(VertDeclDesc);

  // ************

  SetMemoryUsageGPU(uiGPUMem);
}

void ezShaderPermutationResource::UpdateMemoryUsage()
{
  SetMemoryUsageCPU(sizeof(this));
}

void ezShaderPermutationResource::CreateResource(const ezShaderPermutationResourceDescriptor& descriptor)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
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

ezTimestamp ezShaderPermutationResourceLoader::GetFileTimestamp(const char* szFile)
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  bool bExisted = false;
  auto it = m_FileTimestamps.FindOrAdd(szFile, &bExisted);

  if (!bExisted || it.Value().m_LastCheck + ezTime::Seconds(2.0) < ezTime::Now())
  {
    it.Value().m_LastCheck = ezTime::Now();

    ezString sAbsPath;
    if (ezFileSystem::ResolvePath(szFile, false, &sAbsPath, nullptr).Succeeded())
    {
      ezFileStats stats;
      if (ezOSFile::GetFileStats(sAbsPath.GetData(), stats).Succeeded())
      {
        it.Value().m_FileTimestamp = stats.m_LastModificationTime;
      }
    }
  }

  return it.Value().m_FileTimestamp;
#else
  return ezTimestamp();
#endif
}

ezResult ezShaderPermutationResourceLoader::RunCompiler(const ezResourceBase* pResource, ezShaderPermutationBinary& BinaryInfo, bool bForce)
{
  if (ezShaderManager::IsRuntimeCompilationEnabled())
  {
    const ezShaderPermutationResource* pShaderPermutation = static_cast<const ezShaderPermutationResource*>(pResource);

    if (!bForce)
    {
      for (ezUInt32 inc = 0; inc < BinaryInfo.m_IncludeFiles.GetCount(); ++inc)
      {
        ezTimestamp stamp = GetFileTimestamp(BinaryInfo.m_IncludeFiles[inc].GetData());

        if (stamp.GetInt64(ezSIUnitOfTime::Second) > BinaryInfo.m_iMaxTimeStamp)
        {
          ezLog::Info("Detected file change in '%s' (ts %lli > ts max %lli)", BinaryInfo.m_IncludeFiles[inc].GetData(), stamp.GetInt64(ezSIUnitOfTime::Second), BinaryInfo.m_iMaxTimeStamp);

          bForce = true;
          break;
        }
      }
    }

    if (!bForce) // no recompilation necessary
      return EZ_SUCCESS;

    /// \todo Determine whether any file has changed and requires recompilation

    ezStringBuilder sPermutationFile = pResource->GetResourceID();

    sPermutationFile.ChangeFileExtension("");
    sPermutationFile.Shrink(ezShaderManager::GetShaderCacheDirectory().GetCharacterCount() + ezShaderManager::GetPlatform().GetCharacterCount() + 2, 1);

    ezStringView itBack = sPermutationFile.GetIteratorBack();

    // move the start of the iterator 8 characters from the back to the front
    itBack -= 8;

    ezString sHash = itBack; // copy the hash (last 8 characters)
    ezUInt32 uiPermutationHash = ezConversionUtils::ConvertHexStringToUInt32(sHash.GetData());

    sPermutationFile.Shrink(0, 8); // remove the hash at the end
    sPermutationFile.Append(".shader");

    const ezPermutationGenerator* pGenerator = ezShaderManager::GetGeneratorForPermutation(uiPermutationHash);

    EZ_ASSERT(pGenerator != nullptr, "The permutation generator for permutation '%s' is unknown", sHash.GetData());

    //ezFileReader ShaderProgramFile;
    //if (ShaderProgramFile.Open(pResource->GetResourceID().GetData()).Failed())
    //{
      ezShaderCompiler sc;
      return sc.CompileShader(sPermutationFile.GetData(), *pGenerator, ezShaderManager::GetPlatform().GetData());
    //}
  }

  return EZ_SUCCESS;
}

ezResourceLoadData ezShaderPermutationResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  ezResourceLoadData res;

  ezShaderPermutationBinary permutationBinary;

  bool bNeedsCompilation = true;

  ezFileReader File;
  if (File.Open(pResource->GetResourceID().GetData()).Failed())
  {
    bNeedsCompilation = false;
    RunCompiler(pResource, permutationBinary, true);

    // try again
    if (File.Open(pResource->GetResourceID().GetData()).Failed())
      return res;
  }

  if (permutationBinary.Read(File).Failed())
  {
    ezLog::Error("Shader Resource '%s': Could not read shader permutation binary", pResource->GetResourceID().GetData());
    return res;
  }

  File.Close();

  if (bNeedsCompilation)
  {
    RunCompiler(pResource, permutationBinary, false);

    EZ_VERIFY(File.Open(pResource->GetResourceID().GetData()).Succeeded(), "some error");
    EZ_VERIFY(permutationBinary.Read(File).Succeeded(), "some other error");
    File.Close();
  }



  ShaderPermutationResourceLoadData* pData = EZ_DEFAULT_NEW(ShaderPermutationResourceLoadData);

  ezMemoryStreamWriter w(&pData->m_Storage);

  // preload the files that are referenced in the .permutation file
  {
    // write the permutation file info back to the output stream, so that the resource can read it as well
    permutationBinary.Write(w);

    for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      const ezUInt32 uiStageHash = permutationBinary.m_uiShaderStageHashes[stage];

      if (uiStageHash == 0) // not used
        continue;

      // the is where the preloading happens
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


