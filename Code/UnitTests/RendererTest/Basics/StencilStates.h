#pragma once

#include <RendererTest/TestClass/TestClass.h>

/// Tests stencil buffer operations and compare functions.
class ezRendererTestStencilStates : public ezGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "StencilStates"; }

private:
  enum SubTests
  {
    ST_StencilOperations,
    ST_StencilCompareFunctions,
    ST_StencilRefValue,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Stencil Operations", SubTests::ST_StencilOperations);
    AddSubTest("Stencil Compare Functions", SubTests::ST_StencilCompareFunctions);
    AddSubTest("Stencil Reference Value", SubTests::ST_StencilRefValue);
  }

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  ezTestAppRun SubtestStencilOperations();
  ezTestAppRun SubtestStencilCompareFunctions();
  ezTestAppRun SubtestStencilRefValue();

  void RenderQuad(const ezMat4& mTransform, const ezColor& color, ezBitflags<ezShaderBindFlags> ShaderBindFlags = ezShaderBindFlags::Default);

  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override
  {
    m_iFrame = uiInvocationCount;

    if (iIdentifier == SubTests::ST_StencilOperations)
      return SubtestStencilOperations();

    if (iIdentifier == SubTests::ST_StencilCompareFunctions)
      return SubtestStencilCompareFunctions();

    if (iIdentifier == SubTests::ST_StencilRefValue)
      return SubtestStencilRefValue();

    return ezTestAppRun::Quit;
  }

  ezMeshBufferResourceHandle m_hQuadMesh;
  ezShaderResourceHandle m_hStencilShader;
};
