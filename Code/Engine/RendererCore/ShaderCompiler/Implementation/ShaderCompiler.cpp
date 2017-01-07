#include <RendererCore/PCH.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderProgramCompiler, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE


namespace
{
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

  static void GenerateDefines(const char* szPlatform, const ezArrayPtr<ezPermutationVar>& permutationVars, ezHybridArray<ezString, 32>& out_Defines)
  {
    out_Defines.PushBack("TRUE 1");
    out_Defines.PushBack("FALSE 0");

    ezStringBuilder sTemp;

    sTemp = szPlatform;
    sTemp.ToUpper();

    out_Defines.PushBack(sTemp.GetData());

    for (const ezPermutationVar& var : permutationVars)
    {
      const char* szValue = var.m_sValue.GetData();
      const bool isBoolVar = ezStringUtils::IsEqual(szValue, "TRUE") || ezStringUtils::IsEqual(szValue, "FALSE");

      if (isBoolVar)
      {
        sTemp.Format("{0} {1}", var.m_sName.GetData(), var.m_sValue.GetData());
        out_Defines.PushBack(sTemp);
      }
      else
      {
        const char* szName = var.m_sName.GetData();
        auto enumValues = ezShaderManager::GetPermutationEnumValues(var.m_sName);

        for (ezUInt32 i = 0; i < enumValues.GetCount(); ++i)
        {
          if (!enumValues[i].IsEmpty())
          {
            sTemp.Format("{0}_{1} {2}", szName, enumValues[i].GetData(), i);
            out_Defines.PushBack(sTemp);
          }
        }

        if (ezStringUtils::StartsWith(szValue, szName))
        {
          sTemp.Format("{0} {1}", szName, szValue);
        }
        else
        {
          sTemp.Format("{0} {1}_{2}", szName, szName, szValue);
        }
        out_Defines.PushBack(sTemp);
      }
    }
  }

  static const char* s_szStageDefines[ezGALShaderStage::ENUM_COUNT] =
  {
    "VERTEX_SHADER",
    "HULL_SHADER",
    "DOMAIN_SHADER",
    "GEOMETRY_SHADER",
    "PIXEL_SHADER",
    "COMPUTE_SHADER"
  };
}

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
    ezLog::Error("Could not find include file '{0}'", szAbsoluteFile);
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

ezResult ezShaderCompiler::CompileShaderPermutationForPlatforms(const char* szFile, const ezArrayPtr<const ezPermutationVar>& permutationVars, const char* szPlatform)
{
  ezStringBuilder sFileContent, sTemp;

  {
    ezFileReader File;
    if (File.Open(szFile).Failed())
      return EZ_FAILURE;

    sFileContent.ReadAll(File);
  }

  ezShaderHelper::ezTextSectionizer Sections;
  ezShaderHelper::GetShaderSections(sFileContent.GetData(), Sections);

  ezUInt32 uiFirstLine = 0;
  sTemp = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::PLATFORMS, uiFirstLine);
  sTemp.ToUpper();

  m_ShaderData.m_Platforms = sTemp;

  ezHybridArray<ezHashedString, 16> usedPermutations;
  ezShaderParser::ParsePermutationSection(Sections.GetSectionContent(ezShaderHelper::ezShaderSections::PERMUTATIONS, uiFirstLine), usedPermutations);

  for (ezHashedString& usedPermutationVar : usedPermutations)
  {
    ezUInt32 uiIndex = ezInvalidIndex;
    for (ezUInt32 i = 0; i < permutationVars.GetCount(); ++i)
    {
      if (permutationVars[i].m_sName == usedPermutationVar)
      {
        uiIndex = i;
        break;
      }
    }

    if (uiIndex != ezInvalidIndex)
    {
      m_ShaderData.m_Permutations.PushBack(permutationVars[uiIndex]);
    }
    else
    {
      ezLog::Error("No value given for permutation var '{0}'. Assuming default value of zero.", usedPermutationVar.GetData());

      ezPermutationVar& finalVar = m_ShaderData.m_Permutations.ExpandAndGetRef();
      finalVar.m_sName = usedPermutationVar;
      finalVar.m_sValue.Assign("0");
    }
  }


  m_ShaderData.m_StateSource = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::RENDERSTATE, uiFirstLine);

  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    sTemp = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::VERTEXSHADER + stage, uiFirstLine);

    // later code checks whether the string is empty, to see whether we have any shader source, so this has to be kept empty
    if (!sTemp.IsEmpty())
      sTemp.PrependFormat("#line {0}\n", uiFirstLine);

    m_ShaderData.m_ShaderStageSource[stage] = sTemp;
  }

  ezStringBuilder tmp = szFile;
  tmp.MakeCleanPath();

  m_StageSourceFile[ezGALShaderStage::VertexShader] = tmp;
  m_StageSourceFile[ezGALShaderStage::VertexShader].ChangeFileExtension("vs");

  m_StageSourceFile[ezGALShaderStage::HullShader] = tmp;
  m_StageSourceFile[ezGALShaderStage::HullShader].ChangeFileExtension("hs");

  m_StageSourceFile[ezGALShaderStage::DomainShader] = tmp;
  m_StageSourceFile[ezGALShaderStage::DomainShader].ChangeFileExtension("ds");

  m_StageSourceFile[ezGALShaderStage::GeometryShader] = tmp;
  m_StageSourceFile[ezGALShaderStage::GeometryShader].ChangeFileExtension("gs");

  m_StageSourceFile[ezGALShaderStage::PixelShader] = tmp;
  m_StageSourceFile[ezGALShaderStage::PixelShader].ChangeFileExtension("ps");

  m_StageSourceFile[ezGALShaderStage::ComputeShader] = tmp;
  m_StageSourceFile[ezGALShaderStage::ComputeShader].ChangeFileExtension("cs");

  // try out every compiler that we can find
  ezRTTI* pRtti = ezRTTI::GetFirstInstance();
  while (pRtti)
  {
    ezRTTIAllocator* pAllocator = pRtti->GetAllocator();
    if (pRtti->IsDerivedFrom<ezShaderProgramCompiler>() && pAllocator->CanAllocate())
    {
      ezShaderProgramCompiler* pCompiler = static_cast<ezShaderProgramCompiler*>(pAllocator->Allocate());

      RunShaderCompiler(szFile, szPlatform, pCompiler);

      pAllocator->Deallocate(pCompiler);
    }

    pRtti = pRtti->GetNextInstance();
  }

  return EZ_SUCCESS;
}

void ezShaderCompiler::RunShaderCompiler(const char* szFile, const char* szPlatform, ezShaderProgramCompiler* pCompiler)
{
  EZ_LOG_BLOCK("Compiling Shader", szFile);

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

    ezShaderProgramCompiler::ezShaderProgramData spd;
    spd.m_szSourceFile = szFile;
    spd.m_szPlatform = Platforms[p].GetData();

    m_IncludeFiles.Clear();

    ezHybridArray<ezString, 32> defines;
    GenerateDefines(Platforms[p].GetData(), m_ShaderData.m_Permutations, defines);

    bool bSuccess = true;

    ezShaderPermutationBinary shaderPermutationBinary;

    // Generate Shader State Source
    {
      EZ_LOG_BLOCK("Preprocessing Shader State Source");

      ezPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(ezGlobalLog::GetOrCreateInstance());
      pp.SetFileOpenFunction(ezPreprocessor::FileOpenCB(&ezShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(false);
      pp.SetPassThroughLine(false);

      for (auto& define : defines)
      {
        pp.AddCustomDefine(define);
      }

      ezStringBuilder sOutput;
      if (pp.Process("ShaderRenderState", sOutput, false).Failed())
      {
        bSuccess = false;
        ezLog::Error("Preprocessing the Shader State block failed");
      }
      else
      {
        if (shaderPermutationBinary.m_StateDescriptor.Load(sOutput).Failed())
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
      pp.SetLogInterface(ezGlobalLog::GetOrCreateInstance());
      pp.SetFileOpenFunction(ezPreprocessor::FileOpenCB(&ezShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(true);
      pp.SetPassThroughUnknownCmdsCB(ezMakeDelegate(&ezShaderCompiler::PassThroughUnknownCommandCB, this));
      pp.SetPassThroughLine(false);

      pp.AddCustomDefine(s_szStageDefines[stage]);
      for (auto& define : defines)
      {
        pp.AddCustomDefine(define);
      }

      if (pp.Process(m_StageSourceFile[stage], sProcessed[stage], true, true, true).Failed())
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
      shaderPermutationBinary.m_uiShaderStageHashes[stage] = spd.m_StageBinary[stage].m_uiSourceHash;
    }

    // if compilation failed, the stage binary for the source hash will simply not exist and therefore cannot be loaded
    // the .ezPermutation file should be updated, however, to store the new source hash to the broken shader
    if (bSuccess && pCompiler->Compile(spd, ezGlobalLog::GetOrCreateInstance()).Failed())
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
          ezStringBuilder sShaderStageFile = ezShaderManager::GetCacheDirectory();

          sShaderStageFile.AppendPath(ezShaderManager::GetActivePlatform().GetData());
          sShaderStageFile.AppendFormat("/{0}.ezShaderSource", ezArgU(spd.m_StageBinary[stage].m_uiSourceHash, 8, true, 16, true));

          ezFileWriter StageFileOut;
          if (StageFileOut.Open(sShaderStageFile.GetData()).Succeeded())
          {
            StageFileOut.WriteBytes(spd.m_szShaderSource[stage], ezStringUtils::GetStringElementCount(spd.m_szShaderSource[stage]));
            ezLog::Info("Failed shader source written to '{0}'", sShaderStageFile.GetData());
          }
        }
      }
    }

    ezStringBuilder sTemp = ezShaderManager::GetCacheDirectory();
    sTemp.AppendPath(Platforms[p].GetData());
    sTemp.AppendPath(szFile);
    sTemp.ChangeFileExtension("");
    if (sTemp.EndsWith("."))
      sTemp.Shrink(0, 1);

    const ezUInt32 uiPermutationHash = ezShaderHelper::CalculateHash(m_ShaderData.m_Permutations);
    sTemp.AppendFormat("{0}.ezPermutation", ezArgU(uiPermutationHash, 8, true, 16, true));

    shaderPermutationBinary.m_DependencyFile.Clear();
    shaderPermutationBinary.m_DependencyFile.AddFileDependency(szFile);

    for (auto it = m_IncludeFiles.GetIterator(); it.IsValid(); ++it)
    {
      shaderPermutationBinary.m_DependencyFile.AddFileDependency(it.Key());
    }

    shaderPermutationBinary.m_PermutationVars = m_ShaderData.m_Permutations;

    ezDeferredFileWriter PermutationFileOut;
    PermutationFileOut.SetOutput(sTemp.GetData());
    shaderPermutationBinary.Write(PermutationFileOut);

    if (PermutationFileOut.Close().Failed())
    {
      ezLog::Error("Could not open file for writing: '{0}'", sTemp.GetData());
    }
  }
}




EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderCompiler);

