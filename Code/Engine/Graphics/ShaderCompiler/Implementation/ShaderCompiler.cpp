#include <Graphics/PCH.h>
#include <Graphics/ShaderCompiler/ShaderCompiler.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezShaderProgramCompiler);

void ezShaderCompiler::SplitIntoSections(const ezStringBuilder& sContent, ezHybridArray<ShaderSection, 16>& inout_Sections)
{
  for (ezUInt32 s = 0; s < inout_Sections.GetCount(); ++s)
  {
    inout_Sections[s].m_szSectionStart = sContent.FindSubString_NoCase(inout_Sections[s].m_sName.GetData());

    if (inout_Sections[s].m_szSectionStart != nullptr)
      inout_Sections[s].m_Content = ezStringIterator(inout_Sections[s].m_szSectionStart + inout_Sections[s].m_sName.GetElementCount());
  }

  for (ezUInt32 s = 0; s < inout_Sections.GetCount(); ++s)
  {
    if (inout_Sections[s].m_szSectionStart == nullptr)
      continue;

    for (ezUInt32 s2 = 0; s2 < inout_Sections.GetCount(); ++s2)
    {
      if (s == s2)
        continue;

      if (inout_Sections[s2].m_szSectionStart > inout_Sections[s].m_szSectionStart)
      {
        const char* szContentStart = inout_Sections[s].m_Content.GetStart();
        const char* szSectionEnd = ezMath::Min(inout_Sections[s].m_Content.GetEnd(), inout_Sections[s2].m_szSectionStart);

        inout_Sections[s].m_Content = ezStringIterator(szContentStart, szSectionEnd, szContentStart);
      }
    }
  }
}

void ezShaderCompiler::GetShaderSections(const ezStringBuilder& sContent, ezHybridArray<ShaderSection, 16>& out_Sections)
{
  out_Sections.Clear();
  out_Sections.PushBack(ShaderSection("[PLATFORMS]"));
  out_Sections.PushBack(ShaderSection("[PERMUTATIONS]"));
  out_Sections.PushBack(ShaderSection("[VERTEXSHADER]"));
  out_Sections.PushBack(ShaderSection("[HULLSHADER]"));
  out_Sections.PushBack(ShaderSection("[DOMAINSHADER]"));
  out_Sections.PushBack(ShaderSection("[GEOMETRYSHADER]"));
  out_Sections.PushBack(ShaderSection("[PIXELSHADER]"));
  out_Sections.PushBack(ShaderSection("[COMPUTESHADER]"));

  SplitIntoSections(sContent, out_Sections);
}

ezResult ezShaderCompiler::FileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent)
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

  ezFileReader r;
  if (r.Open(szAbsoluteFile).Failed())
    return EZ_FAILURE;

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
  ezTokenizedFileCache FileCache;
  ezStringBuilder sFileContent, sTemp;

  {
    ezFileReader File;
    if (File.Open(szFile).Failed())
      return EZ_FAILURE;
    
    sFileContent.ReadAll(File);
  }

  ezHybridArray<ShaderSection, 16> Sections;
  GetShaderSections(sFileContent, Sections);

  sTemp = Sections[ShaderSections::PLATFORMS].m_Content;
  sTemp.ToUpper();

  m_ShaderData.m_Platforms = sTemp;
  m_ShaderData.m_Permutations = Sections[ShaderSections::PERMUTATIONS].m_Content;

  ezPermutationGenerator Generator = MainGenerator;
  Generator.RemoveUnusedPermutations(m_ShaderData.m_Permutations);

  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_ShaderData.m_ShaderStageSource[stage] = Sections[ShaderSections::VERTEXSHADER + stage].m_Content;

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

  ezStringBuilder sProcessed[ezGALShaderStage::ENUM_COUNT];

  ezDeque<ezPermutationGenerator::PermutationVar> PermVars;

  ezShaderProgramCompiler* pCompiler = ezShaderProgramCompiler::GetFirstInstance();
  while (pCompiler)
  {
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
        Generator.GetPermutation(uiPermutation, PermVars);

        const ezUInt32 uiPermutationHash = Generator.GetHash(PermVars);

        ezShaderProgramData spd;
        spd.m_szPlatform = Platforms[p].GetData();

        for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
        {
          ezPreprocessor pp;
          pp.SetCustomFileCache(&FileCache);
          pp.SetLogInterface(ezGlobalLog::GetInstance());
          pp.SetFileOpenFunction(ezPreprocessor::FileOpenCB(&ezShaderCompiler::FileOpen, this));
          pp.SetPassThroughPragma(true);
          pp.SetPassThroughUnknownCmds(true);
          pp.SetPassThroughLine(true);

          sTemp = Platforms[p];
          sTemp.ToUpper();

          pp.AddCustomDefine(sTemp.GetData());

          for (ezUInt32 pv = 0; pv < PermVars.GetCount(); ++pv)
          {
            sTemp.Format("%s %s", PermVars[pv].m_sVariable.GetData(), PermVars[pv].m_sValue.GetData());
            pp.AddCustomDefine(sTemp.GetData());
          }

          pp.Process(m_StageSourceFile[stage].GetData(), sProcessed[stage], true);

          spd.m_szShaderSource[stage] = sProcessed[stage].GetData();

          spd.m_StageBinary[stage].m_Stage = (ezGALShaderStage::Enum) stage;
          spd.m_StageBinary[stage].m_uiSourceHash = ezHashing::MurmurHash(ezHashing::StringWrapper(sProcessed[stage].GetData()));
        }

        if (pCompiler->Compile(spd, ezGlobalLog::GetInstance()).Failed())
          ezLog::Error("Shader compilation failed.");

        ezShaderPermutationBinary spb;

        for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
        {
          spb.m_uiShaderStageHashes[stage] = spd.m_StageBinary[stage].m_uiSourceHash;

          if (spd.m_StageBinary[stage].m_uiSourceHash != 0)
          {
            sTemp = Platforms[p];
            sTemp.AppendFormat("/%08X", spd.m_StageBinary[stage].m_uiSourceHash);

            ezFileWriter StageFileOut;
            EZ_VERIFY(StageFileOut.Open(sTemp.GetData()).Succeeded(), "Could not write to file '%s'", sTemp.GetData());
            EZ_VERIFY(spd.m_StageBinary[stage].Write(StageFileOut).Succeeded(), "Writing shader stage %u to file '%s' failed.", stage, sTemp.GetData());
          }
        }

        sTemp = Platforms[p];
        sTemp.AppendPath(szFile);
        sTemp.ChangeFileExtension("");
        if (sTemp.EndsWith("."))
          sTemp.Shrink(0, 1);
        sTemp.AppendFormat("%08X.permutation", uiPermutationHash);

        ezFileWriter PermutationFileOut;
        EZ_VERIFY(PermutationFileOut.Open(sTemp.GetData()).Succeeded(), "Could not write file '%s'", sTemp.GetData());
          spb.Write(PermutationFileOut);
      }
    }

    pCompiler = pCompiler->GetNextInstance();
  }

  return EZ_SUCCESS;
}


