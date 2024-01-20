#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class ezRendererTestShaderCompiler : public ezGraphicsTest
{
  using SUPER = ezGraphicsTest;

  enum SubTests
  {
    ST_ShaderResources,
  };

public:
  virtual const char* GetTestName() const override { return "ShaderCompiler"; }

private:
  virtual void SetupSubTests() override;

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

private:
  ezShaderResourceHandle m_hUVColorShader;
};
