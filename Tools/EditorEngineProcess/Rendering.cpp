#include <PCH.h>
#include <EditorEngineProcess/Application.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererCore/Pipeline/RenderHelper.h>
#include <Foundation/Threading/TaskSystem.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>

void ezViewContext::Redraw()
{
  ezGALDevice* pDevide = ezGALDevice::GetDefaultDevice();

  ezInt32 iOwnID = GetViewIndex();

  pDevide->BeginFrame();

  // The ezGALContext class is the main interaction point for draw / compute operations
  ezGALContext* pContext = pDevide->GetPrimaryContext();

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

  if (pDocumentContext && pDocumentContext->m_pWorld)
  {
    pDocumentContext->m_pWorld->Update();

    auto it = pDocumentContext->m_pWorld->GetObjects();
  
    while (it.IsValid())
    {
      const ezVec3 vPos = it->GetLocalPosition();

      m_fRotY -= 30.0f * 0.05f;

      ezMat4 ModelRot;
      ModelRot.SetIdentity();
      //ModelRot.SetRotationMatrixY(ezAngle::Degree(m_fRotY));

      ezMat4 Model;
      Model.SetIdentity();
      Model.SetTranslationMatrix(vPos);


      ezMat4 View;
      View.SetIdentity();
      View.SetLookAtMatrix(ezVec3(0.5f, 1.5f, 2.0f), ezVec3(0.0f, 0.5f, 0.0f), ezVec3(0.0f, 1.0f, 0.0f));

      ezMat4 Proj;
      Proj.SetIdentity();
      Proj.SetPerspectiveProjectionMatrixFromFovY(ezAngle::Degree(80.0f), (float) wndsize.width / (float) wndsize.height, 0.1f, 1000.0f, ezProjectionDepthRange::ZeroToOne);

      ezMat4 ObjectData;

      ObjectData = Proj * View * Model * ModelRot;

      pContext->UpdateBuffer(m_hCB, 0, &ObjectData, sizeof(ObjectData));

      pContext->SetConstantBuffer(1, m_hCB);

      ezRenderHelper::DrawMeshBuffer(pContext, m_hSphere);

      it.Next();
    }
  }

  pDevide->Present(m_hPrimarySwapChain);

  pDevide->EndFrame();

  ezTaskSystem::FinishFrameTasks();
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

