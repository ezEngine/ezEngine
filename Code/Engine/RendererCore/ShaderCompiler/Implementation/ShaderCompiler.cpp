#include <RendererCore/PCH.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderProgramCompiler, ezReflectedClass, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezResult ezShaderCompiler::FileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification)
{
  if (ezStringUtils::IsEqual(szAbsoluteFile, "ShaderRenderState"))
  {
    const ezString& sData = m_ShaderData.m_StateSource;
    const ezUInt32 uiCount = sData.GetElementCount();
    const char* szString = sData.GetData();

    FileContent.SetCount(uiCount);

    for (ezUInt32 i = 0; i < uiCount; ++i)
      FileContent[i] = (ezUInt8) (szString[i]);

    return EZ_SUCCESS;
  }

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
  {
    ezLog::Error("Could not find include file '%s'", szAbsoluteFile);
    return EZ_FAILURE;
  }

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

ezResult ezShaderCompiler::CompileShaderPermutationsForPlatforms(const char* szFile, const ezPermutationGenerator& MainGenerator, const char* szPlatform)
{
  ezStringBuilder sFileContent, sTemp;

  {
    ezFileReader File;
    if (File.Open(szFile).Failed())
      return EZ_FAILURE;

    sFileContent.ReadAll(File);
  }


  ezTextSectionizer Sections;
  GetShaderSections(sFileContent.GetData(), Sections);

  ezUInt32 uiFirstLine = 0;
  sTemp = Sections.GetSectionContent(ezShaderSections::PLATFORMS, uiFirstLine);
  sTemp.ToUpper();

  m_ShaderData.m_Platforms = sTemp;
  m_ShaderData.m_Permutations = Sections.GetSectionContent(ezShaderSections::PERMUTATIONS, uiFirstLine);

  ezPermutationGenerator Generator = MainGenerator;
  Generator.RemoveUnusedPermutations(m_ShaderData.m_Permutations);

  m_ShaderData.m_StateSource = Sections.GetSectionContent(ezShaderSections::RENDERSTATE, uiFirstLine);

  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    sTemp = Sections.GetSectionContent(ezShaderSections::VERTEXSHADER + stage, uiFirstLine);

    // later code checks whether the string is empty, to see whether we have any shader source, so this has to be kept empty
    if (!sTemp.IsEmpty())
      sTemp.PrependFormat("#line %u\n", uiFirstLine);

    m_ShaderData.m_ShaderStageSource[stage] = sTemp;
  }

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

      RunShaderCompilerForPermutations(szFile, Generator, szPlatform, pCompiler);

      pAllocator->Deallocate(pCompiler);
    }

    pRtti = pRtti->GetNextInstance();
  }

  return EZ_SUCCESS;
}

void ezShaderCompiler::RunShaderCompilerForPermutations(const char* szFile, const ezPermutationGenerator& Generator, const char* szPlatform, ezShaderProgramCompiler* pCompiler)
{
  EZ_LOG_BLOCK("Compiling Shader", szFile);

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

    EZ_LOG_BLOCK("Platform", Platforms[p].GetData());

    const ezUInt32 uiMaxPermutations = Generator.GetPermutationCount();

    for (ezUInt32 uiPermutation = 0; uiPermutation < uiMaxPermutations; ++uiPermutation)
    {
      Generator.GetPermutation(uiPermutation, m_PermVars);

      const ezUInt32 uiPermutationHash = Generator.GetHash(m_PermVars);

      ezShaderProgramCompiler::ezShaderProgramData spd;
      spd.m_szSourceFile = szFile;
      spd.m_szPlatform = Platforms[p].GetData();

      m_IncludeFiles.Clear();

      bool bSuccess = true;

      ezShaderPermutationBinary spb;

      // Generate Shader State Source
      {
        EZ_LOG_BLOCK("Preprocessing Shader State Source");

        ezPreprocessor pp;
        pp.SetCustomFileCache(&m_FileCache);
        pp.SetLogInterface(ezGlobalLog::GetInstance());
        pp.SetFileOpenFunction(ezPreprocessor::FileOpenCB(&ezShaderCompiler::FileOpen, this));
        pp.SetPassThroughPragma(false);
        pp.SetPassThroughLine(false);

        sTemp = Platforms[p];
        sTemp.ToUpper();

        pp.AddCustomDefine(sTemp.GetData());

        for (ezUInt32 pv = 0; pv < m_PermVars.GetCount(); ++pv)
        {
          sTemp.Format("%s %s", m_PermVars[pv].m_sVariable.GetData(), m_PermVars[pv].m_sValue.GetData());
          pp.AddCustomDefine(sTemp.GetData());
        }

        ezStringBuilder sOutput;
        if (pp.Process("ShaderRenderState", sOutput, false).Failed())
        {
          bSuccess = false;
          ezLog::Error("Preprocessing the Shader State block failed");
        }
        else
        {
          if (spb.m_StateDescriptor.Load(sOutput).Failed())
          {
            ezLog::Error("Failed to interpret the shader state block");
            bSuccess = false;
          }
        }
      }

      for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        EZ_LOG_BLOCK("Preprocessing", m_StageSourceFile[stage].GetData());

        ezPreprocessor pp;
        pp.SetCustomFileCache(&m_FileCache);
        pp.SetLogInterface(ezGlobalLog::GetInstance());
        pp.SetFileOpenFunction(ezPreprocessor::FileOpenCB(&ezShaderCompiler::FileOpen, this));
        pp.SetPassThroughPragma(true);
        pp.SetPassThroughUnknownCmdsCB(ezMakeDelegate(&ezShaderCompiler::PassThroughUnknownCommandCB, this));
        pp.SetPassThroughLine(false);

        sTemp = Platforms[p];
        sTemp.ToUpper();

        pp.AddCustomDefine(sTemp.GetData());

        for (ezUInt32 pv = 0; pv < m_PermVars.GetCount(); ++pv)
        {
          sTemp.Format("%s %s", m_PermVars[pv].m_sVariable.GetData(), m_PermVars[pv].m_sValue.GetData());
          pp.AddCustomDefine(sTemp.GetData());
        }

        if (pp.Process(m_StageSourceFile[stage].GetData(), sProcessed[stage], true, true, true).Failed())
        {
          bSuccess = false;
          ezLog::Error("Shader preprocessing failed");

          sProcessed[stage].Clear();
          
          spd.m_szShaderSource[stage] = m_StageSourceFile[stage].GetData();
        }
        else
        {
          spd.m_szShaderSource[stage] = sProcessed[stage].GetData();
        }

        spd.m_StageBinary[stage].m_Stage = (ezGALShaderStage::Enum) stage;
        spd.m_StageBinary[stage].m_uiSourceHash = ezHashing::MurmurHash(ezHashing::StringWrapper(spd.m_szShaderSource[stage]));

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

      // copy the source hashes
      for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        spb.m_uiShaderStageHashes[stage] = spd.m_StageBinary[stage].m_uiSourceHash;
      }

      // if compilation failed, the stage binary for the source hash will simply not exist and therefore cannot be loaded
      // the .ezPermutation file should be updated, however, to store the new source hash to the broken shader
      if (bSuccess && pCompiler->Compile(spd, ezGlobalLog::GetInstance()).Failed())
      {
        ezLog::Error("Shader compilation failed.");
        bSuccess = false;
      }

      for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (spd.m_StageBinary[stage].m_uiSourceHash != 0 && spd.m_bWriteToDisk[stage])
        {
          if (bSuccess)
          {
            if (spd.m_StageBinary[stage].WriteStageBinary().Failed())
            {
              ezLog::Error("Writing stage binary failed");
              continue;
            }
          }
          else
          {
            ezStringBuilder sShaderStageFile = ezRenderContext::GetShaderCacheDirectory();

            sShaderStageFile.AppendPath(ezRenderContext::GetActiveShaderPlatform().GetData());
            sShaderStageFile.AppendFormat("/%08X.ezShaderSource", spd.m_StageBinary[stage].m_uiSourceHash);

            ezFileWriter StageFileOut;
            if (StageFileOut.Open(sShaderStageFile.GetData()).Succeeded())
            {
              StageFileOut.WriteBytes(spd.m_szShaderSource[stage], ezStringUtils::GetStringElementCount(spd.m_szShaderSource[stage]));
              ezLog::Info("Failed shader source written to '%s'", sShaderStageFile.GetData());
            }
          }
        }
      }

      sTemp = ezRenderContext::GetShaderCacheDirectory();
      sTemp.AppendPath(Platforms[p].GetData());
      sTemp.AppendPath(szFile);
      sTemp.ChangeFileExtension("");
      if (sTemp.EndsWith("."))
        sTemp.Shrink(0, 1);
      sTemp.AppendFormat("%08X.ezPermutation", uiPermutationHash);

      spb.m_DependencyFile.Clear();
      spb.m_DependencyFile.AddFileDependency(szFile);

      for (auto it = m_IncludeFiles.GetIterator(); it.IsValid(); ++it)
        spb.m_DependencyFile.AddFileDependency(it.Key());

      ezFileWriter PermutationFileOut;
      if (PermutationFileOut.Open(sTemp.GetData()).Failed())
      {
        ezLog::Error("Could not open file for writing: '%s'", sTemp.GetData());
      }
      else
      {
        spb.Write(PermutationFileOut);
      }
    }
  }
}




EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderCompiler);

