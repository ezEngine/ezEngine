#include <RendererTestPCH.h>

#include "TestClass.h"
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>



ezGraphicsTest::ezGraphicsTest()
{
  m_pWindow = nullptr;
  m_pDevice = nullptr;
}

ezResult ezGraphicsTest::InitializeSubTest(ezInt32 iIdentifier)
{
  // initialize everything up to 'core'
  ezStartup::StartupCoreSystems();
  return EZ_SUCCESS;
}

ezResult ezGraphicsTest::DeInitializeSubTest(ezInt32 iIdentifier)
{
  // shut down completely
  ezStartup::ShutdownCoreSystems();
  ezMemoryTracker::DumpMemoryLeaks();
  return EZ_SUCCESS;
}

ezSizeU32 ezGraphicsTest::GetResolution() const
{
  return m_pWindow->GetClientAreaSize();
}

ezResult ezGraphicsTest::SetupRenderer(ezUInt32 uiResolutionX, ezUInt32 uiResolutionY)
{
  {
    ezFileSystem::SetSpecialDirectory("testout", ezTestFramework::GetInstance()->GetAbsOutputPath());

    ezStringBuilder sBaseDir = ">sdk/Data/Base/";
    ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
    sReadDir.PathParentDirectory();

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    ezFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", ezFileSystem::AllowWrites); // for shader files

    if (ezFileSystem::AddDataDirectory(sBaseDir, "Base").Failed())
      return EZ_FAILURE;

    if (ezFileSystem::AddDataDirectory(">eztest/", "ImageComparisonDataDir", "imgout", ezFileSystem::AllowWrites).Failed())
      return EZ_FAILURE;

    if (ezFileSystem::AddDataDirectory(sReadDir, "UnitTestData").Failed())
      return EZ_FAILURE;

    sReadDir.Set(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
    if (ezFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir").Failed())
      return EZ_FAILURE;
  }

  // Create a window for rendering
  ezWindowCreationDesc WindowCreationDesc;
  WindowCreationDesc.m_Resolution.width = uiResolutionX;
  WindowCreationDesc.m_Resolution.height = uiResolutionY;
  m_pWindow = EZ_DEFAULT_NEW(ezWindow);
  if (m_pWindow->Initialize(WindowCreationDesc).Failed())
    return EZ_FAILURE;

  // Create a device
  ezGALDeviceCreationDescription DeviceInit;
  DeviceInit.m_bCreatePrimarySwapChain = true;
  DeviceInit.m_bDebugDevice = false;
  DeviceInit.m_PrimarySwapChainDescription.m_pWindow = m_pWindow;
  DeviceInit.m_PrimarySwapChainDescription.m_SampleCount = ezGALMSAASampleCount::None;
  DeviceInit.m_PrimarySwapChainDescription.m_bAllowScreenshots = true;

  m_pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11, DeviceInit);

  if (m_pDevice->Init().Failed())
    return EZ_FAILURE;

  ezGALSwapChainHandle hPrimarySwapChain = m_pDevice->GetPrimarySwapChain();
  const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(hPrimarySwapChain);
  EZ_ASSERT_DEV(pPrimarySwapChain != nullptr, "Failed to init swapchain");

  ezGALDevice::SetDefaultDevice(m_pDevice);

  {
    m_hObjectTransformCB = ezRenderContext::CreateConstantBufferStorage<ObjectCB>();

    ezShaderManager::Configure("DX11_SM40", true);

    EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Default.ezShader");

    ezGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = uiResolutionX;
    texDesc.m_uiHeight = uiResolutionY;
    texDesc.m_Format = ezGALResourceFormat::D24S8;
    texDesc.m_bCreateRenderTarget = true;

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
  }

  ezStartup::StartupHighLevelSystems();

  return EZ_SUCCESS;
}

void ezGraphicsTest::ShutdownRenderer()
{
  m_pDevice->DestroyTexture(m_hDepthStencilTexture);
  m_hDepthStencilTexture.Invalidate();

  m_hShader.Invalidate();

  ezRenderContext::DeleteConstantBufferStorage(m_hObjectTransformCB);
  m_hObjectTransformCB.Invalidate();

  ezStartup::ShutdownHighLevelSystems();

  ezResourceManager::FreeUnusedResources(true);

  if (m_pDevice)
  {
    m_pDevice->Shutdown();
    EZ_DEFAULT_DELETE(m_pDevice);
  }

  if (m_pWindow)
  {
    m_pWindow->Destroy();
    EZ_DEFAULT_DELETE(m_pWindow);
  }

  ezFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");
}

void ezGraphicsTest::BeginFrame()
{
  m_pDevice->BeginFrame();
}

void ezGraphicsTest::EndFrame()
{
  m_pWindow->ProcessWindowMessages();

  m_pDevice->Present(m_pDevice->GetPrimarySwapChain(), false);

  m_pDevice->EndFrame();

  ezTaskSystem::FinishFrameTasks();
}

ezResult ezGraphicsTest::GetImage(ezImage& img)
{
  ezGALContext* pContext = m_pDevice->GetPrimaryContext();

  ezGALTextureHandle hBBTexture = m_pDevice->GetSwapChain(m_pDevice->GetPrimarySwapChain())->GetBackBufferTexture();
  pContext->ReadbackTexture(hBBTexture);

  ezImageHeader header;
  header.SetWidth(m_pWindow->GetClientAreaSize().width);
  header.SetHeight(m_pWindow->GetClientAreaSize().height);
  header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  img.ResetAndAlloc(header);

  ezGALSystemMemoryDescription MemDesc;
  MemDesc.m_pData = img.GetPixelPointer<ezUInt8>();
  MemDesc.m_uiRowPitch = 4 * m_pWindow->GetClientAreaSize().width;
  MemDesc.m_uiSlicePitch = 4 * m_pWindow->GetClientAreaSize().width * m_pWindow->GetClientAreaSize().height;

  ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);

  pContext->CopyTextureReadbackResult(hBBTexture, &SysMemDescs);

  return EZ_SUCCESS;
}

void ezGraphicsTest::ClearScreen(const ezColor& color)
{
  ezGALContext* pContext = m_pDevice->GetPrimaryContext();

  ezGALSwapChainHandle hPrimarySwapChain = m_pDevice->GetPrimarySwapChain();
  const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(hPrimarySwapChain);

  ezGALRenderTagetSetup RTS;
  RTS.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()))
      .SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));

  pContext->SetRenderTargetSetup(RTS);
  pContext->SetViewport(ezRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height),
                        0.0f, 1.0f);
  pContext->Clear(color);
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateMesh(const ezGeometry& geom, const char* szResourceName)
{
  ezMeshBufferResourceHandle hMesh;
  hMesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  ezGALPrimitiveTopology::Enum Topology = ezGALPrimitiveTopology::Triangles;
  if (geom.GetLines().GetCount() > 0)
    Topology = ezGALPrimitiveTopology::Lines;

  ezMeshBufferResourceDescriptor desc;
  desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
  desc.AllocateStreamsFromGeometry(geom, Topology);

  hMesh = ezResourceManager::CreateResource<ezMeshBufferResource>(szResourceName, std::move(desc), szResourceName);

  return hMesh;
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateSphere(ezInt32 iSubDivs, float fRadius)
{
  ezMat4 mTrans;
  mTrans.SetIdentity();

  ezGeometry geom;
  geom.AddGeodesicSphere(fRadius, iSubDivs, ezColorLinearUB(255, 255, 255), mTrans);

  ezStringBuilder sName;
  sName.Format("Sphere_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateTorus(ezInt32 iSubDivs, float fInnerRadius, float fOuterRadius)
{
  ezMat4 mTrans;
  mTrans.SetIdentity();

  ezGeometry geom;
  geom.AddTorus(fInnerRadius, fOuterRadius, iSubDivs, iSubDivs, ezColorLinearUB(255, 255, 255), mTrans);

  ezStringBuilder sName;
  sName.Format("Torus_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateBox(float fWidth, float fHeight, float fDepth)
{
  ezMat4 mTrans;
  mTrans.SetIdentity();

  ezGeometry geom;
  geom.AddBox(ezVec3(fWidth, fHeight, fDepth), ezColorLinearUB(255, 255, 255), mTrans);

  ezStringBuilder sName;
  sName.Format("Box_{0}_{1}_{2}", ezArgF(fWidth, 1), ezArgF(fHeight, 1), ezArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateLineBox(float fWidth, float fHeight, float fDepth)
{
  ezMat4 mTrans;
  mTrans.SetIdentity();

  ezGeometry geom;
  geom.AddLineBox(ezVec3(fWidth, fHeight, fDepth), ezColorLinearUB(255, 255, 255), mTrans);

  ezStringBuilder sName;
  sName.Format("LineBox_{0}_{1}_{2}", ezArgF(fWidth, 1), ezArgF(fHeight, 1), ezArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

void ezGraphicsTest::RenderObject(ezMeshBufferResourceHandle hObject, const ezMat4& mTransform, const ezColor& color,
                                  ezBitflags<ezShaderBindFlags> ShaderBindFlags)
{
  ezRenderContext::GetDefaultInstance()->BindShader(m_hShader, ShaderBindFlags);

  // ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  // ezGALContext* pContext = pDevice->GetPrimaryContext();

  ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
  ocb->m_MVP = mTransform;
  ocb->m_Color = color;

  ezRenderContext::GetDefaultInstance()->BindConstantBuffer("PerObject", m_hObjectTransformCB);

  ezRenderContext::GetDefaultInstance()->BindMeshBuffer(hObject);
  ezRenderContext::GetDefaultInstance()->DrawMeshBuffer();
}
