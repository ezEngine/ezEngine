#pragma once

#include "../TestClass/TestClass.h"
#include <Foundation/Communication/IpcChannel.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererTest/Advanced/OffscreenRenderer.h>

class ezRendererTestAdvancedFeatures : public ezGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "AdvancedFeatures"; }

private:
  enum SubTests
  {
    ST_ReadRenderTarget,
    ST_VertexShaderRenderTargetArrayIndex,
    ST_SharedTexture,
    ST_Tessellation,
    ST_Compute,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,

  };

  virtual void SetupSubTests() override;

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void ReadRenderTarget();
  void VertexShaderRenderTargetArrayIndex();
  void Tessellation();
  void Compute();
  ezTestAppRun SharedTexture();
  void OffscreenProcessMessageFunc(const ezProcessMessage* pMsg);


private:
  ezShaderResourceHandle m_hShader2;

  ezGALTextureHandle m_hTexture2D;
  ezGALTextureResourceViewHandle m_hTexture2DMips[4];
  ezGALTextureHandle m_hTexture2DArray;

  // Tessellation Test
  ezMeshBufferResourceHandle m_hSphereMesh;

  // Shared Texture Test
#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  ezUniquePtr<ezProcess> m_pOffscreenProcess;
  ezUniquePtr<ezIpcChannel> m_pChannel;
  ezUniquePtr<ezIpcProcessMessageProtocol> m_pProtocol;
  ezGALTextureCreationDescription m_SharedTextureDesc;

  static constexpr ezUInt32 s_SharedTextureCount = 3;

  bool m_bExiting = false;
  ezGALTextureHandle m_hSharedTextures[s_SharedTextureCount];
  ezDeque<ezOffscreenTest_SharedTexture> m_SharedTextureQueue;
  ezUInt32 m_uiReceivedTextures = 0;
  float m_fOldProfilingThreshold = 0.0f;
  ezShaderUtils::ezBuiltinShader m_CopyShader;
#endif
};
