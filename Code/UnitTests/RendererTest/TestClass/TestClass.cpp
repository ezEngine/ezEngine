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
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

ezGraphicsTest::ezGraphicsTest() = default;


ezResult ezGraphicsTest::InitializeTest()
{
  return EZ_SUCCESS;
}

ezResult ezGraphicsTest::DeInitializeTest()
{
  return EZ_SUCCESS;
}

ezResult ezGraphicsTest::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();

  // initialize everything up to 'core'
  ezStartup::StartupCoreSystems();

  if (SetupRenderer().Failed())
    return EZ_FAILURE;
  return EZ_SUCCESS;
}

ezResult ezGraphicsTest::DeInitializeSubTest(ezInt32 iIdentifier)
{
  ShutdownRenderer();
  // shut down completely
  ezStartup::ShutdownCoreSystems();
  ezMemoryTracker::DumpMemoryLeaks();
  return EZ_SUCCESS;
}

ezSizeU32 ezGraphicsTest::GetResolution() const
{
  return m_pWindow->GetClientAreaSize();
}


ezResult ezGraphicsTest::CreateRenderer(ezGALDevice*& out_pDevice)
{
  {
    ezFileSystem::SetSpecialDirectory("testout", ezTestFramework::GetInstance()->GetAbsOutputPath());

    ezStringBuilder sBaseDir = ">sdk/Data/Base/";
    ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
    sReadDir.PathParentDirectory();

    EZ_SUCCEED_OR_RETURN(ezFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", ezFileSystem::AllowWrites)); // for shader files

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

  ezStringView sRendererName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
  const char* szShaderModel = "";
  const char* szShaderCompiler = "";
  ezGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);

  ezShaderManager::Configure(szShaderModel, true);
  if (ezPlugin::LoadPlugin(szShaderCompiler).Failed())
    ezLog::Warning("Shader compiler '{}' plugin not found", szShaderCompiler);

  // Create a device
  {
    ezGALDeviceCreationDescription DeviceInit;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    DeviceInit.m_bDebugDevice = true;
#endif
    out_pDevice = ezGALDeviceFactory::CreateDevice(sRendererName, ezFoundation::GetDefaultAllocator(), DeviceInit);
    if (out_pDevice->Init().Failed())
      return EZ_FAILURE;

    ezGALDevice::SetDefaultDevice(out_pDevice);
  }

  if (sRendererName.IsEqual_NoCase("DX11"))
  {
    if (out_pDevice->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver" || out_pDevice->GetCapabilities().m_sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics"))
    {
      // Use different images for comparison when running the D3D11 Reference Device
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
    }
    else if (out_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("AMD") || out_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Radeon"))
    {
      // Line rendering is different on AMD and requires separate images for tests rendering lines.
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11AMD");
    }
    else if (out_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Nvidia") || out_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("GeForce"))
    {
      // Line rendering is different on AMD and requires separate images for tests rendering lines.
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Nvidia");
    }
    else
    {
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
    }
  }
  else if (sRendererName.IsEqual_NoCase("Vulkan"))
  {
    if (out_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("llvmpipe"))
    {
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_LLVMPIPE");
    }
    else if (out_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("SwiftShader"))
    {
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_SwiftShader");
    }
    else
    {
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_Vulkan");
    }
  }
  return EZ_SUCCESS;
}

ezResult ezGraphicsTest::SetupRenderer()
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::CreateRenderer(m_pDevice));

  m_hObjectTransformCB = ezRenderContext::CreateConstantBufferStorage<ObjectCB>();
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Default.ezShader");

  {
    // Unit cube mesh
    ezGeometry geom;
    geom.AddBox(ezVec3(1.0f), true);

    ezGALPrimitiveTopology::Enum Topology = ezGALPrimitiveTopology::Triangles;
    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::RGFloat);
    desc.AllocateStreamsFromGeometry(geom, Topology);

    m_hCubeUV = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>("Texture2DBox", std::move(desc), "Texture2DBox");
  }

  ezStartup::StartupHighLevelSystems();
  return EZ_SUCCESS;
}

void ezGraphicsTest::ShutdownRenderer()
{
  EZ_ASSERT_DEV(m_pWindow == nullptr, "DestroyWindow needs to be called before ShutdownRenderer");
  m_hShader.Invalidate();
  m_hCubeUV.Invalidate();

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

ezResult ezGraphicsTest::CreateWindow(ezUInt32 uiResolutionX, ezUInt32 uiResolutionY)
{
  // Create a window for rendering
  {
    ezWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = uiResolutionX;
    WindowCreationDesc.m_Resolution.height = uiResolutionY;
    WindowCreationDesc.m_bShowMouseCursor = true;
    m_pWindow = EZ_DEFAULT_NEW(ezWindow);
    if (m_pWindow->Initialize(WindowCreationDesc).Failed())
      return EZ_FAILURE;
  }

  // Create a Swapchain
  {
    ezGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow = m_pWindow;
    swapChainDesc.m_SampleCount = ezGALMSAASampleCount::None;
    swapChainDesc.m_bAllowScreenshots = true;
    m_hSwapChain = ezGALWindowSwapChain::Create(swapChainDesc);
    if (m_hSwapChain.IsInvalidated())
    {
      return EZ_FAILURE;
    }
  }

  {
    ezGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = uiResolutionX;
    texDesc.m_uiHeight = uiResolutionY;
    texDesc.m_Format = ezGALResourceFormat::D24S8;
    texDesc.m_bCreateRenderTarget = true;

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
    if (m_hDepthStencilTexture.IsInvalidated())
    {
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
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
    m_pDevice->WaitIdle();
  }


  if (m_pWindow)
  {
    m_pWindow->Destroy().IgnoreResult();
    EZ_DEFAULT_DELETE(m_pWindow);
  }
}

void ezGraphicsTest::BeginFrame(const char* szPipe)
{
  m_pDevice->BeginFrame(m_iFrame);
  m_pDevice->BeginPipeline(szPipe, m_hSwapChain);
}

void ezGraphicsTest::EndFrame()
{
  m_pWindow->ProcessWindowMessages();

  ezRenderContext::GetDefaultInstance()->ResetContextState();
  m_pDevice->EndPipeline(m_hSwapChain);

  m_pDevice->EndFrame();

  ezTaskSystem::FinishFrameTasks();
}


void ezGraphicsTest::BeginPass(const char* szPassName)
{
  EZ_ASSERT_DEV(m_pPass == nullptr, "Call EndPass first before calling BeginPass again");
  m_pPass = m_pDevice->BeginPass(szPassName);
}


void ezGraphicsTest::EndPass()
{
  EZ_ASSERT_DEV(m_pPass != nullptr, "Call BeginPass first before calling EndPass");
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

ezGALRenderCommandEncoder* ezGraphicsTest::BeginRendering(ezColor clearColor, ezUInt32 uiRenderTargetClearMask, ezRectFloat* pViewport, ezRectU32* pScissor)
{
  const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

  ezGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
  renderingSetup.m_ClearColor = clearColor;
  renderingSetup.m_uiRenderTargetClearMask = uiRenderTargetClearMask;
  if (!m_hDepthStencilTexture.IsInvalidated())
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
    renderingSetup.m_bClearDepth = true;
    renderingSetup.m_bClearStencil = true;
  }
  ezRectFloat viewport = ezRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);
  if (pViewport)
  {
    viewport = *pViewport;
  }

  ezGALRenderCommandEncoder* pCommandEncoder = ezRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
  ezRectU32 scissor = ezRectU32(0, 0, m_pWindow->GetClientAreaSize().width, m_pWindow->GetClientAreaSize().height);
  if (pScissor)
  {
    scissor = *pScissor;
  }
  pCommandEncoder->SetScissorRect(scissor);

  SetClipSpace();
  return pCommandEncoder;
}

void ezGraphicsTest::EndRendering()
{
  ezRenderContext::GetDefaultInstance()->EndRendering();
  m_pWindow->ProcessWindowMessages();
}

void ezGraphicsTest::SetClipSpace()
{
  static ezHashedString sClipSpaceFlipped = ezMakeHashedString("CLIP_SPACE_FLIPPED");
  static ezHashedString sTrue = ezMakeHashedString("TRUE");
  static ezHashedString sFalse = ezMakeHashedString("FALSE");
  ezClipSpaceYMode::Enum clipSpace = ezClipSpaceYMode::RenderToTextureDefault;
  ezRenderContext::GetDefaultInstance()->SetShaderPermutationVariable(sClipSpaceFlipped, clipSpace == ezClipSpaceYMode::Flipped ? sTrue : sFalse);
}

void ezGraphicsTest::RenderCube(ezRectFloat viewport, ezMat4 mMVP, ezUInt32 uiRenderTargetClearMask, ezGALResourceViewHandle hSRV)
{
  ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, uiRenderTargetClearMask, &viewport);

  ezRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", hSRV);
  RenderObject(m_hCubeUV, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);
  if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
  {
    EZ_TEST_IMAGE(m_iFrame, 100);
  }
  EndRendering();
};


ezMat4 ezGraphicsTest::CreateSimpleMVP(float fAspectRatio)
{
  ezCamera cam;
  cam.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(ezVec3(0, 0, 0), ezVec3(0, 0, -1), ezVec3(0, 0, 1));
  ezMat4 mProj;
  cam.GetProjectionMatrix(fAspectRatio, mProj);
  ezMat4 mView = cam.GetViewMatrix();

  ezMat4 mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -1.2f));
  return mProj * mView * mTransform;
}

ezResult ezGraphicsTest::GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber)
{
  auto pCommandEncoder = ezRenderContext::GetDefaultInstance()->GetCommandEncoder();

  ezGALTextureHandle hBBTexture = m_pDevice->GetSwapChain(m_hSwapChain)->GetBackBufferTexture();
  const ezGALTexture* pBackbuffer = ezGALDevice::GetDefaultDevice()->GetTexture(hBBTexture);
  pCommandEncoder->ReadbackTexture(hBBTexture);
  const ezEnum<ezGALResourceFormat> format = pBackbuffer->GetDescription().m_Format;

  ezImageHeader header;
  header.SetWidth(m_pWindow->GetClientAreaSize().width);
  header.SetHeight(m_pWindow->GetClientAreaSize().height);
  header.SetImageFormat(ezTextureUtils::GalFormatToImageFormat(format, true));
  ref_img.ResetAndAlloc(header);

  ezGALSystemMemoryDescription MemDesc;
  MemDesc.m_pData = ref_img.GetPixelPointer<ezUInt8>();
  MemDesc.m_uiRowPitch = 4 * m_pWindow->GetClientAreaSize().width;
  MemDesc.m_uiSlicePitch = 4 * m_pWindow->GetClientAreaSize().width * m_pWindow->GetClientAreaSize().height;

  ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
  ezGALTextureSubresource sourceSubResource;
  ezArrayPtr<ezGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
  pCommandEncoder->CopyTextureReadbackResult(hBBTexture, sourceSubResources, SysMemDescs);

  return EZ_SUCCESS;
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
  sName.SetFormat("Sphere_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateTorus(ezInt32 iSubDivs, float fInnerRadius, float fOuterRadius)
{
  ezGeometry geom;
  geom.AddTorus(fInnerRadius, fOuterRadius, static_cast<ezUInt16>(iSubDivs), static_cast<ezUInt16>(iSubDivs), true);

  ezStringBuilder sName;
  sName.SetFormat("Torus_{0}", iSubDivs);

  return CreateMesh(geom, sName);
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateBox(float fWidth, float fHeight, float fDepth)
{
  ezGeometry geom;
  geom.AddBox(ezVec3(fWidth, fHeight, fDepth), false);

  ezStringBuilder sName;
  sName.SetFormat("Box_{0}_{1}_{2}", ezArgF(fWidth, 1), ezArgF(fHeight, 1), ezArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

ezMeshBufferResourceHandle ezGraphicsTest::CreateLineBox(float fWidth, float fHeight, float fDepth)
{
  ezGeometry geom;
  geom.AddLineBox(ezVec3(fWidth, fHeight, fDepth));

  ezStringBuilder sName;
  sName.SetFormat("LineBox_{0}_{1}_{2}", ezArgF(fWidth, 1), ezArgF(fHeight, 1), ezArgF(fDepth, 1));

  return CreateMesh(geom, sName);
}

void ezGraphicsTest::RenderObject(ezMeshBufferResourceHandle hObject, const ezMat4& mTransform, const ezColor& color, ezBitflags<ezShaderBindFlags> ShaderBindFlags)
{
  ezRenderContext::GetDefaultInstance()->BindShader(m_hShader, ShaderBindFlags);

  ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
  ocb->m_MVP = mTransform;
  ocb->m_Color = color;

  ezRenderContext::GetDefaultInstance()->BindConstantBuffer("PerObject", m_hObjectTransformCB);

  ezRenderContext::GetDefaultInstance()->BindMeshBuffer(hObject);
  ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();
}
