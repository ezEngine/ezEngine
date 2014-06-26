#include <Graphics/PCH.h>
#include <Graphics/ShaderCompiler/ShaderManager.h>
#include <Graphics/ShaderCompiler/ShaderCompiler.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <RendererFoundation/Context/Context.h>

ezPermutationGenerator ezShaderManager::s_AllowedPermutations;
ezMap<ezString, ezString> ezShaderManager::s_PermutationVariables;
ezString ezShaderManager::s_sPlatform;
ezMap<ezString, ezShaderManager::ShaderConfig> ezShaderManager::s_ShaderConfigs;
bool ezShaderManager::s_bShaderValid = false;
ezGALDevice* ezShaderManager::s_pDevice = nullptr;
ezMap<ezString, ezShaderManager::ShaderProgramData> ezShaderManager::s_ShaderPrograms;

void ezShaderManager::SetPlatform(const char* szPlatform, ezGALDevice* pDevice)
{
  s_pDevice = pDevice;

  ezStringBuilder s = szPlatform;
  s.ToUpper();

  s_sPlatform = s;

  s_AllowedPermutations.ReadFromFile("ShaderPermutations.txt", s.GetData());

  s_PermutationVariables.Clear();

  // initialize all permutation variables
  for (auto it = s_AllowedPermutations.GetPermutationSet().GetIterator(); it.IsValid(); ++it)
  {
    SetPermutationVariable(it.Key().GetData(), it.Value().GetIterator().Key().GetData());
  }
}

void ezShaderManager::SetPermutationVariable(const char* szVariable, const char* szValue)
{
  ezStringBuilder sVar, sVal;
  sVar = szVariable;
  sVal = szValue;

  sVar.ToUpper();
  sVal.ToUpper();

  if (!s_AllowedPermutations.IsValueAllowed(sVar.GetData(), sVal.GetData()))
  {
    ezLog::Error("Invalid Shader Permutation: '%s' cannot be set to value '%s'", sVar.GetData(), sVal.GetData());
    return;
  }

  s_PermutationVariables[sVar] = sVal;
}

void ezShaderManager::BindShader(const char* szShaderFile)
{
  s_bShaderValid = false;

  bool bExisted = false;
  auto itShader = s_ShaderConfigs.FindOrAdd(szShaderFile, &bExisted);

  if (!bExisted)
  {
    itShader.Value().m_bShaderValid = false;

    // not yet loaded shader

    ezFileReader File;
    if (File.Open(szShaderFile).Failed())
      return;

    ezStringBuilder sFileContent;
    sFileContent.ReadAll(File);

    ezHybridArray<ezShaderCompiler::ShaderSection, 16> Sections;
    ezShaderCompiler::GetShaderSections(sFileContent, Sections);

    itShader.Value().m_PermutationVarsUsed = Sections[ezShaderCompiler::PERMUTATIONS].m_Content;

    itShader.Value().m_bShaderValid = true;
  }

  if (!itShader.Value().m_bShaderValid)
    return;

  ezPermutationGenerator PermGen;
  for (auto itPerm = s_PermutationVariables.GetIterator(); itPerm.IsValid(); ++itPerm)
    PermGen.AddPermutation(itPerm.Key().GetData(), itPerm.Value().GetData());

  PermGen.RemoveUnusedPermutations(itShader.Value().m_PermutationVarsUsed);

  EZ_ASSERT(PermGen.GetPermutationCount() == 1, "bla");

  ezDeque<ezPermutationGenerator::PermutationVar> UsedPermVars;
  PermGen.GetPermutation(0, UsedPermVars);

  const ezUInt32 uiShaderHash = ezPermutationGenerator::GetHash(UsedPermVars);

  ezStringBuilder sShaderDir = szShaderFile;
  const ezString sShaderName = sShaderDir.GetFileName();

  sShaderDir.PathParentDirectory();
  sShaderDir.AppendPath(s_sPlatform.GetData());

  ezStringBuilder sShaderFile = sShaderDir;
  sShaderFile.AppendPath(sShaderName.GetData());
  sShaderFile.AppendFormat("%08X.permutation", uiShaderHash);

  auto itShaderProgram = s_ShaderPrograms.FindOrAdd(sShaderFile, &bExisted);

  if (!bExisted)
  {
    // shader permutation was not yet loaded, 

    ezFileReader ShaderProgramFile;
    if (ShaderProgramFile.Open(sShaderFile.GetData()).Failed())
    {
      ezShaderCompiler sc;
      sc.CompileShader(szShaderFile, PermGen, s_sPlatform.GetData());

      // try again
      if (ShaderProgramFile.Open(sShaderFile.GetData()).Failed())
        return;
    }

    ezShaderPermutationBinary spb;
    EZ_VERIFY(spb.Read(ShaderProgramFile).Succeeded(), "Could not read shader permutation file '%s'", sShaderFile.GetData());

    ezGALShaderCreationDescription CompiledShader;

    for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      const ezUInt32 uiStageHash = spb.m_uiShaderStageHashes[stage];

      if (uiStageHash == 0) // not used
        continue;

      // check if this shader stage has already been loaded before
      auto itStage = ezShaderStageBinary::s_ShaderStageBinaries[stage].Find(uiStageHash);

      // if not, load it now
      if (!itStage.IsValid())
      {
        ezStringBuilder sShaderStageFile = sShaderDir;
        sShaderStageFile.AppendFormat("/%08X", uiStageHash);

        ezFileReader StageFileIn;
        EZ_VERIFY(StageFileIn.Open(sShaderStageFile.GetData()).Succeeded(), "Could not open shader stage file '%s'", sShaderStageFile.GetData());

        ezShaderStageBinary ssb;
        EZ_VERIFY(ssb.Read(StageFileIn).Succeeded(), "Could not read shader stage file '%s'", sShaderStageFile.GetData());

        itStage = ezShaderStageBinary::s_ShaderStageBinaries[stage].Insert(uiStageHash, ssb);
      }

      EZ_ASSERT(itStage.IsValid(), "Implementation error");

      // if it is invalid, do not create the shader
      if (itStage.Value().m_Stage < ezGALShaderStage::ENUM_COUNT && !itStage.Value().m_ByteCode.IsEmpty())
      {
        CompiledShader.m_ByteCodes[stage] = new ezGALShaderByteCode(&itStage.Value().m_ByteCode[0], itStage.Value().m_ByteCode.GetCount());
      }
    }

    ezGALShaderHandle hShader = s_pDevice->CreateShader(CompiledShader);

    itShaderProgram.Value().m_hShader = hShader;

    ezGALVertexDeclarationCreationDescription VertDeclDesc;
    VertDeclDesc.m_hShader = hShader;
    VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat, 0, 0, false));
    VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat, 12, 0, false));
    VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat, 24, 0, false));

    itShaderProgram.Value().m_hVertexDeclaration = s_pDevice->CreateVertexDeclaration(VertDeclDesc);
  }

  s_pDevice->GetPrimaryContext()->SetShader(itShaderProgram.Value().m_hShader);
  s_pDevice->GetPrimaryContext()->SetVertexDeclaration(itShaderProgram.Value().m_hVertexDeclaration);
}


