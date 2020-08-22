#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <FontSample.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>
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
#include <Foundation/Utilities/GraphicsUtils.h>

static ezUInt32 g_uiWindowWidth = 640;
static ezUInt32 g_uiWindowHeight = 480;

ezFontRenderingApp::ezFontRenderingApp()
  : ezApplication("Font Rendering")
{
  m_vCameraPosition.SetZero();
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

  // do the rendering
  {
    // Before starting to render in a frame call this function
    m_pDevice->BeginFrame();

    // The ezGALContext class is the main interaction point for draw / compute operations
    ezGALContext* pContext = m_pDevice->GetPrimaryContext();

    auto& gc = ezRenderContext::GetDefaultInstance()->WriteGlobalConstants();
    ezMemoryUtils::ZeroFill(&gc, 1);

    gc.WorldToCameraMatrix[0] = m_camera->GetViewMatrix(ezCameraEye::Left);
    gc.WorldToCameraMatrix[1] = m_camera->GetViewMatrix(ezCameraEye::Right);
    gc.CameraToWorldMatrix[0] = gc.WorldToCameraMatrix[0].GetInverse();
    gc.CameraToWorldMatrix[1] = gc.WorldToCameraMatrix[1].GetInverse();
    gc.ViewportSize = ezVec4((float)g_uiWindowWidth, (float)g_uiWindowHeight, 1.0f / (float)g_uiWindowWidth, 1.0f / (float)g_uiWindowHeight);
    // Wrap around to prevent floating point issues. Wrap around is dividable by all whole numbers up to 11.
    gc.GlobalTime = (float)ezMath::Mod(ezClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 20790.0);
    gc.WorldTime = gc.GlobalTime;


    ezGALRenderTargetSetup RTS;
    RTS.SetRenderTarget(0, m_hBBRTV).SetDepthStencilTarget(m_hBBDSV);

    pContext->SetRenderTargetSetup(RTS);
    pContext->SetViewport(ezRectFloat(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight), 0.0f, 1.0f);
    pContext->Clear(ezColor::Blue);

    ezRenderContext::GetDefaultInstance()->BindConstantBuffer("ezTextureSampleConstants", m_hSampleConstants);

    RenderText();

    m_pDevice->Present(m_pDevice->GetPrimarySwapChain(), true);

    m_pDevice->EndFrame();
    ezRenderContext::GetDefaultInstance()->ResetContextState();
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
  m_camera = EZ_DEFAULT_NEW(ezCamera);
  m_camera->LookAt(ezVec3(3, 3, 1.5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
  m_directoryWatcher = EZ_DEFAULT_NEW(ezDirectoryWatcher);

  ezStringBuilder sProjectDir = ">sdk/Data/Samples/FontSample";
  ezStringBuilder sProjectDirResolved;
  ezFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved);

  ezFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

  EZ_VERIFY(
    m_directoryWatcher->OpenDirectory(sProjectDirResolved, ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded(),
    "Failed to watch project directory");

  ezFileSystem::AddDataDirectory("", "", ":", ezFileSystem::AllowWrites);
  ezFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", ezFileSystem::AllowWrites);                                     // writing to the binary directory
  ezFileSystem::AddDataDirectory(">appdir/", "ShaderCache", "shadercache", ezFileSystem::AllowWrites);                        // for shader files
  ezFileSystem::AddDataDirectory(">user/ezEngine Project/FontRenderingApp", "AppData", "appdata", ezFileSystem::AllowWrites); // app user data

  ezFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base");
  ezFileSystem::AddDataDirectory(">project/", "Project", "project", ezFileSystem::AllowWrites);

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

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
    WindowCreationDesc.m_Title = "Font Rendering";
    WindowCreationDesc.m_bShowMouseCursor = true;
    WindowCreationDesc.m_bClipMouseCursor = false;
    WindowCreationDesc.m_WindowMode = ezWindowMode::WindowResizable;
    m_pWindow = EZ_DEFAULT_NEW(ezFontRenderingWindow);
    m_pWindow->Initialize(WindowCreationDesc);
  }

  // Create a device
  {
    ezGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bCreatePrimarySwapChain = true;
    DeviceInit.m_bDebugDevice = false; // On Windows 10 this makes device creation fail :-(
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

  // Setup Shaders and Materials
  {
    ezShaderManager::Configure("DX11_SM40", true);

    m_hFontShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Font.ezShader");
  }

  {
    m_hSampleConstants = ezRenderContext::CreateConstantBufferStorage(m_pSampleConstantBuffer);
  }

  m_Font = ezResourceManager::LoadResource<ezFontResource>(":/Fonts/Roboto-Black.ezFont");

  m_TextSpriteDesc.Anchor = ezTextSpriteAnchor::TopLeft;
  m_TextSpriteDesc.HorizontalAlignment = ezTextHorizontalAlignment::Left;
  m_TextSpriteDesc.VerticalAlignment = ezTextVerticalAlignment::Center;
  m_TextSpriteDesc.Font = m_Font;
  m_TextSpriteDesc.FontSize = 30;
  m_TextSpriteDesc.Width = 100;
  m_TextSpriteDesc.Height = 100;
  m_TextSpriteDesc.Color = ezColor::Red;
  m_TextSpriteDesc.Text = "Hello World";
  m_TextSpriteDesc.WrapText = true;
  m_TextSpriteDesc.BreakTextWhenWrapped = true;
}

void ezFontRenderingApp::BeforeHighLevelSystemsShutdown()
{

  ezRenderContext::DeleteConstantBufferStorage(m_hSampleConstants);
  m_hSampleConstants.Invalidate();

  m_directoryWatcher->CloseDirectory();

  m_pDevice->DestroyTexture(m_hDepthStencilTexture);
  m_hDepthStencilTexture.Invalidate();

  m_hFontShader.Invalidate();
  m_hTextMeshBuffer.Invalidate();

  // tell the engine that we are about to destroy window and graphics device,
  // and that it therefore needs to cleanup anything that depends on that
  ezStartup::ShutdownHighLevelSystems();

  // now we can destroy the graphics device
  m_pDevice->Shutdown();

  EZ_DEFAULT_DELETE(m_pDevice);

  // finally destroy the window
  m_pWindow->Destroy();
  EZ_DEFAULT_DELETE(m_pWindow);

  m_camera.Clear();
  m_directoryWatcher.Clear();
}

void ezFontRenderingApp::RenderText()
{
  ezTextSprite textSprite;
  textSprite.Update(m_TextSpriteDesc);
  ezUInt32 numRenderElements = textSprite.GetNumRenderElements();

  struct Vertex
  {
    ezVec2 Position;
    ezVec2 TexCoord0;
  };

  ezDynamicArray<ezVec2> vertices;
  ezDynamicArray<ezVec2> uvs;

  ezRenderContext::GetDefaultInstance()->BindShader(m_hFontShader);

  for (ezUInt32 i = 0; i < numRenderElements; i++)
  {
    const ezTextSpriteRenderElementData& renderData = textSprite.GetRenderElementData(i);

    vertices.SetCount(renderData.m_NumQuads * 4);
    uvs.SetCount(renderData.m_NumQuads * 4);
    ezGeometry geom;

    for (ezUInt32 vertexIndex = 0; vertexIndex < renderData.m_Vertices.GetCount(); vertexIndex++)
    {
      auto& vertex = renderData.m_Vertices[vertexIndex];
      auto& uv = renderData.m_UVs[vertexIndex];

      vertices[vertexIndex] = {vertex.x, vertex.y};
      uvs[vertexIndex] = {uv.x, uv.y};
      geom.AddRectXY(ezVec2(vertex.x, vertex.y), ezColor::Red);
    };

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);

    desc.AllocateStreams(vertices.GetCount(), ezGALPrimitiveTopology::Triangles, vertices.GetCount() * 2);
    for (ezUInt32 v = 0; v < vertices.GetCount(); ++v)
    {
      desc.SetVertexData<ezVec3>(0, v, ezVec3(vertices[v].x, vertices[v].y, 1.0f));
      desc.SetVertexData<ezVec2>(1, v, ezVec2(uvs[v].x, uvs[v].y));
    }

    //ezUInt32 t = 0;
    //for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    //{
    //  for (ezUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
    //  {
    //    desc.SetTriangleIndices(
    //      t, geom.GetPolygons()[p].m_Vertices[0], geom.GetPolygons()[p].m_Vertices[v + 1], geom.GetPolygons()[p].m_Vertices[v + 2]);

    //    ++t;
    //  }
    //}

    ezMat4 Proj = ezGraphicsUtils::CreateOrthographicProjectionMatrix(m_vCameraPosition.x + -(float)g_uiWindowWidth * 0.5f,
      m_vCameraPosition.x + (float)g_uiWindowWidth * 0.5f, m_vCameraPosition.y + -(float)g_uiWindowHeight * 0.5f,
      m_vCameraPosition.y + (float)g_uiWindowHeight * 0.5f, -1.0f, 1.0f);

    ezFontSampleConstants& cb = m_pSampleConstantBuffer->GetDataForWriting();

    cb.ModelMatrix = ezMat4::IdentityMatrix();
    cb.ViewProjectionMatrix = Proj;


    ezStringBuilder meshId("{E692442B-9E15-46C5-8A00-1B07C02BF8F8}_");
    meshId.Append(i);

    ezMeshBufferResourceHandle mesh = ezResourceManager::GetExistingResource<ezMeshBufferResource>(meshId);
    if (!mesh.IsValid())
      mesh = ezResourceManager::CreateResource<ezMeshBufferResource>(meshId, std::move(desc));

    ezRenderContext::GetDefaultInstance()->BindTexture2D("FontAtlasTexture", renderData.m_hTexture);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(mesh);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer();
  }
}

EZ_CONSOLEAPP_ENTRY_POINT(ezFontRenderingApp);
