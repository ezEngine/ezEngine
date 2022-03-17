#include <RendererTest/RendererTestPCH.h>

#include "TestClass.h"
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>


ezGraphicsTest::ezGraphicsTest() = default;

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

ezResult ezGraphicsTest::SetupRenderer()
{
  {
    ezFileSystem::SetSpecialDirectory("testout", ezTestFramework::GetInstance()->GetAbsOutputPath());

    ezStringBuilder sBaseDir = ">sdk/Data/Base/";
    ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
    sReadDir.PathParentDirectory();

    EZ_SUCCEED_OR_RETURN(ezFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", ezFileSystem::AllowWrites)); // for shader files

    EZ_SUCCEED_OR_RETURN(ezFileSystem::AddDataDirectory(sBaseDir, "Base"));

    EZ_SUCCEED_OR_RETURN(ezFileSystem::AddDataDirectory(">eztest/", "ImageComparisonDataDir", "imgout", ezFileSystem::AllowWrites));

    EZ_SUCCEED_OR_RETURN(ezFileSystem::AddDataDirectory(sReadDir, "UnitTestData"));

    sReadDir.Set(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
    EZ_SUCCEED_OR_RETURN(ezFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir"));
  }

#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  constexpr const char* szDefaultRenderer = "Vulkan";
#else
  constexpr const char* szDefaultRenderer = "DX11";
#endif

  const char* szRendererName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
  const char* szShaderModel = "";
  const char* szShaderCompiler = "";
  ezGALDeviceFactory::GetShaderModelAndCompiler(szRendererName, szShaderModel, szShaderCompiler);

  ezShaderManager::Configure(szShaderModel, true);
  EZ_VERIFY(ezPlugin::LoadPlugin(szShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", szShaderCompiler);

  // Create a device
  {
    ezGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bDebugDevice = false;
    m_pDevice = ezGALDeviceFactory::CreateDevice(szRendererName, ezFoundation::GetDefaultAllocator(), DeviceInit);
    if (m_pDevice->Init().Failed())
      return EZ_FAILURE;

    ezGALDevice::SetDefaultDevice(m_pDevice);
  }

  if (m_pDevice->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver" || m_pDevice->GetCapabilities().m_sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics"))
  {
    // Use different images for comparison when running the D3D11 Reference Device
    ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
  }
  else if (m_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("AMD") || m_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Radeon"))
  {
    // Line rendering is different on AMD and requires separate images for tests rendering lines.
    ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_AMD");
  }
  else
  {
    ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
  }

  m_hObjectTransformCB = ezRenderContext::CreateConstantBufferStorage<ObjectCB>();
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Default.ezShader");

  ezStartup::StartupHighLevelSystems();
  return EZ_SUCCESS;
}

ezResult ezGraphicsTest::CreateWindow(ezUInt32 uiResolutionX, ezUInt32 uiResolutionY)
{
  // Create a window for rendering
  {
    ezWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = uiResolutionX;
    WindowCreationDesc.m_Resolution.height = uiResolutionY;
    m_pWindow = EZ_DEFAULT_NEW(ezWindow);
    if (m_pWindow->Initialize(WindowCreationDesc).Failed())
      return EZ_FAILURE;
  }

  // Create a Swapchain
  {
    ezGALSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow = m_pWindow;
    swapChainDesc.m_SampleCount = ezGALMSAASampleCount::None;
    swapChainDesc.m_bAllowScreenshots = true;
    m_hSwapChain = m_pDevice->CreateSwapChain(swapChainDesc);
  }

  {
    ezGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = uiResolutionX;
    texDesc.m_uiHeight = uiResolutionY;
    texDesc.m_Format = ezGALResourceFormat::D24S8;
    texDesc.m_bCreateRenderTarget = true;

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
  }

  return EZ_SUCCESS;
}

void ezGraphicsTest::ShutdownRenderer()
{
  EZ_ASSERT_DEV(m_pWindow == nullptr, "DestroyWindow needs to be called before ShutdownRenderer");
  m_hShader.Invalidate();

  ezRenderContext::DeleteConstantBufferStorage(m_hObjectTransformCB);
  m_hObjectTransformCB.Invalidate();

  ezStartup::ShutdownHighLevelSystems();

  ezResourceManager::FreeAllUnusedResources();

  if (m_pDevice)
  {
    m_pDevice->Shutdown().IgnoreResult();
    EZ_DEFAULT_DELETE(m_pDevice);
  }

  ezFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");
}

void ezGraphicsTest::DestroyWindow()
{
  if (m_pDevice)
  {
    if (!m_hSwapChain.IsInvalidated())
    {
      m_pDevice->DestroySwapChain(m_hSwapChain);
      m_hSwapChain.Invalidate();
    }

    if (!m_hDepthStencilTexture.IsInvalidated())
    {
      m_pDevice->DestroyTexture(m_hDepthStencilTexture);
      m_hDepthStencilTexture.Invalidate();
    }
  }

  if (m_pWindow)
  {
    m_pWindow->Destroy().IgnoreResult();
    EZ_DEFAULT_DELETE(m_pWindow);
  }
}

void ezGraphicsTest::BeginFrame()
{
  m_pDevice->BeginFrame();
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);
}

void ezGraphicsTest::EndFrame()
{
  m_pWindow->ProcessWindowMessages();

  ezRenderContext::GetDefaultInstance()->EndRendering();
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;

  m_pDevice->EndPipeline(m_hSwapChain);

  m_pDevice->EndFrame();

  ezTaskSystem::FinishFrameTasks();
}

ezResult ezGraphicsTest::GetImage(ezImage& img)
{
  auto pCommandEncoder = ezRenderContext::GetDefaultInstance()->GetCommandEncoder();

  ezGALTextureHandle hBBTexture = m_pDevice->GetSwapChain(m_hSwapChain)->GetBackBufferTexture();
  pCommandEncoder->ReadbackTexture(hBBTexture);

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
  ezGALTextureSubresource sourceSubResource;
  ezArrayPtr<ezGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
  pCommandEncoder->CopyTextureReadbackResult(hBBTexture, sourceSubResources, SysMemDescs);

  return EZ_SUCCESS;
}

void ezGraphicsTest::ClearScreen(const ezColor& color)
{
  m_pPass = m_pDevice->BeginPass("RendererTest");

  const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

  ezGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture())).SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
  renderingSetup.m_ClearColor = color;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_bClearDepth = true;
  renderingSetup.m_bClearStencil = true;

  ezRectFloat viewport = ezRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

  ezRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
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
  desc.AddStream(ezGALVertexAttributeSemantic::Color0, ezGALResourceFormat::RGBAUByteNormalized);
  desc.AllocateStreamsFromGeometry(geom, Topology);

  hMesh = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>(szResourceName, std::move(desc), szResourceName);

  return hMesh;
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateSphere(ezInt32 iSubDivs, float fRadius)
{
  ezGeometry geom;
  geom.AddGeodesicSphere(fRadius, static_cast<ezUInt8>(iSubDivs));

  ezStringBuilder sName;
  sName.Format("Sphere_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateTorus(ezInt32 iSubDivs, float fInnerRadius, float fOuterRadius)
{
  ezGeometry geom;
  geom.AddTorus(fInnerRadius, fOuterRadius, static_cast<ezUInt16>(iSubDivs), static_cast<ezUInt16>(iSubDivs), true);

  ezStringBuilder sName;
  sName.Format("Torus_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateBox(float fWidth, float fHeight, float fDepth)
{
  ezGeometry geom;
  geom.AddBox(ezVec3(fWidth, fHeight, fDepth), false);

  ezStringBuilder sName;
  sName.Format("Box_{0}_{1}_{2}", ezArgF(fWidth, 1), ezArgF(fHeight, 1), ezArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateLineBox(float fWidth, float fHeight, float fDepth)
{
  ezGeometry geom;
  geom.AddLineBox(ezVec3(fWidth, fHeight, fDepth));

  ezStringBuilder sName;
  sName.Format("LineBox_{0}_{1}_{2}", ezArgF(fWidth, 1), ezArgF(fHeight, 1), ezArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

void ezGraphicsTest::RenderObject(ezMeshBufferResourceHandle hObject, const ezMat4& mTransform, const ezColor& color, ezBitflags<ezShaderBindFlags> ShaderBindFlags)
{
  ezRenderContext::GetDefaultInstance()->BindShader(m_hShader, ShaderBindFlags);

  // ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  // ezGALContext* pContext = pDevice->GetPrimaryContext();

  ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
  ocb->m_MVP = mTransform;
  ocb->m_Color = color;

  ezRenderContext::GetDefaultInstance()->BindConstantBuffer("PerObject", m_hObjectTransformCB);

  ezRenderContext::GetDefaultInstance()->BindMeshBuffer(hObject);
  ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();
}
