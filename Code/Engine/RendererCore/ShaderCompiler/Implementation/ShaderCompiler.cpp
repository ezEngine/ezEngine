#include <RendererCore/PCH.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Shader/Helper.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderProgramCompiler, ezNoBase, 1, ezRTTINoAllocator);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezResult ezShaderCompiler::FileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification)
{
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_StageSourceFile[stage] == szAbsoluteFile)
    {
      const ezString& sData = m_ShaderData.m_ShaderStageSource[stage];
      const ezUInt32 uiCount = sData.GetElementCount();
      const char* szString = sData.GetData();

      FileContent.SetCount(uiCount);

      for (ezUInt32 i = 0; i < uiCount; ++i)
        FileContent[i] = (ezUInt8) (szString[i]);

      return EZ_SUCCESS;
    }
  }

  m_IncludeFiles.Insert(szAbsoluteFile);

  ezFileReader r;
  if (r.Open(szAbsoluteFile).Failed())
    return EZ_FAILURE;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  ezFileStats stats;
  if (ezOSFile::GetFileStats(r.GetFilePathAbsolute().GetData(), stats).Succeeded())
  {
    out_FileModification = stats.m_LastModificationTime;
  }
#endif

  ezUInt8 Temp[4096];

  while (ezUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    FileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32) uiRead));
  }

  return EZ_SUCCESS;
}

static bool PlatformEnabled(const ezString& sPlatforms, const char* szPlatform)
{
  ezStringBuilder sTemp;
  sTemp = szPlatform;

  sTemp.Prepend("!");

  // if it contains '!platform'
  if (sPlatforms.FindWholeWord_NoCase(sTemp.GetData(), ezStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
    return false;

  sTemp = szPlatform;

  // if it contains 'platform'
  if (sPlatforms.FindWholeWord_NoCase(sTemp.GetData(), ezStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
    return true;

  // if it contains 'ALL'
  if (sPlatforms.FindWholeWord_NoCase("ALL", ezStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
    return true;

  return false;
}

ezResult ezShaderCompiler::CompileShader(const char* szFile, const ezPermutationGenerator& MainGenerator, const char* szPlatform)
{
  ezStringBuilder sFileContent, sTemp;

  ezInt64 iMainFileTimeStamp = 0;

  {
    ezFileReader File;
    if (File.Open(szFile).Failed())
      return EZ_FAILURE;
    
    sFileContent.ReadAll(File);

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
    ezFileStats stats;
    if (ezOSFile::GetFileStats(File.GetFilePathAbsolute().GetData(), stats).Succeeded())
      iMainFileTimeStamp = stats.m_LastModificationTime.GetInt64(ezSIUnitOfTime::Second);
#endif
  }

  

  ezTextSectionizer Sections;
  GetShaderSections(sFileContent.GetData(), Sections);

  sTemp = Sections.GetSectionContent(ezShaderSections::PLATFORMS);
  sTemp.ToUpper();

  m_ShaderData.m_Platforms = sTemp;
  m_ShaderData.m_Permutations = Sections.GetSectionContent(ezShaderSections::PERMUTATIONS);

  ezPermutationGenerator Generator = MainGenerator;
  Generator.RemoveUnusedPermutations(m_ShaderData.m_Permutations);

  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_ShaderData.m_ShaderStageSource[stage] = Sections.GetSectionContent(ezShaderSections::VERTEXSHADER + stage);

  m_StageSourceFile[ezGALShaderStage::VertexShader] = szFile;
  m_StageSourceFile[ezGALShaderStage::VertexShader].ChangeFileExtension("vs");

  m_StageSourceFile[ezGALShaderStage::HullShader] = szFile;
  m_StageSourceFile[ezGALShaderStage::HullShader].ChangeFileExtension("hs");

  m_StageSourceFile[ezGALShaderStage::DomainShader] = szFile;
  m_StageSourceFile[ezGALShaderStage::DomainShader].ChangeFileExtension("ds");

  m_StageSourceFile[ezGALShaderStage::GeometryShader] = szFile;
  m_StageSourceFile[ezGALShaderStage::GeometryShader].ChangeFileExtension("gs");

  m_StageSourceFile[ezGALShaderStage::PixelShader] = szFile;
  m_StageSourceFile[ezGALShaderStage::PixelShader].ChangeFileExtension("ps");

  m_StageSourceFile[ezGALShaderStage::ComputeShader] = szFile;
  m_StageSourceFile[ezGALShaderStage::ComputeShader].ChangeFileExtension("cs");

  m_PermVars.Clear();

  // try out every compiler that we can find
  ezRTTI* pRtti = ezRTTI::GetFirstInstance();
  while (pRtti)
  {
    ezRTTIAllocator* pAllocator = pRtti->GetAllocator();
    if (pRtti->IsDerivedFrom<ezShaderProgramCompiler>() && pAllocator->CanAllocate())
    {
      ezShaderProgramCompiler* pCompiler = static_cast<ezShaderProgramCompiler*>(pAllocator->Allocate());

      CompileShader(szFile, Generator, szPlatform, pCompiler, iMainFileTimeStamp);

      pAllocator->Deallocate(pCompiler);
    }

    pRtti = pRtti->GetNextInstance();
  }

  return EZ_SUCCESS;
}

ezResult ezShaderCompiler::CompileShader(const char* szFile, const ezPermutationGenerator& Generator, const char* szPlatform, ezShaderProgramCompiler* pCompiler, ezInt64 iMainFileTimeStamp)
{
  ezStringBuilder sTemp;
  ezStringBuilder sProcessed[ezGALShaderStage::ENUM_COUNT];

  ezHybridArray<ezString, 4> Platforms;
  pCompiler->GetSupportedPlatforms(Platforms);

  for (ezUInt32 p = 0; p < Platforms.GetCount(); ++p)
  {
    if (!PlatformEnabled(szPlatform, Platforms[p].GetData()))
      continue;

    // if this shader is not tagged for this platform, ignore it
    if (!PlatformEnabled(m_ShaderData.m_Platforms, Platforms[p].GetData()))
      continue;

    const ezUInt32 uiMaxPermutations = Generator.GetPermutationCount();

    for (ezUInt32 uiPermutation = 0; uiPermutation < uiMaxPermutations; ++uiPermutation)
    {
      Generator.GetPermutation(uiPermutation, m_PermVars);

      const ezUInt32 uiPermutationHash = Generator.GetHash(m_PermVars);

      ezShaderProgramCompiler::ezShaderProgramData spd;
      spd.m_szPlatform = Platforms[p].GetData();

      m_IncludeFiles.Clear();

      for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        ezPreprocessor pp;
        pp.SetCustomFileCache(&m_FileCache);
        pp.SetLogInterface(ezGlobalLog::GetInstance());
        pp.SetFileOpenFunction(ezPreprocessor::FileOpenCB(&ezShaderCompiler::FileOpen, this));
        pp.SetPassThroughPragma(true);
        pp.SetPassThroughUnknownCmdsCB(ezDelegate<bool (const char*)>(&ezShaderCompiler::PassThroughUnknownCommandCB, this));
        pp.SetPassThroughLine(true);

        sTemp = Platforms[p];
        sTemp.ToUpper();

        pp.AddCustomDefine(sTemp.GetData());

        for (ezUInt32 pv = 0; pv < m_PermVars.GetCount(); ++pv)
        {
          sTemp.Format("%s %s", m_PermVars[pv].m_sVariable.GetData(), m_PermVars[pv].m_sValue.GetData());
          pp.AddCustomDefine(sTemp.GetData());
        }

        pp.Process(m_StageSourceFile[stage].GetData(), sProcessed[stage], true);

        spd.m_szShaderSource[stage] = sProcessed[stage].GetData();

        spd.m_StageBinary[stage].m_Stage = (ezGALShaderStage::Enum) stage;
        spd.m_StageBinary[stage].m_uiSourceHash = ezHashing::MurmurHash(ezHashing::StringWrapper(sProcessed[stage].GetData()));

        if (spd.m_StageBinary[stage].m_uiSourceHash != 0)
        {
          ezShaderStageBinary* pBinary = ezShaderStageBinary::LoadStageBinary((ezGALShaderStage::Enum) stage, spd.m_StageBinary[stage].m_uiSourceHash);

          if (pBinary)
          {
            spd.m_StageBinary[stage] = *pBinary;
            spd.m_bWriteToDisk[stage] = false;
          }
        }
      }

      if (pCompiler->Compile(spd, ezGlobalLog::GetInstance()).Failed())
        ezLog::Error("Shader compilation failed.");

      ezShaderPermutationBinary spb;

      for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        spb.m_uiShaderStageHashes[stage] = spd.m_StageBinary[stage].m_uiSourceHash;

        if (spd.m_StageBinary[stage].m_uiSourceHash != 0 && spd.m_bWriteToDisk[stage])
        {
          if (spd.m_StageBinary[stage].WriteStageBinary().Failed())
          {
            ezLog::Error("Writing stage binary failed");
            continue;
          }
        }
      }

      sTemp = ezShaderManager::GetShaderCacheDirectory();
      sTemp.AppendPath(Platforms[p].GetData());
      sTemp.AppendPath(szFile);
      sTemp.ChangeFileExtension("");
      if (sTemp.EndsWith("."))
        sTemp.Shrink(0, 1);
      sTemp.AppendFormat("%08X.permutation", uiPermutationHash);

      spb.m_IncludeFiles.Clear();
      spb.m_IncludeFiles.Reserve(m_IncludeFiles.GetCount());

      spb.m_IncludeFiles.PushBack(szFile);
      spb.m_iMaxTimeStamp = iMainFileTimeStamp;

      for (auto it = m_IncludeFiles.GetIterator(); it.IsValid(); ++it)
      {
        spb.m_IncludeFiles.PushBack(it.Key());
        spb.m_iMaxTimeStamp = ezMath::Max(spb.m_iMaxTimeStamp, m_FileCache.Lookup(it.Key()).Value().m_Timestamp.GetInt64(ezSIUnitOfTime::Second));
      }

      ezFileWriter PermutationFileOut;
      EZ_VERIFY(PermutationFileOut.Open(sTemp.GetData()).Succeeded(), "Could not write file '%s'", sTemp.GetData());
        spb.Write(PermutationFileOut);
    }
  }

  return EZ_SUCCESS;
}


