#include <PCH.h>
#include <EditorPluginTest/Document/TestDocumentWindow.moc.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <CoreUtils/Geometry/OBJLoader.h>
#include <QTimer>

ezSizeU32 ezEmbeddedWindow::GetClientAreaSize() const
{
  ezSizeU32 s;
  s.width = m_pWidget->size().width();
  s.height = m_pWidget->size().height();
  return s;
}

ezWindowHandle ezEmbeddedWindow::GetNativeWindowHandle() const
{
  HWND hWnd = (HWND) m_pWidget->winId();
  return hWnd;
}

ezGALDevice* ezTestDocumentWindow::s_pDevice = nullptr;
ezUInt32 ezTestDocumentWindow::s_uiInstances = 0;

ezTestDocumentWindow::ezTestDocumentWindow(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  m_iCurObject = 2;
  m_fRotY = 0;

  switch (s_uiInstances % 7)
  {
  case 0:
    m_Color = ezColor::GetBlack();
    break;
  case 1:
    m_Color = ezColor::GetCornflowerBlue();
    break;
  case 2:
    m_Color = ezColor::GetWhite();
    break;
  case 3:
    m_Color = ezColor::GetGreen();
    break;
  case 4:
    m_Color = ezColor::GetBlue();
    break;
  case 5:
    m_Color = ezColor::GetPink();
    break;
  case 6:
    m_Color = ezColor::GetYellow();
    break;
  }

  ++s_uiInstances;

  m_pCenterWidget = new ez3DViewWidget(this, this);
  //pCenter->setAutoFillBackground(true);
  m_pCenterWidget->setAutoFillBackground(false);
  setCentralWidget(m_pCenterWidget);

  m_Window.m_pWidget = m_pCenterWidget;

  SetTargetFramerate(15);
}

ezTestDocumentWindow::~ezTestDocumentWindow()
{
  if (!m_hBBRT.IsInvalidated())
    DeinitWindow();
}

void ezTestDocumentWindow::InternalRedraw()
{
  if (s_pDevice == nullptr)
    InitDevice();

  if (m_hBBRT.IsInvalidated())
  {
    InitWindow();
    m_pCenterWidget->HasBeenResized(); // resets the state
  }
  else
  if (m_pCenterWidget->HasBeenResized())
  {
    RecreateRenderTarget();
  }

  // Before starting to render in a frame call this function
  s_pDevice->BeginFrame();

  // The ezGALContext class is the main interaction point for draw / compute operations
  ezGALContext* pContext = s_pDevice->GetPrimaryContext();

  ezSizeU32 wndsize = m_Window.GetClientAreaSize();

  pContext->SetRenderTargetConfig(m_hBBRT);
  pContext->SetViewport(0.0f, 0.0f, (float) wndsize.width, (float) wndsize.height, 0.0f, 1.0f);

  pContext->Clear(m_Color);

  pContext->SetRasterizerState(m_hRasterizerState);
  pContext->SetDepthStencilState(m_hDepthStencilState);

  m_fRotY -= 30.0f * 0.05f;

  ezMat4 ModelRot;
  ModelRot.SetRotationMatrixY(ezAngle::Degree(m_fRotY));

  ezMat4 Model;
  Model.SetIdentity();
  Model.SetScalingFactors(ezVec3(0.75f));


  ezMat4 View;
  View.SetIdentity();
  View.SetLookAtMatrix(ezVec3(0.5f, 1.5f, 2.0f), ezVec3(0.0f, 0.5f, 0.0f), ezVec3(0.0f, 1.0f, 0.0f));

  ezMat4 Proj;
  Proj.SetIdentity();
  Proj.SetPerspectiveProjectionMatrixFromFovY(ezAngle::Degree(80.0f), (float) wndsize.width / (float) wndsize.height, 0.1f, 1000.0f, ezProjectionDepthRange::ZeroToOne);

  TestCB ObjectData;

  ObjectData.mvp = Proj * View * Model * ModelRot;

  pContext->UpdateBuffer(m_hCB, 0, &ObjectData, sizeof(TestCB));

  pContext->SetConstantBuffer(1, m_hCB);

  ezRenderHelper::DrawMeshBuffer(pContext, m_pObj[m_iCurObject]->m_hMeshBuffer);
  m_iCurObject = (m_iCurObject + 1) % MaxObjs;



  s_pDevice->Present(m_hPrimarySwapChain);

  s_pDevice->EndFrame();

  ezTaskSystem::FinishFrameTasks();
}

void ezTestDocumentWindow::InitDevice()
{
  if (s_pDevice)
    return;

  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");

  // Create a device
  ezGALDeviceCreationDescription DeviceInit;
  DeviceInit.m_bCreatePrimarySwapChain = false;
  DeviceInit.m_bDebugDevice = true;

  s_pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11)(DeviceInit);

  EZ_VERIFY(s_pDevice->Init() == EZ_SUCCESS, "Device init failed!");

  ezGALDevice::SetDefaultDevice(s_pDevice);
}

void ezTestDocumentWindow::InitWindow()
{
  RecreateRenderTarget();

  // Create a constant buffer for matrix upload
  m_hCB = s_pDevice->CreateConstantBuffer(sizeof(TestCB));

  for (int i = 0; i < MaxObjs; ++i)
    m_pObj[i] = DontUse::MayaObj::LoadFromFile("ez.obj", s_pDevice, i);

  ezShaderManager::SetPlatform("DX11_SM40", s_pDevice, true);

  int iColorValue = 200;
  ezShaderManager::SetPermutationVariable("COLORED", "0");
  ezShaderManager::SetPermutationVariable("COLORVALUE", ezConversionUtils::ToString(iColorValue).GetData());

  m_hShader = ezResourceManager::GetResourceHandle<ezShaderResource>("Shaders/ez2.shader");

  ezShaderManager::SetActiveShader(m_hShader);
  ezShaderManager::SetPermutationVariable("COLORED", "1");

  ezGALRasterizerStateCreationDescription RasterStateDesc;
  RasterStateDesc.m_bWireFrame = true;
  RasterStateDesc.m_CullMode = ezGALCullMode::Back;
  RasterStateDesc.m_bFrontCounterClockwise = true;
  m_hRasterizerState = s_pDevice->CreateRasterizerState(RasterStateDesc);
  EZ_ASSERT(!m_hRasterizerState.IsInvalidated(), "Couldn't create rasterizer state!");

  ezGALDepthStencilStateCreationDescription DepthStencilStateDesc;
  DepthStencilStateDesc.m_bDepthTest = true;
  DepthStencilStateDesc.m_bDepthWrite = true;
  m_hDepthStencilState = s_pDevice->CreateDepthStencilState(DepthStencilStateDesc);
  EZ_ASSERT(!m_hDepthStencilState.IsInvalidated(), "Couldn't create depth-stencil state!");
}

void ezTestDocumentWindow::DeinitWindow()
{
  for (int i = 0; i < MaxObjs; ++i)
    EZ_DEFAULT_DELETE(m_pObj[i]);

  m_hShader.Invalidate();

  s_pDevice->DestroyRenderTargetConfig(m_hBBRT);
  s_pDevice->DestroySwapChain(m_hPrimarySwapChain);
  s_pDevice->DestroyBuffer(m_hCB);
  s_pDevice->DestroyRasterizerState(m_hRasterizerState);
  s_pDevice->DestroyDepthStencilState(m_hDepthStencilState);
}

void ezTestDocumentWindow::RecreateRenderTarget()
{
  if (!m_hPrimarySwapChain.IsInvalidated())
  {
    s_pDevice->DestroyRenderTargetConfig(m_hBBRT);
    s_pDevice->DestroySwapChain(m_hPrimarySwapChain);
    m_hBBRT.Invalidate();
    m_hPrimarySwapChain.Invalidate();
  }

  ezGALSwapChainCreationDescription scd;
  scd.m_pWindow = &m_Window;
  scd.m_SampleCount = ezGALMSAASampleCount::None;
  scd.m_bCreateDepthStencilBuffer = true;
  scd.m_DepthStencilBufferFormat = ezGALResourceFormat::D24S8;
  scd.m_bAllowScreenshots = true;
  scd.m_bVerticalSynchronization = true;

  m_hPrimarySwapChain = s_pDevice->CreateSwapChain(scd);
  const ezGALSwapChain* pPrimarySwapChain = s_pDevice->GetSwapChain(m_hPrimarySwapChain);

  m_hBBRT = pPrimarySwapChain->GetRenderTargetViewConfig();

}

namespace DontUse
{
  MayaObj* MayaObj::LoadFromFile(const char* szPath, ezGALDevice* pDevice, int i)
  {
    ezDynamicArray<MayaObj::Vertex> Vertices;
    ezDynamicArray<ezUInt16> Indices;

    if (true)
    {
      ezMat4 m;

      ezGeometry geom;
      //geom.AddRectXY(1.0f, 2.0f, ezColor8UNorm(255, 255, 0));

      m.SetRotationMatrixY(ezAngle::Degree(90));
      //geom.AddRectXY(2.0f, 1.0f, ezColor8UNorm(255, 255, 0), m);

      ezColor8UNorm col(0, 255, 0);

      ezMat4 mTrans;
      mTrans.SetIdentity();
      mTrans.SetRotationMatrixZ(ezAngle::Degree(90));
      //mTrans.SetTranslationMatrix(ezVec3(1, 0, 0));
      //mTrans.SetScalingFactors(ezVec3(0.5f, 1, 0.3f));
      //geom.AddGeodesicSphere(0.5f, i, ezColor8UNorm(0, 255, 0), mTrans);

      //geom.AddBox(ezVec3(1, 2, 3), ezColor8UNorm(0, 255, 0), mTrans);
      //geom.AddPyramid(ezVec3(1.0f, 1.5f, 2.0f), col);
      //geom.AddCylinder(1.0f, 1.0f, 1.1f, true, true, 3 * (i + 1), col, mTrans, 0, ezAngle::Degree(270));
      //geom.AddCone(-1.0f, 2.0f, true, 3 * (i + 1), col, mTrans);
      geom.AddHalfSphere(1.0f, i + 3, i + 1 , true, col, mTrans, 0);
      //geom.AddCylinder(1.0f, 1.0f, 0.5f, false, false, i + 3, col, mTrans);
      //geom.AddCapsule(1.0f, 1.5f, i + 3, i + 1, col, mTrans);
      //geom.AddTorus(1.0f, 1.5f, 16 * (i+1), 3 * (i+1), col);


      ezLog::Info("Polygons: %u, Vertices: %u", geom.GetPolygons().GetCount(), geom.GetVertices().GetCount());

      Vertices.Reserve(geom.GetVertices().GetCount());
      Indices.Reserve(geom.GetPolygons().GetCount() * 6);

      for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        MayaObj::Vertex vert;
        vert.pos = geom.GetVertices()[v].m_vPosition;
        vert.norm = geom.GetVertices()[v].m_vNormal;
        vert.tex0 = ezColor(geom.GetVertices()[v].m_Color).GetRGB<float>().GetAsVec2();

        Vertices.PushBack(vert);
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
    }
    else
    {
      ezOBJLoader l;
      if (l.LoadOBJ(szPath).Failed())
        return nullptr;

      Vertices.Reserve(l.m_Faces.GetCount() * 3);
      Indices.Reserve(l.m_Faces.GetCount() * 3);

      for (ezUInt32 f = 0; f < l.m_Faces.GetCount(); ++f)
      {
        ezUInt32 uiFirstVertex = Vertices.GetCount();

        for (ezUInt32 v = 0; v < l.m_Faces[f].m_Vertices.GetCount(); ++v)
        {
          MayaObj::Vertex vert;
          vert.pos = l.m_Positions[l.m_Faces[f].m_Vertices[v].m_uiPositionID];
          vert.tex0 = l.m_TexCoords[l.m_Faces[f].m_Vertices[v].m_uiTexCoordID].GetAsVec2();
          vert.norm = l.m_Normals[l.m_Faces[f].m_Vertices[v].m_uiNormalID];

          Vertices.PushBack(vert);
        }

        for (ezUInt32 v = 2; v < l.m_Faces[f].m_Vertices.GetCount(); ++v)
        {
          Indices.PushBack(uiFirstVertex);
          Indices.PushBack(uiFirstVertex + v - 1);
          Indices.PushBack(uiFirstVertex + v);
        }
      }
    }

    MayaObj* Obj = EZ_DEFAULT_NEW(MayaObj)(Vertices, Indices, pDevice, i);
    return Obj;
  }

  MayaObj::MayaObj(const ezArrayPtr<MayaObj::Vertex>& pVertices, const ezArrayPtr<ezUInt16>& pIndices, ezGALDevice* pDevice, int iMesh)
  {
    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);

    desc.AllocateStreams(pVertices.GetCount(), pIndices.GetCount() / 3);

    // you can do this more efficiently, by just accessing the vertex array directly, this is just more convenient
    for (ezUInt32 v = 0; v < pVertices.GetCount(); ++v)
    {
      desc.SetVertexData<ezVec3>(0, v, pVertices[v].pos);
      desc.SetVertexData<ezVec3>(1, v, pVertices[v].norm);
      desc.SetVertexData<ezVec2>(2, v, pVertices[v].tex0);
    }

    for (ezUInt32 t = 0; t < pIndices.GetCount(); t += 3)
    {
      desc.SetTriangleIndices(t / 3, pIndices[t], pIndices[t + 1], pIndices[t + 2]);
    }

    // there should be a function to generate a unique ID
    // or we should just move this into CreateResource, not sure yet
    {
      ezStringBuilder s;
      s.Format("MayaMesh%i", iMesh);

      m_hMeshBuffer = ezResourceManager::GetResourceHandle<ezMeshBufferResource>(s.GetData());

      ezResourceLock<ezMeshBufferResource> res(m_hMeshBuffer, ezResourceAcquireMode::PointerOnly);

      if (res->GetLoadingState() != ezResourceLoadState::Loaded)
        ezResourceManager::CreateResource(m_hMeshBuffer, desc);
    }
  }

}

