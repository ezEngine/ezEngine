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
class ezGameObject;
class ezViewCameraMsgToEngine;

class ezPickingIDCache
{
public:
  struct ObjectInfo
  {
    EZ_DECLARE_POD_TYPE();

    void* m_pObject;
    const char* m_szObjectType;
    ezUInt32 m_uiObjectID;
  };

  ezUInt32 GeneratePickingID(void* pObject, const char* szObjectType)
  {
    bool bExisted = false;
    auto it = m_ObjectToID.FindOrAdd(pObject, &bExisted);

    if (!bExisted)
    {
      it.Value() = (m_IDtoObject.GetCount() + 1) << 8;
      auto& ref = m_IDtoObject.ExpandAndGetRef();
      ref.m_pObject = pObject;
      ref.m_szObjectType = szObjectType;
      ref.m_uiObjectID = it.Value();
    }

    return it.Value();
  }

  ezUInt32 GenerateCombinedPickingID(ezUInt32 uiObjectPickingID, ezUInt8 uiPartID) { return uiObjectPickingID | (ezUInt32) uiPartID; }

  const ObjectInfo& GetObjectInfo(ezUInt32 uiPickingID) { return m_IDtoObject[(uiPickingID >> 8) - 1]; }

  void Clear() { m_ObjectToID.Clear(); m_IDtoObject.Clear(); }

private:
  ezMap<void*, ezUInt32> m_ObjectToID;
  ezDeque<ObjectInfo> m_IDtoObject;
};

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

private:
  void RenderObject(ezGameObject* pObject, const ezMat4& ViewProj);
  void RenderTranslateGizmo(const ezMat4& mTransformation);
  void RenderScene();
  void UpdateConstantBuffer(const ObjectData& od);

  ezGALRenderTargetConfigHandle m_hBBRT;
  ezConstantBufferResourceHandle m_hObjectTransformCB;
  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALRasterizerStateHandle m_hRasterizerStateGizmo;
  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezGALSwapChainHandle m_hPrimarySwapChain;

  ezShaderResourceHandle m_hShader;
  ezShaderResourceHandle m_hGizmoShader;
  ezMeshBufferResourceHandle m_hSphere;
  ezMeshBufferResourceHandle m_hTranslateGizmo;
  ezGALTextureHandle m_hPickingRT;
  ezGALTextureHandle m_hPickingDepthRT;
  ezGALRenderTargetConfigHandle m_hPickingRenderTargetCfg;
  ezPickingIDCache m_PickingCache;

  ezCamera m_Camera;
  ezView* m_pView;

  static ezDataTransfer m_PickingRenderTargetDT;
};

