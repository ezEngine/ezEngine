#include <PCH.h>
#include <EditorEngineProcess/Application.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererCore/RendererCore.h>
#include <Foundation/Threading/TaskSystem.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <Core/ResourceManager/ResourceManager.h>

ezDataTransfer ezViewContext::m_PickingRenderTargetDT;

ezMeshBufferResourceHandle CreateMeshResource(const ezGeometry& geom, const char* szResourceName)
{
  ezDynamicArray<ezUInt16> Indices;
  Indices.Reserve(geom.GetPolygons().GetCount() * 6);

  for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
  {
    for (ezUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
    {
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
      Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
    }
  }

  ezMeshBufferResourceDescriptor desc;
  desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);

  desc.AllocateStreams(geom.GetVertices().GetCount(), Indices.GetCount() / 3);

  for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
  {
    desc.SetVertexData<ezVec3>(0, v, geom.GetVertices()[v].m_vPosition);
    desc.SetVertexData<ezColorLinearUB>(1, v, geom.GetVertices()[v].m_Color);
  }

  for (ezUInt32 t = 0; t < Indices.GetCount(); t += 3)
  {
    desc.SetTriangleIndices(t / 3, Indices[t], Indices[t + 1], Indices[t + 2]);
  }

  ezMeshBufferResourceHandle hMesh = ezResourceManager::GetCreatedResource<ezMeshBufferResource>(szResourceName);
    
  if (!hMesh.IsValid())
    hMesh = ezResourceManager::CreateResource<ezMeshBufferResource>(szResourceName, desc);

  return hMesh;
}

ezMeshBufferResourceHandle CreateTranslateGizmoMesh()
{
  const float fThickness = 0.01f;
  const float fLength = 0.5f;
  const float fRectSize = fLength / 3.0f;

  ezMat4 m;
  m.SetIdentity();

  ezGeometry geom;

  //geom.AddGeodesicSphere(fThickness * 3.0f, 1, ezColorLinearUB(255, 255, 0), m, 0);
  geom.AddBox(ezVec3(fThickness * 5.0f), ezColorLinearUB(255, 255, 0), m, 0);

  m.SetRotationMatrixZ(ezAngle::Degree(-90.0f));
  geom.AddCylinder(fThickness, fThickness, fLength, false, true, 16, ezColorLinearUB(255, 0, 0), m, 1);

  m.SetTranslationVector(ezVec3(fLength * 0.5f, 0, 0));
  geom.AddCone(fThickness * 3.0f, fThickness * 6.0f, true, 16, ezColorLinearUB(255, 0, 0), m, 1);

  m.SetIdentity();
  geom.AddCylinder(fThickness, fThickness, fLength, false, true, 16, ezColorLinearUB(0, 255, 0), m, 2);

  m.SetTranslationVector(ezVec3(0, fLength * 0.5f, 0));
  geom.AddCone(fThickness * 3.0f, fThickness * 6.0f, true, 16, ezColorLinearUB(0, 255, 0), m, 2);

  m.SetRotationMatrixX(ezAngle::Degree(90.0f));
  geom.AddCylinder(fThickness, fThickness, fLength, false, true, 16, ezColorLinearUB(0, 0, 255), m, 3);

  m.SetTranslationVector(ezVec3(0, 0, fLength * 0.5f));
  geom.AddCone(fThickness * 3.0f, fThickness * 6.0f, true, 16, ezColorLinearUB(0, 0, 255), m, 3);


  ezUInt8 uiDark = 200;
  ezUInt8 uiLight = 255;

  m.SetRotationMatrixY(ezAngle::Degree(-90.0f));
  geom.AddRectXY(ezVec2(fRectSize), ezColorLinearUB(uiLight, uiDark, uiDark), m, 1);
  m.SetRotationMatrixY(ezAngle::Degree( 90.0f));
  geom.AddRectXY(ezVec2(fRectSize), ezColorLinearUB(uiLight, uiDark, uiDark), m, 1);

  m.SetRotationMatrixX(ezAngle::Degree(-90.0f));
  geom.AddRectXY(ezVec2(fRectSize), ezColorLinearUB(uiDark, uiLight, uiDark), m, 2);
  m.SetRotationMatrixX(ezAngle::Degree( 90.0f));
  geom.AddRectXY(ezVec2(fRectSize), ezColorLinearUB(uiDark, uiLight, uiDark), m, 2);

  m.SetIdentity();
  geom.AddRectXY(ezVec2(fRectSize), ezColorLinearUB(uiDark, uiDark, uiLight), m, 3);
  m.SetRotationMatrixX(ezAngle::Degree(180.0f));
  geom.AddRectXY(ezVec2(fRectSize), ezColorLinearUB(uiDark, uiDark, uiLight), m, 3);

  return CreateMeshResource(geom, "TranslateGizmo");
}

void ezViewContext::RenderTranslateGizmo(const ezMat4& mTransformation)
{
  ezUInt32 uiPickingID = m_PickingCache.GeneratePickingID(nullptr, "ezTranslateGizmo");

  ezRendererCore::SetActiveShader(m_hGizmoShader);

  ObjectData od;
  od.m_ModelView = m_ProjectionMatrix * m_ViewMatrix * mTransformation;
  od.m_PickingID[0] = (uiPickingID & 0xFF) / 255.0f;
  od.m_PickingID[1] = ((uiPickingID & 0xFF00) >> 8) / 255.0f;
  od.m_PickingID[2] = ((uiPickingID & 0xFF0000) >> 16) / 255.0f;
  od.m_PickingID[3] = ((uiPickingID & 0xFF000000) >> 24) / 255.0f;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pContext = pDevice->GetPrimaryContext();

  pContext->SetRasterizerState(m_hRasterizerStateGizmo);

  pContext->UpdateBuffer(m_hCB, 0, &od, sizeof(ObjectData));

  pContext->SetConstantBuffer(1, m_hCB);

  ezRendererCore::DrawMeshBuffer(pContext, m_hTranslateGizmo);
}

void ezViewContext::RenderObject(ezGameObject* pObject, const ezMat4& ViewProj)
{
  ezUInt32 uiPickingID = m_PickingCache.GeneratePickingID(pObject, "ezGameObject");

  ezRendererCore::SetActiveShader(m_hShader);

  const ezVec3 vPos = pObject->GetWorldPosition();

  ezMat4 Model;
  Model.SetTranslationMatrix(vPos);

  ObjectData od;
  od.m_ModelView = ViewProj * Model;
  od.m_PickingID[0] = (uiPickingID & 0xFF) / 255.0f;
  od.m_PickingID[1] = ((uiPickingID & 0xFF00) >> 8) / 255.0f;
  od.m_PickingID[2] = ((uiPickingID & 0xFF0000) >> 16) / 255.0f;
  od.m_PickingID[3] = ((uiPickingID & 0xFF000000) >> 24) / 255.0f;


  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pContext = pDevice->GetPrimaryContext();

  pContext->UpdateBuffer(m_hCB, 0, &od, sizeof(ObjectData));

  pContext->SetConstantBuffer(1, m_hCB);

  ezRendererCore::DrawMeshBuffer(pContext, m_hSphere);
}

void ezViewContext::RenderScene()
{
  ezSizeU32 wndsize = GetEditorWindow().GetClientAreaSize();

  ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(GetDocumentGuid());

  ezMat4 mViewMatrix, mProjectionMatrix, mViewProjection;
  m_Camera.GetViewMatrix(mViewMatrix);
  m_Camera.GetProjectionMatrix((float) wndsize.width / (float) wndsize.height, ezProjectionDepthRange::ZeroToOne, mProjectionMatrix);
  
  mViewProjection = mProjectionMatrix * mViewMatrix;
  //mViewProjection = m_ProjectionMatrix * m_ViewMatrix;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pContext = pDevice->GetPrimaryContext();

  pContext->SetRasterizerState(m_hRasterizerState);
  pContext->SetDepthStencilState(m_hDepthStencilState);

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

  RenderTranslateGizmo(ezMat4::IdentityMatrix());
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

  pContext->SetViewport(0.0f, 0.0f, (float) wndsize.width, (float) wndsize.height, 0.0f, 1.0f);

  {
    m_PickingCache.Clear();

    pContext->SetRenderTargetConfig(m_hPickingRenderTargetCfg);

    pContext->Clear(ezColor::Black);
    ezRendererCore::SetShaderPermutationVariable("EDITOR_PICKING", "1");

    RenderScene();
  }

  {
    pContext->SetRenderTargetConfig(m_hBBRT);

    ezColor c = ezColor::CornflowerBlue * 0.25f; // The original! * 0.25f
    pContext->Clear(c);
    ezRendererCore::SetShaderPermutationVariable("EDITOR_PICKING", "0");

    RenderScene();
  }

  pDevice->Present(m_hPrimarySwapChain);

  if (m_PickingRenderTargetDT.IsTransferRequested())
  {
    pContext->ReadbackTexture(m_hPickingRT);

    ezDynamicArray<ezUInt8> ImageContent;
    ImageContent.SetCount(4 * wndsize.width * wndsize.height);

    ezGALSystemMemoryDescription MemDesc;
    MemDesc.m_pData = ImageContent.GetData();
    MemDesc.m_uiRowPitch = 4 * wndsize.width;
    MemDesc.m_uiSlicePitch = 4 * wndsize.width * wndsize.height;

    ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);

    pContext->CopyTextureReadbackResult(m_hPickingRT, &SysMemDescs);

    for (ezUInt32 i = 0; i < ImageContent.GetCount(); i += 4)
    {
      ezMath::Swap(ImageContent[i + 0], ImageContent[i + 2]);
    }

    ezDataTransferObject DataObject(m_PickingRenderTargetDT, "Picking IDs", "image/rgba8", "rgba");
    DataObject.GetWriter() << wndsize.width;
    DataObject.GetWriter() << wndsize.height;
    DataObject.GetWriter().WriteBytes(ImageContent.GetData(), ImageContent.GetCount());

    DataObject.Transmit();
  }

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
  ezMeshBufferResourceHandle CreateSphereMesh(ezInt32 iMesh)
  {
    ezDynamicArray<ezVec3> Vertices;
    ezDynamicArray<ezUInt16> Indices;

    ezMat4 m;
    m.SetIdentity();
    
    ezColorLinearUB col(0, 255, 0);

    ezMat4 mTrans;
    mTrans.SetIdentity();
    mTrans.SetRotationMatrixZ(ezAngle::Degree(90));

    ezGeometry geom;
    geom.AddGeodesicSphere(0.5f, iMesh, ezColorLinearUB(0, 255, 0), mTrans);

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


    return CreateMeshResource(Vertices, Indices, iMesh);
  }

  ezMeshBufferResourceHandle CreateMeshResource(const ezArrayPtr<ezVec3>& pVertices, const ezArrayPtr<ezUInt16>& pIndices, ezInt32 iMesh)
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

      hMesh = ezResourceManager::GetCreatedResource<ezMeshBufferResource>(s);

      if (!hMesh.IsValid())
        hMesh = ezResourceManager::CreateResource<ezMeshBufferResource>(s.GetData(), desc);
    }

    return hMesh;
  }

}

