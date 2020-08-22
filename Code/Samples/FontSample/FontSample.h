#pragma once

#include <Foundation/Application/Application.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Font/FontResource.h>
#include <RendererCore/Font/TextSprite.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <System/Window/Window.h>

struct ezFontSampleConstants
{
  ezMat4 ModelMatrix;
  ezMat4 ViewProjectionMatrix;
};

struct EZ_ALIGN_16(Vertex)
{
  EZ_DECLARE_POD_TYPE();

  ezVec3 Position;
  ezVec2 TexCoord;
};

class ezGALDevice;

class ezFontRenderingWindow : public ezWindow
{
public:
  ezFontRenderingWindow()
    : ezWindow()
  {
    m_bCloseRequested = false;
  }

  virtual void OnClickClose() override { m_bCloseRequested = true; }

  bool m_bCloseRequested;
};

// A simple application that renders a full screen quad with a single shader and does live reloading of that shader
// Can be used for singed distance rendering experiments or other single shader experiments.
class ezFontRenderingApp : public ezApplication
{
public:
  typedef ezApplication SUPER;

  ezFontRenderingApp();

  virtual ApplicationExecution Run() override;

  virtual void AfterCoreSystemsStartup() override;

  void BeforeCoreSystemsShutdown() override;

private:
  ezFontRenderingWindow* m_pWindow = nullptr;
  ezGALDevice* m_pDevice = nullptr;

  ezGALRenderTargetViewHandle m_hBBRTV;
  ezGALRenderTargetViewHandle m_hBBDSV;
  ezGALTextureHandle m_hDepthStencilTexture;

  ezUniquePtr<ezCamera> m_camera;

  bool m_stuffChanged;
  ezFontResourceHandle m_Font;
  ezShaderResourceHandle m_hFontShader;
  ezTextSpriteDescriptor m_TextSpriteDesc;

  ezConstantBufferStorageHandle m_hSampleConstants;
  ezConstantBufferStorage<ezFontSampleConstants>* m_pSampleConstantBuffer;
  ezVec2 m_vCameraPosition;


  static const ezUInt32 VertexBufferSize = 10000;
  static const ezUInt32 IndexBufferSize = VertexBufferSize * 2;
  ezVertexDeclarationInfo m_VertexDeclarationInfo;
  ezGALBufferHandle m_hVertexBuffer;
  ezGALBufferHandle m_hIndexBuffer;


  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;

  void RenderText();
};
