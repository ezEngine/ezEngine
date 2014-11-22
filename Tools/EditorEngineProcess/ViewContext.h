#pragma once

#include <EditorFramework/EditorApp.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <System/Window/Window.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>

class ezViewContext : public ezEngineProcessViewContext
{
public:
  ezViewContext(ezInt32 iViewIndex, ezUuid DocumentGuid) : ezEngineProcessViewContext(iViewIndex, DocumentGuid)
  {
    m_fRotY = 0;
  }

  void SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight);

  void Redraw();

private:
  ezGALRenderTargetConfigHandle m_hBBRT;
  ezGALBufferHandle m_hCB;
  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezGALSwapChainHandle m_hPrimarySwapChain;

  float m_fRotY;
  ezShaderResourceHandle m_hShader;
  ezMeshBufferResourceHandle m_hSphere;
};

namespace DontUse
{
  ezMeshBufferResourceHandle CreateMesh(const ezArrayPtr<ezVec3>& pVertices, const ezArrayPtr<ezUInt16>& pIndices, ezInt32 iMesh);

  ezMeshBufferResourceHandle CreateSphere(ezInt32 iMesh);
}

