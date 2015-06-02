#pragma once

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <System/Window/Window.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <CoreUtils/Graphics/Camera.h>
#include <CoreUtils/Debugging/DataTransfer.h>

class ezView;
class ezViewCameraMsgToEngine;

struct ObjectData
{
  ezMat4 m_ModelView;
  float m_PickingID[4];
};

class ezViewContext : public ezEngineProcessViewContext
{
public:
  ezViewContext(ezInt32 iViewIndex, ezUuid DocumentGuid) : ezEngineProcessViewContext(iViewIndex, DocumentGuid)
  {
    m_pView = nullptr;
  }

  void SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight);

  void Redraw();

  void SetCamera(ezViewCameraMsgToEngine* pMsg);

  void PickObjectAt(ezUInt16 x, ezUInt16 y);

  void SendViewMessage(ezEditorEngineDocumentMsg* pViewMsg);

private:
  ezGALRenderTargetConfigHandle m_hBBRT;
  ezGALSwapChainHandle m_hPrimarySwapChain;

  ezCamera m_Camera;
  ezView* m_pView;

  ezGALTextureHandle m_hPickingIdRT;
  ezGALTextureHandle m_hPickingDepthRT;
  ezGALRenderTargetConfigHandle m_hPickingRenderTargetCfg;

  /// we need this matrix to compute the world space position of picked pixels
  ezMat4 m_PickingInverseViewProjectionMatrix;

  /// stores the 2D depth buffer image (32 Bit depth precision), to compute pixel positions from
  ezDynamicArray<float> m_PickingResultsDepth;

  /// Stores the 32 Bit picking ID values of each pixel. This can lead back to the ezComponent, etc. that rendered to that pixel
  ezDynamicArray<ezUInt32> m_PickingResultsID;

  /// Stores the optional/additional "part index" of a pickable object. E.g. for a mesh this can be the sub-mesh or material index
  ezDynamicArray<ezUInt16> m_PickingResultsPartIndex;
};

