#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Device/DeviceFactory.h>

// Constant buffer definition is shared between shader code and C++
#include <RendererCore/../../../Data/Samples/MeshRenderSample/Shaders/SampleConstantBuffer.h>


#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
constexpr const char* szDefaultRenderer = "Vulkan";
#else
constexpr const char* szDefaultRenderer = "DX11";
#endif

static ezUInt32 g_uiWindowWidth = 1280;
static ezUInt32 g_uiWindowHeight = 720;

/// \brief Custom window implementation to hook into the "Close" button
class MeshRenderSampleWindow : public ezWindow
{
public:
  virtual void OnClickClose() override { m_bCloseRequested = true; }

  bool m_bCloseRequested = false;
};

/// \brief The application class.
/// Instantiated and run through the EZ_CONSOLEAPP_ENTRY_POINT macro at the end of this file.
class MeshRenderSample : public ezApplication
{
public:
  MeshRenderSample()
    : ezApplication("MeshRenderSample")
  {
  }

  void AfterCoreSystemsStartup() override
  {
    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);

    ezStringBuilder sProjectDir = ">sdk/Data/Samples/MeshRenderSample";
    ezStringBuilder sProjectDirResolved;
    ezFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).AssertSuccess();

    ezFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

    ezFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", ezDataDirUsage::AllowWrites).AssertSuccess();
    ezFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
    ezFileSystem::AddDataDirectory(">project/", "Project", "project", ezDataDirUsage::AllowWrites).AssertSuccess();

    ezStringView sRendererName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);

    const char* szShaderModel = "";
    const char* szShaderCompiler = "";
    ezGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);

    ezShaderManager::Configure(szShaderModel, true);
    ezPlugin::LoadPlugin(szShaderCompiler).AssertSuccess("Shader compiler plugin not found", szShaderCompiler);

    // Create a window for rendering
    {
      ezWindowCreationDesc WindowCreationDesc;
      WindowCreationDesc.m_Resolution.width = g_uiWindowWidth;
      WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;

      m_pWindow = EZ_DEFAULT_NEW(MeshRenderSampleWindow);
      m_pWindow->Initialize(WindowCreationDesc).AssertSuccess();
      m_pWindow->GetInputDevice()->SetShowMouseCursor(true);
      m_pWindow->GetInputDevice()->SetClipMouseCursor(ezMouseCursorClipMode::NoClip);
    }

    // Create a rendering device
    {
      ezGALDeviceCreationDescription DeviceInit;
      DeviceInit.m_bDebugDevice = true;

      m_pDevice = ezGALDeviceFactory::CreateDevice(sRendererName, ezFoundation::GetDefaultAllocator(), DeviceInit);
      EZ_ASSERT_DEV(m_pDevice != nullptr, "Device implemention for '{}' not found", sRendererName);
      EZ_VERIFY(m_pDevice->Init() == EZ_SUCCESS, "Device init failed!");

      ezGALDevice::SetDefaultDevice(m_pDevice);
    }

    // now that we have a window and device, tell the engine to initialize the rendering infrastructure
    ezStartup::StartupHighLevelSystems();

    // Create a Swapchain
    {
      ezGALWindowSwapChainCreationDescription swapChainDesc;
      swapChainDesc.m_pWindow = m_pWindow;
      swapChainDesc.m_SampleCount = ezGALMSAASampleCount::None;
      swapChainDesc.m_bAllowScreenshots = true;
      m_hSwapChain = ezGALWindowSwapChain::Create(swapChainDesc);

      const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

      ezGALTextureCreationDescription texDesc;
      texDesc.m_uiWidth = g_uiWindowWidth;
      texDesc.m_uiHeight = g_uiWindowHeight;
      texDesc.m_Format = ezGALResourceFormat::D24S8;
      texDesc.m_bCreateRenderTarget = true;

      m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);

      m_hBBRTV = m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture());
      m_hBBDSV = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);
    }

    // Setup Shaders and Materials
    {
      // the shader (referenced by the material) also defines the render pipeline state
      // such as backface-culling and depth-testing
      m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/Sample.ezMaterial");

      // Create the mesh that we use for rendering
      m_hMeshBuffer = CreateTorusMesh();
    }

    // Setup constant buffer that this sample uses
    {
      m_hSampleConstants = ezRenderContext::CreateConstantBufferStorage(m_pSampleConstantBuffer);
    }
  }


  Execution Run() override
  {
    m_pWindow->ProcessWindowMessages();

    if (m_pWindow->m_bCloseRequested)
      return Execution::Quit;

    // make sure time goes on
    ezClock::GetGlobalClock()->Update();

    // do the rendering
    {
      // Before starting to render in a frame call this function
      m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
      m_pDevice->BeginFrame();

      ezGALCommandEncoder* pCommandEncoder = m_pDevice->BeginCommands("ezMeshRenderSampleMainPass");

      ezGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_hBBRTV).SetDepthStencilTarget(m_hBBDSV);
      renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
      renderingSetup.m_bClearDepth = true;

      ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, ezRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight));

      const ezMat4 mProj = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(ezAngle::MakeFromDegree(70), (float)g_uiWindowWidth / (float)g_uiWindowHeight, 0.1f, 1000.0f);

      ezRenderContext::GetDefaultInstance()->BindConstantBuffer("ezMeshRenderSampleConstants", m_hSampleConstants);
      ezRenderContext::GetDefaultInstance()->BindMaterial(m_hMaterial);

      m_CameraRotation += ezAngle::MakeFromDegree(ezClock::GetGlobalClock()->GetTimeDiff().AsFloatInSeconds() * 90.0f);
      m_CameraRotation.NormalizeRange();

      ezVec3 vCamPos(0);
      vCamPos.x = ezMath::Sin(m_CameraRotation) * 5.0f;
      vCamPos.y = ezMath::Cos(m_CameraRotation) * 5.0f;
      vCamPos.z = 3.0f;

      const ezMat4 mView = ezGraphicsUtils::CreateLookAtViewMatrix(vCamPos, ezVec3::MakeZero(), ezVec3(0, 0, 1));

      // Update the constant buffer
      {
        ezMeshRenderSampleConstants& cb = m_pSampleConstantBuffer->GetDataForWriting();
        cb.ModelMatrix = mView;
        cb.ViewProjectionMatrix = mProj;
      }

      ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hMeshBuffer);
      ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

      ezRenderContext::GetDefaultInstance()->EndRendering();
      m_pDevice->EndCommands(pCommandEncoder);

      m_pDevice->EndFrame();
      ezRenderContext::GetDefaultInstance()->ResetContextState();
    }

    // needs to be called once per frame to finish resource loading
    ezResourceManager::PerFrameUpdate();

    // tell the task system to finish its work for this frame
    // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
    // uploading GPU data etc.
    ezTaskSystem::FinishFrameTasks();

    return ezApplication::Execution::Continue;
  }

  void BeforeCoreSystemsShutdown() override
  {
    // make sure that no textures are continue to be streamed in while the engine shuts down
    ezResourceManager::EngineAboutToShutdown();

    ezRenderContext::DeleteConstantBufferStorage(m_hSampleConstants);
    m_hSampleConstants.Invalidate();

    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();

    m_hMaterial.Invalidate();
    m_hMeshBuffer.Invalidate();

    // tell the engine that we are about to destroy window and graphics device,
    // and that it therefore needs to cleanup anything that depends on that
    ezStartup::ShutdownHighLevelSystems();

    ezResourceManager::FreeAllUnusedResources();

    m_pDevice->DestroySwapChain(m_hSwapChain);

    // now we can destroy the graphics device
    m_pDevice->Shutdown().AssertSuccess();
    EZ_DEFAULT_DELETE(m_pDevice);

    // finally destroy the window
    m_pWindow->Destroy().AssertSuccess();
    EZ_DEFAULT_DELETE(m_pWindow);
  }

  ezMeshBufferResourceHandle CreateTorusMesh()
  {
    ezGeometry geom;
    ezGeometry::GeoOptions opt;
    opt.m_Color = ezColor::Black;
    geom.AddTorus(1.0f, 2.5f, 64, 32, true, opt);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);

    desc.AllocateStreamsFromGeometry(geom);

    return ezResourceManager::GetOrCreateResource<ezMeshBufferResource>("TorusMesh", std::move(desc));
  }

private:
  MeshRenderSampleWindow* m_pWindow = nullptr;
  ezGALDevice* m_pDevice = nullptr;

  ezGALSwapChainHandle m_hSwapChain;
  ezGALRenderTargetViewHandle m_hBBRTV;
  ezGALRenderTargetViewHandle m_hBBDSV;
  ezGALTextureHandle m_hDepthStencilTexture;

  ezConstantBufferStorageHandle m_hSampleConstants;
  ezConstantBufferStorage<ezMeshRenderSampleConstants>* m_pSampleConstantBuffer;
  ezMaterialResourceHandle m_hMaterial;
  ezMeshBufferResourceHandle m_hMeshBuffer;
  ezAngle m_CameraRotation = ezAngle::MakeZero();
};

EZ_CONSOLEAPP_ENTRY_POINT(MeshRenderSample);
