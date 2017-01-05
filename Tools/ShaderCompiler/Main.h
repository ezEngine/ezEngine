#pragma once

#include <GameFoundation/GameApplication/GameApplication.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

class ezShaderCompilerApplication : public ezGameApplication
{
public:
  ezShaderCompilerApplication();

  virtual ezApplication::ApplicationExecution Run() override;

private:
  void PrintConfig();
  ezResult CompileShader(const char* szShaderFile);
  ezResult ExtractPermutationVarValues(const char* szShaderFile);

  virtual void BeforeCoreStartup() override;
  virtual void AfterCoreStartup() override;
  virtual void BeforeCoreShutdown() override;
  virtual void DoLoadCustomPlugins() override;
  virtual void DoLoadPluginsFromConfig() override {}
  virtual void DoSetupDefaultResources() override {}
  virtual void DoConfigureInput(bool bReinitialize) override {}
  virtual void DoLoadTags() override {}
  virtual void ProcessApplicationInput() override {}

  ezPermutationGenerator m_PermutationGenerator;
  ezString m_sPlatforms;
  ezString m_sShaderFiles;
  ezMap<ezString, ezHybridArray<ezString, 4>> m_FixedPermVars;
};


