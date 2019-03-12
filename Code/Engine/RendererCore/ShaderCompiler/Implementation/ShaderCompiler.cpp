#include <RendererCorePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderProgramCompiler, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE;


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

    // do not enable this when ALL is specified
    if (ezStringUtils::IsEqual(szPlatform, "DEBUG"))
      return false;

    // if it contains 'ALL'
    if (sPlatforms.FindWholeWord_NoCase("ALL", ezStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    return false;
  }

  static void GenerateDefines(const char* szPlatform, const ezArrayPtr<ezPermutationVar>& permutationVars,
                              ezHybridArray<ezString, 32>& out_Defines)
  {
    ezStringBuilder sTemp;

    if (out_Defines.IsEmpty())
    {
      out_Defines.PushBack("TRUE 1");
      out_Defines.PushBack("FALSE 0");

      sTemp = szPlatform;
      sTemp.ToUpper();

      out_Defines.PushBack(sTemp.GetData());
    }

    for (const ezPermutationVar& var : permutationVars)
    {
      const char* szValue = var.m_sValue.GetData();
      const bool isBoolVar = ezStringUtils::IsEqual(szValue, "TRUE") || ezStringUtils::IsEqual(szValue, "FALSE");

      if (isBoolVar)
      {
        sTemp.Set(var.m_sName, " ", var.m_sValue);
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
            sTemp.Format("{1} {2}", szName, enumValues[i], i);
            out_Defines.PushBack(sTemp);
          }
        }

        if (ezStringUtils::StartsWith(szValue, szName))
        {
          sTemp.Set(szName, " ", szValue);
        }
        else
        {
          sTemp.Set(szName, " ", szName, "_", szValue);
        }
        out_Defines.PushBack(sTemp);
      }
    }
  }

  static const char* s_szStageDefines[ezGALShaderStage::ENUM_COUNT] = {"VERTEX_SHADER",   "HULL_SHADER",  "DOMAIN_SHADER",
                                                                       "GEOMETRY_SHADER", "PIXEL_SHADER", "COMPUTE_SHADER"};
}

ezResult ezShaderCompiler::FileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent, ezTimestamp& out_FileModification)
{
  if (ezStringUtils::IsEqual(szAbsoluteFile, "ShaderRenderState"))
  {
    const ezString& sData = m_ShaderData.m_StateSource;
    const ezUInt32 uiCount = sData.GetElementCount();
    const char* szString = sData.GetData();

    FileContent.SetCountUninitialized(uiCount);

    if (uiCount > 0)
    {
      ezMemoryUtils::Copy<ezUInt8>(FileContent.GetData(), (const ezUInt8*)szString, uiCount);
    }

    return EZ_SUCCESS;
  }

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_StageSourceFile[stage] == szAbsoluteFile)
    {
      const ezString& sData = m_ShaderData.m_ShaderStageSource[stage];
      const ezUInt32 uiCount = sData.GetElementCount();
      const char* szString = sData.GetData();

      FileContent.SetCountUninitialized(uiCount);

      if (uiCount > 0)
      {
        ezMemoryUtils::Copy<ezUInt8>(FileContent.GetData(), (const ezUInt8*)szString, uiCount);
      }

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
    FileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32)uiRead));
  }

  return EZ_SUCCESS;
}

ezResult ezShaderCompiler::CompileShaderPermutationForPlatforms(const char* szFile,
                                                                const ezArrayPtr<const ezPermutationVar>& permutationVars,
                                                                ezLogInterface* pLog, const char* szPlatform)
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
  ezShaderParser::ParsePermutationSection(Sections.GetSectionContent(ezShaderHelper::ezShaderSections::PERMUTATIONS, uiFirstLine),
                                          usedPermutations, m_ShaderData.m_FixedPermVars);

  for (const ezHashedString& usedPermutationVar : usedPermutations)
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
      ezLog::Error("No value given for permutation var '{0}'. Assuming default value of zero.", usedPermutationVar);

      ezPermutationVar& finalVar = m_ShaderData.m_Permutations.ExpandAndGetRef();
      finalVar.m_sName = usedPermutationVar;
      finalVar.m_sValue.Assign("0");
    }
  }

  m_ShaderData.m_StateSource = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::RENDERSTATE, uiFirstLine);

  ezUInt32 uiFirstShaderLine = 0;
  ezStringView sShaderSource = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::SHADER, uiFirstShaderLine);

  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    ezStringView sStageSource = Sections.GetSectionContent(ezShaderHelper::ezShaderSections::VERTEXSHADER + stage, uiFirstLine);

    // later code checks whether the string is empty, to see whether we have any shader source, so this has to be kept empty
    if (!sStageSource.IsEmpty())
    {
      sTemp.Clear();

      // prepend common shader section if there is any
      if (!sShaderSource.IsEmpty())
      {
        sTemp.AppendFormat("#line {0}\n{1}", uiFirstShaderLine, sShaderSource);
      }

      sTemp.AppendFormat("#line {0}\n{1}", uiFirstLine, sStageSource);

      m_ShaderData.m_ShaderStageSource[stage] = sTemp;
    }
    else
    {
      m_ShaderData.m_ShaderStageSource[stage].Clear();
    }
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
      ezShaderProgramCompiler* pCompiler = pAllocator->Allocate<ezShaderProgramCompiler>();

      const ezResult ret = RunShaderCompiler(szFile, szPlatform, pCompiler, pLog);
      pAllocator->Deallocate(pCompiler);

      if (ret.Failed())
        return ret;
    }

    pRtti = pRtti->GetNextInstance();
  }

  return EZ_SUCCESS;
}

ezResult ezShaderCompiler::RunShaderCompiler(const char* szFile, const char* szPlatform, ezShaderProgramCompiler* pCompiler,
                                             ezLogInterface* pLog)
{
  EZ_LOG_BLOCK(pLog, "Compiling Shader", szFile);

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

    EZ_LOG_BLOCK(pLog, "Platform", Platforms[p].GetData());

    ezShaderProgramCompiler::ezShaderProgramData spd;
    spd.m_szSourceFile = szFile;
    spd.m_szPlatform = Platforms[p].GetData();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    // 'DEBUG' is a platform tag that enables additional compiler flags
    if (PlatformEnabled(m_ShaderData.m_Platforms, "DEBUG"))
    {
      ezLog::Warning("Shader specifies the 'DEBUG' platform, which enables the debug shader compiler flag.");
      spd.m_Flags.Add(ezShaderCompilerFlags::Debug);
    }
#endif

    m_IncludeFiles.Clear();

    ezHybridArray<ezString, 32> defines;
    GenerateDefines(Platforms[p].GetData(), m_ShaderData.m_Permutations, defines);
    GenerateDefines(Platforms[p].GetData(), m_ShaderData.m_FixedPermVars, defines);

    ezShaderPermutationBinary shaderPermutationBinary;

    // Generate Shader State Source
    {
      EZ_LOG_BLOCK(pLog, "Preprocessing Shader State Source");

      ezPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(ezLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(ezPreprocessor::FileOpenCB(&ezShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(false);
      pp.SetPassThroughLine(false);

      for (auto& define : defines)
      {
        pp.AddCustomDefine(define);
      }

      bool bFoundUndefinedVars = false;
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const ezPreprocessor::ProcessingEvent& e) {
        if (e.m_Type == ezPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          ezLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File,
                       e.m_pToken->m_uiLine);
        }
      });

      ezStringBuilder sOutput;
      if (pp.Process("ShaderRenderState", sOutput, false).Failed() || bFoundUndefinedVars)
      {
        ezLog::Error(pLog, "Preprocessing the Shader State block failed");
        return EZ_FAILURE;
      }
      else
      {
        if (shaderPermutationBinary.m_StateDescriptor.Load(sOutput).Failed())
        {
          ezLog::Error(pLog, "Failed to interpret the shader state block");
          return EZ_FAILURE;
        }
      }
    }

    for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      spd.m_StageBinary[stage].m_Stage = (ezGALShaderStage::Enum)stage;
      spd.m_StageBinary[stage].m_uiSourceHash = 0;

      if (m_ShaderData.m_ShaderStageSource[stage].IsEmpty())
        continue;
      
      bool bFoundUndefinedVars = false;

      ezPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(ezLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(ezPreprocessor::FileOpenCB(&ezShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(true);
      pp.SetPassThroughUnknownCmdsCB(ezMakeDelegate(&ezShaderCompiler::PassThroughUnknownCommandCB, this));
      pp.SetPassThroughLine(false);
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const ezPreprocessor::ProcessingEvent& e) {
        if (e.m_Type == ezPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          ezLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File,
            e.m_pToken->m_uiLine);
        }
      });

      pp.AddCustomDefine(s_szStageDefines[stage]);
      for (auto& define : defines)
      {
        pp.AddCustomDefine(define);
      }

      ezUInt32 uiSourceStringLen = 0;
      if (pp.Process(m_StageSourceFile[stage], sProcessed[stage], true, true, true).Failed() || bFoundUndefinedVars)
      {
        sProcessed[stage].Clear();
        spd.m_szShaderSource[stage] = m_StageSourceFile[stage];
        uiSourceStringLen = m_StageSourceFile[stage].GetElementCount();

        ezLog::Error(pLog, "Shader preprocessing failed");
        return EZ_FAILURE;
      }
      else
      {
        spd.m_szShaderSource[stage] = sProcessed[stage];
        uiSourceStringLen = sProcessed[stage].GetElementCount();
      }

      spd.m_StageBinary[stage].m_uiSourceHash = ezHashingUtils::xxHash32(spd.m_szShaderSource[stage], uiSourceStringLen);

      if (spd.m_StageBinary[stage].m_uiSourceHash != 0)
      {
        ezShaderStageBinary* pBinary =
            ezShaderStageBinary::LoadStageBinary((ezGALShaderStage::Enum)stage, spd.m_StageBinary[stage].m_uiSourceHash);

        if (pBinary)
        {
          spd.m_StageBinary[stage] = *pBinary;
          spd.m_bWriteToDisk[stage] = pBinary->GetByteCode().IsEmpty();
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
    if (pCompiler->Compile(spd, ezLog::GetThreadLocalLogSystem()).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return EZ_FAILURE;
    }

    for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      if (spd.m_StageBinary[stage].m_uiSourceHash != 0 && spd.m_bWriteToDisk[stage])
      {
        if (spd.m_StageBinary[stage].WriteStageBinary(pLog).Failed())
        {
          ezLog::Error(pLog, "Writing stage {0} binary failed", stage);
          return EZ_FAILURE;
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
    sTemp.AppendFormat("_{0}.ezPermutation", ezArgU(uiPermutationHash, 8, true, 16, true));

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
      ezLog::Error(pLog, "Could not open file for writing: '{0}'", sTemp);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}


void ezShaderCompiler::WriteFailedShaderSource(ezShaderProgramCompiler::ezShaderProgramData& spd, ezLogInterface* pLog)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (spd.m_StageBinary[stage].m_uiSourceHash != 0 && spd.m_bWriteToDisk[stage])
    {
      ezStringBuilder sShaderStageFile = ezShaderManager::GetCacheDirectory();

      sShaderStageFile.AppendPath(ezShaderManager::GetActivePlatform().GetData());
      sShaderStageFile.AppendFormat("/_Failed_{0}_{1}.ezShaderSource", ezGALShaderStage::Names[stage], ezArgU(spd.m_StageBinary[stage].m_uiSourceHash, 8, true, 16, true));

      ezFileWriter StageFileOut;
      if (StageFileOut.Open(sShaderStageFile.GetData()).Succeeded())
      {
        StageFileOut.WriteBytes(spd.m_szShaderSource[stage], ezStringUtils::GetStringElementCount(spd.m_szShaderSource[stage]));
        ezLog::Info(pLog, "Failed shader source written to '{0}'", sShaderStageFile);
      }
    }
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderCompiler);

