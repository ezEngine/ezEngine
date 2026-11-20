#pragma once

#include "../TestClass/TestClass.h"
#include <Core/Graphics/Camera.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/RasterizerView.h>

class ezRasterizerTest2 : public ezGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "Rasterizer2"; }

private:
  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  void UpdateDepthTexture(ezUInt32 uiWidth, ezUInt32 uiHeight);
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void CreateCity();

  struct Building
  {
    ezTransform m_Transform;
    ezVec3 m_vExtents;
    ezSharedPtr<const ezRasterizerObject> m_pObject;
  };

  // City
  ezHybridArray<Building, 128> m_Buildings;
  ezSharedPtr<const ezRasterizerObject> m_pGround;
  ezSharedPtr<const ezRasterizerObject> m_pFacade;
  ezUniquePtr<ezRasterizerView> m_pView;

  ezCamera m_Camera;

  // Rendering
  ezMeshBufferResourceHandle m_hLineBox;
  ezShaderResourceHandle m_hLineShader;
  ezGALTextureHandle m_hTexture;
  ezDynamicArray<ezColorLinearUB> m_Pixels;
};
