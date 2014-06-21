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

  ezStringBuilder sShaderFile = szShaderFile;
  const ezString sShaderName = sShaderFile.GetFileName();

  sShaderFile.PathParentDirectory();
  sShaderFile.AppendPath(s_sPlatform.GetData());
  sShaderFile.AppendPath(sShaderName.GetData());
  sShaderFile.AppendFormat("%08X.compiled", uiShaderHash);

  auto itShaderProgram = s_ShaderPrograms.FindOrAdd(sShaderFile, &bExisted);

  if (!bExisted)
  {
    // shader permutation was not yet loaded, 

    ezFileReader ShaderProgramFile;
    if (ShaderProgramFile.Open(sShaderFile.GetData()).Failed())
    {
      ezShaderCompiler sc;
      sc.CompileShader(szShaderFile, PermGen, s_sPlatform.GetData());
    }

    // try again
    if (ShaderProgramFile.Open(sShaderFile.GetData()).Failed())
      return;

    ezUInt8 uiVersion = 1;
    ShaderProgramFile >> uiVersion;

    EZ_ASSERT(uiVersion == 1, "");

    ezGALShaderCreationDescription CompiledShader;

    ezHybridArray<ezUInt8, 4096> Bytecode;

    for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      ezUInt32 uiBytes = 0;
      ShaderProgramFile >> uiBytes;

      if (uiBytes > 0)
      {
        Bytecode.SetCount(uiBytes);
        ShaderProgramFile.ReadBytes(&Bytecode[0], uiBytes);

        CompiledShader.m_ByteCodes[stage] = new ezGALShaderByteCode(&Bytecode[0], uiBytes);
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


