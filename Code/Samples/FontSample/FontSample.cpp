#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <FontSample.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Font/TextData.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

static ezUInt32 g_uiWindowWidth = 640;
static ezUInt32 g_uiWindowHeight = 480;

ezFontRenderingApp::ezFontRenderingApp()
  : ezApplication("Font Rendering")
{
  m_vCameraPosition.SetZero();

  m_TextSpriteDesc.Anchor = ezTextSpriteAnchor::TopLeft;
  m_TextSpriteDesc.HorizontalAlignment = ezTextHorizontalAlignment::Left;
  m_TextSpriteDesc.VerticalAlignment = ezTextVerticalAlignment::Top;
  m_TextSpriteDesc.FontSize = 30;
  m_TextSpriteDesc.Width = 640;
  m_TextSpriteDesc.Height = 480;
  m_TextSpriteDesc.Color = ezColor::Red;
  m_TextSpriteDesc.Text = "Hello World. This is a test";
  m_TextSpriteDesc.WrapText = true;
  m_TextSpriteDesc.BreakTextWhenWrapped = true;
}

ezApplication::ApplicationExecution ezFontRenderingApp::Run()
{
  m_pWindow->ProcessWindowMessages();

  if (m_pWindow->m_bCloseRequested || ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
    return ApplicationExecution::Quit;

  // make sure time goes on
  ezClock::GetGlobalClock()->Update();

  // update all input state
  ezInputManager::Update(ezClock::GetGlobalClock()->GetTimeDiff());

  // make sure telemetry is sent out regularly
  ezTelemetry::PerFrameUpdate();

  // do the rendering
  {
    // Before starting to render in a frame call this function
    m_pDevice->BeginFrame();

    // The ezGALContext class is the main interaction point for draw / compute operations
    ezGALContext* pContext = m_pDevice->GetPrimaryContext();


    ezGALRenderTargetSetup RTS;
    RTS.SetRenderTarget(0, m_hBBRTV).SetDepthStencilTarget(m_hBBDSV);

    pContext->SetRenderTargetSetup(RTS);
    pContext->SetViewport(ezRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight), 0.0f, 1.0f);
    pContext->Clear(ezColor::Black);

    pContext->SetRasterizerState(m_hRasterizerState);
    pContext->SetDepthStencilState(m_hDepthStencilState);

    RenderText();

    m_pDevice->Present(m_pDevice->GetPrimarySwapChain(), true);

    m_pDevice->EndFrame();
  }

  // needs to be called once per frame
  ezResourceManager::PerFrameUpdate();

  // tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  ezTaskSystem::FinishFrameTasks();

  return ezApplication::Continue;
}

void ezFontRenderingApp::AfterCoreSystemsStartup()
{
  ezStringBuilder sProjectDir = ">sdk/Data/Samples/FontSample";
  ezStringBuilder sProjectDirResolved;
  ezFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved);

  ezFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

  // setup the 'asset management system'
  {
    // which redirection table to search
    ezDataDirectory::FolderType::s_sRedirectionFile = "AssetCache/LookupTable.ezAsset";
    // which platform assets to use
    ezDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/PC/";
  }

  ezFileSystem::AddDataDirectory("", "", ":", ezFileSystem::AllowWrites);
  ezFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", ezFileSystem::AllowWrites);              // writing to the binary directory
  ezFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", ezFileSystem::AllowWrites); // for shader files
  ezFileSystem::AddDataDirectory(">user/ezEngine Project/FontSample", "AppData", "appdata",
    ezFileSystem::AllowWrites); // app user data

  ezFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base");
  ezFileSystem::AddDataDirectory(">sdk/Data/FreeContent", "Shared", "shared");
  ezFileSystem::AddDataDirectory(">project/", "Project", "project", ezFileSystem::AllowWrites);

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  ezTelemetry::CreateServer();
  ezPlugin::LoadPlugin("ezInspectorPlugin");

  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");

  // Register Input
  {
    ezInputActionConfig cfg;

    cfg = ezInputManager::GetInputActionConfig("Main", "CloseApp");
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyEscape;
    ezInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);
  }

  // Create a window for rendering
  {
    ezWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = g_uiWindowWidth;
    WindowCreationDesc.m_Resolution.height = g_uiWindowHeight;
    m_pWindow = EZ_DEFAULT_NEW(ezFontRenderingWindow);
    m_pWindow->Initialize(WindowCreationDesc);
  }

  // Create a device
  {
    ezGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bCreatePrimarySwapChain = true;
    DeviceInit.m_bDebugDevice = true;
    DeviceInit.m_PrimarySwapChainDescription.m_pWindow = m_pWindow;
    DeviceInit.m_PrimarySwapChainDescription.m_SampleCount = ezGALMSAASampleCount::None;
    DeviceInit.m_PrimarySwapChainDescription.m_bAllowScreenshots = true;

    m_pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11, DeviceInit);
    EZ_VERIFY(m_pDevice->Init() == EZ_SUCCESS, "Device init failed!");

    ezGALDevice::SetDefaultDevice(m_pDevice);
  }

  // now that we have a window and device, tell the engine to initialize the rendering infrastructure
  ezStartup::StartupHighLevelSystems();


  // Get the primary swapchain (this one will always be created by device init except if the user instructs no swap chain creation
  // explicitly)
  {
    ezGALSwapChainHandle hPrimarySwapChain = m_pDevice->GetPrimarySwapChain();
    const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(hPrimarySwapChain);

    ezGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = g_uiWindowWidth;
    texDesc.m_uiHeight = g_uiWindowHeight;
    texDesc.m_Format = ezGALResourceFormat::D24S8;
    texDesc.m_bCreateRenderTarget = true;

    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);

    m_hBBRTV = m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture());
    m_hBBDSV = m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture);
  }

  // Create Rasterizer State
  {
    ezGALRasterizerStateCreationDescription RasterStateDesc;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bFrontCounterClockwise = true;
    m_hRasterizerState = m_pDevice->CreateRasterizerState(RasterStateDesc);
    EZ_ASSERT_DEV(!m_hRasterizerState.IsInvalidated(), "Couldn't create rasterizer state!");
  }

  // Create Depth Stencil state
  {
    ezGALDepthStencilStateCreationDescription DepthStencilStateDesc;
    DepthStencilStateDesc.m_bDepthTest = false;
    DepthStencilStateDesc.m_bDepthWrite = false;
    m_hDepthStencilState = m_pDevice->CreateDepthStencilState(DepthStencilStateDesc);
    EZ_ASSERT_DEV(!m_hDepthStencilState.IsInvalidated(), "Couldn't create depth-stencil state!");
  }

  // Setup Shaders and Materials
  {
    ezShaderManager::Configure("DX11_SM50", true);

    m_hFontShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Font.ezShader");
  }

  {
    m_hSampleConstants = ezRenderContext::CreateConstantBufferStorage(m_pSampleConstantBuffer);
  }

  m_Font = ezResourceManager::LoadResource<ezFontResource>(":/Fonts/Roboto-Black.ezFont");

  m_TextSpriteDesc.Font = m_Font;

  // Create the vertex buffer
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(Vertex);
    desc.m_uiTotalSize = VertexBufferSize * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::VertexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hVertexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  // Create the index buffer
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezUInt32);
    desc.m_uiTotalSize = IndexBufferSize * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::IndexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hIndexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  {
    ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic = ezGALVertexAttributeSemantic::Position;
    si.m_Format = ezGALResourceFormat::XYZFloat;
    si.m_uiOffset = 0;
    si.m_uiElementSize = 12;
  }

  {
    ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
    si.m_Semantic = ezGALVertexAttributeSemantic::TexCoord0;
    si.m_Format = ezGALResourceFormat::UVFloat;
    si.m_uiOffset = 12;
    si.m_uiElementSize = 8;
  }
}

void ezFontRenderingApp::BeforeCoreSystemsShutdown()
{
  // make sure that no textures are continue to be streamed in while the engine shuts down
  ezResourceManager::EngineAboutToShutdown();

  ezRenderContext::DeleteConstantBufferStorage(m_hSampleConstants);
  m_hSampleConstants.Invalidate();

  if (!m_hVertexBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexBuffer);
    m_hVertexBuffer.Invalidate();
  }

  if (!m_hIndexBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);
    m_hIndexBuffer.Invalidate();
  }

  m_hFontShader.Invalidate();
  m_Font.Invalidate();

  m_pDevice->DestroyTexture(m_hDepthStencilTexture);
  m_hDepthStencilTexture.Invalidate();

  // tell the engine that we are about to destroy window and graphics device,
  // and that it therefore needs to cleanup anything that depends on that
  ezStartup::ShutdownHighLevelSystems();

  ezResourceManager::FreeAllUnusedResources();

  m_pDevice->DestroyRasterizerState(m_hRasterizerState);
  m_pDevice->DestroyDepthStencilState(m_hDepthStencilState);

  // now we can destroy the graphics device
  m_pDevice->Shutdown();

  EZ_DEFAULT_DELETE(m_pDevice);

  // finally destroy the window
  m_pWindow->Destroy();
  EZ_DEFAULT_DELETE(m_pWindow);
}

void ezFontRenderingApp::RenderText()
{
  ezRenderContext::GetDefaultInstance()->BindConstantBuffer("ezTextureSampleConstants", m_hSampleConstants);

  ezMat4 Proj = ezGraphicsUtils::CreateOrthographicProjectionMatrix(m_vCameraPosition.x + -(float)g_uiWindowWidth * 0.5f,
    m_vCameraPosition.x + (float)g_uiWindowWidth * 0.5f, m_vCameraPosition.y + -(float)g_uiWindowHeight * 0.5f,
    m_vCameraPosition.y + (float)g_uiWindowHeight * 0.5f, -1.0f, 1.0f);

  ezMat4 mTransform;
  mTransform.SetIdentity();

  ezTextSprite textSprite;
  textSprite.Update(m_TextSpriteDesc);
  ezUInt32 numRenderElements = textSprite.GetNumRenderElements();

  ezRenderContext::GetDefaultInstance()->BindShader(m_hFontShader);

  for (ezUInt32 i = 0; i < numRenderElements; i++)
  {
    const ezTextSpriteRenderElementData& renderData = textSprite.GetRenderElementData(i);

    ezArrayPtr<Vertex> vertices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), Vertex, renderData.m_Vertices.GetCount());
    ezArrayPtr<ezUInt32> indices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezUInt32, renderData.m_Indices.GetCount());

    for (ezUInt32 vertexIndex = 0; vertexIndex < renderData.m_Vertices.GetCount(); vertexIndex++)
    {
      auto& vertex = renderData.m_Vertices[vertexIndex];
      auto& uv = renderData.m_UVs[vertexIndex];

      vertices[vertexIndex] = {{vertex.x, vertex.y, 1.0f}, {uv.x, uv.y}};
    };

    indices.CopyFrom(renderData.m_Indices.GetArrayPtr());

    ezFontSampleConstants& cb = m_pSampleConstantBuffer->GetDataForWriting();

    cb.ModelMatrix = mTransform;
    cb.ViewProjectionMatrix = Proj;

    ezRenderContext::GetDefaultInstance()->BindTexture2D("FontAtlasTexture", renderData.m_hTexture);

    ezRenderContext::GetDefaultInstance()->GetGALContext()->UpdateBuffer(m_hVertexBuffer, 0, ezMakeArrayPtr(vertices.GetPtr(), vertices.GetCount()).ToByteArray());
    ezRenderContext::GetDefaultInstance()->GetGALContext()->UpdateBuffer(m_hIndexBuffer, 0, ezMakeArrayPtr(indices.GetPtr(), indices.GetCount()).ToByteArray());

    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hVertexBuffer, m_hIndexBuffer, &m_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, indices.GetCount() / 3);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer();
  }
}

EZ_CONSOLEAPP_ENTRY_POINT(ezFontRenderingApp);
