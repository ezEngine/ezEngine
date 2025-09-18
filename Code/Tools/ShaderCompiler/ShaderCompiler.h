#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

class ezShaderCompilerApplication : public ezGameApplication
{
public:
  using SUPER = ezGameApplication;

  ezShaderCompilerApplication();

  virtual void Run() override;

private:
  void PrintConfig();
  ezResult CompileShader(ezStringView sShaderFile);
  ezResult ExtractPermutationVarValues(ezStringView sShaderFile);

  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void Init_LoadProjectPlugins() override {}
  virtual void Init_SetupDefaultResources() override {}
  virtual void Init_ConfigureTags() override {}
  virtual bool Run_ProcessApplicationInput() override { return true; }

  ezPermutationGenerator m_PermutationGenerator;
  ezString m_sPlatforms;
  ezString m_sShaderFiles;
  ezMap<ezString, ezHybridArray<ezString, 4>> m_FixedPermVars;
};
