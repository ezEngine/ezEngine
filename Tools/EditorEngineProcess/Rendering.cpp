#include <PCH.h>
#include <EditorEngineProcess/Application.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererCore/Pipeline/RenderHelper.h>
#include <Foundation/Threading/TaskSystem.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>

void ezViewContext::RenderObject(ezGameObject* pObject, const ezMat4& ViewProj)
{
  //const ezVec3 vLocalPos = pObject->GetLocalPosition();
  const ezVec3 vPos = pObject->GetWorldPosition();

  ezSizeU32 wndsize = GetEditorWindow().GetClientAreaSize();

  ezMat4 Model;
  Model.SetTranslationMatrix(vPos);

    ezMat4 ObjectData;

  ObjectData = ViewProj * Model;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pContext = pDevice->GetPrimaryContext();

  pContext->UpdateBuffer(m_hCB, 0, &ObjectData, sizeof(ObjectData));

  pContext->SetConstantBuffer(1, m_hCB);

  ezRenderHelper::DrawMeshBuffer(pContext, m_hSphere);
}


void ezViewContext::Redraw()
{
  if (m_Camera.GetCameraMode() == ezCamera::CameraMode::None)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezInt32 iOwnID = GetViewIndex();

  pDevice->BeginFrame();

  // The ezGALContext class is the main interaction point for draw / compute operations
  ezGALContext* pContext = pDevice->GetPrimaryContext();

  ezSizeU32 wndsize = GetEditorWindow().GetClientAreaSize();

  pContext->SetRenderTargetConfig(m_hBBRT);
  pContext->SetViewport(0.0f, 0.0f, (float) wndsize.width, (float) wndsize.height, 0.0f, 1.0f);

  static float fBlue = 0;
  fBlue = ezMath::Mod(fBlue + 0.01f, 1.0f);
  ezColor c(ezMath::Mod(0.3f * iOwnID, 1.0f), ezMath::Mod(0.1f * iOwnID, 1.0f), fBlue);
  pContext->Clear(c);

  pContext->SetRasterizerState(m_hRasterizerState);
  pContext->SetDepthStencilState(m_hDepthStencilState);

  ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(GetDocumentGuid());

  ezMat4 mViewMatrix, mProjectionMatrix, mViewProjection;
  m_Camera.GetViewMatrix(mViewMatrix);
  m_Camera.GetProjectionMatrix((float) wndsize.width / (float) wndsize.height, ezProjectionDepthRange::ZeroToOne, mProjectionMatrix);
  
  mViewProjection = mProjectionMatrix * mViewMatrix;
  //mViewProjection = m_ProjectionMatrix * m_ViewMatrix;

  if (pDocumentContext && pDocumentContext->m_pWorld)
  {
    pDocumentContext->m_pWorld->Update();

    auto it = pDocumentContext->m_pWorld->GetObjects();
  
    while (it.IsValid())
    {
      RenderObject(&(*it), mViewProjection);

      it.Next();
    }
  }

  pDevice->Present(m_hPrimarySwapChain);

  pDevice->EndFrame();

  ezTaskSystem::FinishFrameTasks();
}

void ezViewContext::SetCamera(ezEngineViewCameraMsg* pMsg)
{
  m_Camera.SetCameraMode((ezCamera::CameraMode) pMsg->m_iCameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);

  ezMat4 mOri;
  mOri.SetLookAtMatrix(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  // TODO: This is somehow buggy
  m_Camera.SetFromMatrix(mOri);

  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);
  

  m_ViewMatrix = pMsg->m_ViewMatrix;
  m_ProjectionMatrix = pMsg->m_ProjMatrix;
}

void ezEditorProcessApp::InitDevice()
{
  // Create a device
  ezGALDeviceCreationDescription DeviceInit;
  DeviceInit.m_bCreatePrimarySwapChain = false;
  DeviceInit.m_bDebugDevice = true;

  s_pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11)(DeviceInit);

  EZ_VERIFY(s_pDevice->Init() == EZ_SUCCESS, "Device init failed!");

  ezGALDevice::SetDefaultDevice(s_pDevice);
}

#include <CoreUtils/Geometry/GeomUtils.h>
#include <Core/ResourceManager/ResourceManager.h>

namespace DontUse
{
  ezMeshBufferResourceHandle CreateSphere(ezInt32 iMesh)
  {
    ezDynamicArray<ezVec3> Vertices;
    ezDynamicArray<ezUInt16> Indices;

      ezMat4 m;
      m.SetIdentity();

    

      ezColor8UNorm col(0, 255, 0);

      ezMat4 mTrans;
      mTrans.SetIdentity();
      mTrans.SetRotationMatrixZ(ezAngle::Degree(90));

      ezGeometry geom;
      geom.AddGeodesicSphere(0.5f, iMesh, ezColor8UNorm(0, 255, 0), mTrans);

      Vertices.Reserve(geom.GetVertices().GetCount());
      Indices.Reserve(geom.GetPolygons().GetCount() * 6);

      for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        Vertices.PushBack(geom.GetVertices()[v].m_vPosition);
      }

      for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
      {
        for (ezUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
        {
          Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
          Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
          Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
        }
      }


    return CreateMesh(Vertices, Indices, iMesh);
  }

  ezMeshBufferResourceHandle CreateMesh(const ezArrayPtr<ezVec3>& pVertices, const ezArrayPtr<ezUInt16>& pIndices, ezInt32 iMesh)
  {
    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);

    desc.AllocateStreams(pVertices.GetCount(), pIndices.GetCount() / 3);

    for (ezUInt32 v = 0; v < pVertices.GetCount(); ++v)
      desc.SetVertexData<ezVec3>(0, v, pVertices[v]);

    for (ezUInt32 t = 0; t < pIndices.GetCount(); t += 3)
      desc.SetTriangleIndices(t / 3, pIndices[t], pIndices[t + 1], pIndices[t + 2]);

    ezMeshBufferResourceHandle hMesh;
    {
      ezStringBuilder s;
      s.Format("MayaMesh%i", iMesh);

      hMesh = ezResourceManager::GetResourceHandle<ezMeshBufferResource>(s.GetData());

      ezResourceLock<ezMeshBufferResource> res(hMesh, ezResourceAcquireMode::PointerOnly);

      if (res->GetLoadingState() != ezResourceLoadState::Loaded)
        ezResourceManager::CreateResource(hMesh, desc);
    }

    return hMesh;
  }

}

