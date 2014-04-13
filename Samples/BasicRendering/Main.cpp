#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/TaskSystem.h>

#include <Core/Basics.h>
#include <Core/Application/Application.h>

#include <System/Window/Window.h>

#define DEMO_GL EZ_ON
#define DEMO_DX11 EZ_OFF

#if EZ_ENABLED(DEMO_GL)
  #include <RendererGL/Device/DeviceGL.h>
#else
  #include <RendererDX11/Device/DeviceDX11.h>
#endif

#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Context/Context.h>

#include <Helper/MayaObj.h>
#include <Helper/Shader.h>
#include <Helper/Misc.h>

#include <CoreUtils/Debugging/DataTransfer.h>

// This sample is a really simple low-level rendering demo showing how the high level renderer will interact with the GPU abstraction layer

class TestWindow : public ezWindow
{
  public:

    TestWindow()
      : ezWindow()
    {
      m_bCloseRequested = false;
    }

    virtual void OnClickCloseMessage() EZ_OVERRIDE
    {
      m_bCloseRequested = true;
    }

    bool m_bCloseRequested;
};

struct TestCB
{
  ezMat4 mvp;
};

static bool g_bMSAA = false;
static ezUInt32 g_uiWindowWidth = 1280;
static ezUInt32 g_uiWindowHeight = 720;

class BasicRendering : public ezApplication
{
public:

  void AfterEngineInit() EZ_OVERRIDE
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::AddDataDirectory("");

    ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
    sReadDir.AppendPath("../../Shared/FreeContent/Basic Rendering/");

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::ReadOnly, "Basic Rendering Content");

    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);

    ezTelemetry::CreateServer();
    ezPlugin::LoadPlugin("ezInspectorPlugin");

    ezClock::SetNumGlobalClocks();

    // Create a window for rendering
    ezWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_ClientAreaSize.width = g_uiWindowWidth;
    WindowCreationDesc.m_ClientAreaSize.height = g_uiWindowHeight;
    m_pWindow = EZ_DEFAULT_NEW(TestWindow)();
    m_pWindow->Initialize(WindowCreationDesc);


    // Create a device
    ezGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bCreatePrimarySwapChain = true;
    DeviceInit.m_bDebugDevice = true;
    DeviceInit.m_PrimarySwapChainDescription.m_pWindow = m_pWindow;
    DeviceInit.m_PrimarySwapChainDescription.m_SampleCount = g_bMSAA ? ezGALMSAASampleCount::FourSamples : ezGALMSAASampleCount::None;
    DeviceInit.m_PrimarySwapChainDescription.m_bCreateDepthStencilBuffer = true;
    DeviceInit.m_PrimarySwapChainDescription.m_DepthStencilBufferFormat = ezGALResourceFormat::D24S8;
    DeviceInit.m_PrimarySwapChainDescription.m_bAllowScreenshots = true;
    DeviceInit.m_PrimarySwapChainDescription.m_bVerticalSynchronization = true;

#if EZ_ENABLED(DEMO_GL)
    m_pDevice = EZ_DEFAULT_NEW(ezGALDeviceGL)(DeviceInit);
#else
    m_pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11)(DeviceInit);
#endif
    EZ_VERIFY(m_pDevice->Init() == EZ_SUCCESS, "Device init failed!");

    // Get the primary swapchain (this one will always be created by device init except if the user instructs no swap chain creation explicitly)
    ezGALSwapChainHandle hPrimarySwapChain = m_pDevice->GetPrimarySwapChain();
    const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(hPrimarySwapChain);
    
    m_hBBRT = pPrimarySwapChain->GetRenderTargetViewConfig();
    

    // Create a constant buffer for matrix upload
    m_hCB = m_pDevice->CreateConstantBuffer(sizeof(TestCB));

    m_pObj = DontUse::MayaObj::LoadFromFile("ez.obj", m_pDevice);

    // Create a shader (uses a quick hacky implementation to compile the HLSL shaders)
    ezGALShaderCreationDescription ShaderDesc;
#if EZ_ENABLED(DEMO_GL)
    ezDynamicArray<ezUInt8> pixelShader, vertexShader;
    DontUse::ReadCompleteFile("ez_vert.glsl", vertexShader);
    vertexShader.PushBack('\0');
    DontUse::ReadCompleteFile("ez_frag.glsl", pixelShader);
    pixelShader.PushBack('\0');
    ShaderDesc.m_ByteCodes[ezGALShaderStage::VertexShader] = new ezGALShaderByteCode(&vertexShader[0], vertexShader.GetCount());
    ShaderDesc.m_ByteCodes[ezGALShaderStage::PixelShader] = new ezGALShaderByteCode(&pixelShader[0], pixelShader.GetCount());
#else
    DontUse::ShaderCompiler::Compile("ez.hlsl", ShaderDesc);
#endif

    m_hShader = m_pDevice->CreateShader(ShaderDesc);
    EZ_ASSERT(!m_hShader.IsInvalidated(), "Couldn't create shader!");

    // Now the vertex declaration needs to be built
    ezGALVertexDeclarationCreationDescription VertDeclDesc;
    VertDeclDesc.m_hShader = m_hShader;
    VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat, 0, 0, false));
    VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat, 12, 0, false));
    VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat, 24, 0, false));

    m_hVertexDeclaration = m_pDevice->CreateVertexDeclaration(VertDeclDesc);
    EZ_ASSERT(!m_hVertexDeclaration.IsInvalidated(), "Couldn't create input layout!");

    ezGALRasterizerStateCreationDescription RasterStateDesc;
    //RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    m_hRasterizerState = m_pDevice->CreateRasterizerState(RasterStateDesc);
    EZ_ASSERT(!m_hRasterizerState.IsInvalidated(), "Couldn't create rasterizer state!");

 
    ezGALDepthStencilStateCreationDescription DepthStencilStateDesc;
    DepthStencilStateDesc.m_bDepthTest = true;
    DepthStencilStateDesc.m_bDepthWrite = true;
    m_hDepthStencilState = m_pDevice->CreateDepthStencilState(DepthStencilStateDesc);
    EZ_ASSERT(!m_hDepthStencilState.IsInvalidated(), "Couldn't create depth-stencil state!");

    // Create texture (not used on the mesh)
    ezGALTextureCreationDescription TexDesc;
    TexDesc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    TexDesc.m_uiWidth = 16;
    TexDesc.m_uiHeight = 16;

    static ezUInt32 Pixels[] = {
      0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000,
      0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC,
      0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000,
      0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC,
      0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000,
      0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC,
      0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000,
      0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC,
      0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000,
      0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC,
      0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000,
      0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC,
      0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000,
      0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC,
      0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000,
      0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC, 0xFF000000, 0xFFCCCCCC,
    };

    ezGALSystemMemoryDescription InitData[1];
    InitData[0].m_pData = Pixels;
    InitData[0].m_uiRowPitch = TexDesc.m_uiWidth * sizeof(ezUInt32);
    InitData[0].m_uiSlicePitch = TexDesc.m_uiHeight * InitData[0].m_uiRowPitch;

    ezArrayPtr<ezGALSystemMemoryDescription> InitDataPtr(InitData);

    m_hTexture = m_pDevice->CreateTexture(TexDesc, &InitDataPtr);

    ezGALResourceViewCreationDescription TexViewDesc;
    TexViewDesc.m_hTexture = m_hTexture;
    
    m_hTexView = m_pDevice->CreateResourceView(TexViewDesc);

    ezGALSamplerStateCreationDescription SamplerDesc;
    SamplerDesc.m_MagFilter = SamplerDesc.m_MinFilter = SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;
    m_hSamplerState = m_pDevice->CreateSamplerState(SamplerDesc);

    m_DebugBackBufferDT.EnableDataTransfer("Back Buffer");
  }


  ApplicationExecution Run() EZ_OVERRIDE
  {
    m_pWindow->ProcessWindowMessages();

    if (m_pWindow->m_bCloseRequested)
      return ApplicationExecution::Quit;

    ezClock::UpdateAllGlobalClocks();

    ezTelemetry::PerFrameUpdate();

    ezTaskSystem::FinishFrameTasks();

 
    // Before starting to render in a frame call this function
    m_pDevice->BeginFrame();

    // The ezGALContext class is the main interaction point for draw / compute operations
    ezGALContext* pContext = m_pDevice->GetPrimaryContext();

    pContext->SetRenderTargetConfig(m_hBBRT);
    pContext->SetViewport(0.0f, 0.0f, (float)g_uiWindowWidth, (float)g_uiWindowHeight, 0.0f, 1.0f);
    pContext->Clear(ezColor::GetBlack());

    pContext->SetVertexBuffer(0, m_pObj->GetVB());
    pContext->SetIndexBuffer(m_pObj->GetIB());

    pContext->SetShader(m_hShader);

    pContext->SetVertexDeclaration(m_hVertexDeclaration);
    pContext->SetPrimitiveTopology(ezGALPrimitiveTopology::Triangles);
    pContext->SetRasterizerState(m_hRasterizerState);
    pContext->SetDepthStencilState(m_hDepthStencilState);
    pContext->SetResourceView(ezGALShaderStage::PixelShader, 0, m_hTexView);
    pContext->SetSamplerState(ezGALShaderStage::PixelShader, 0, m_hSamplerState);

    static float fRotY = 0.0f;
    fRotY -= 10.0f * ezClock::Get()->GetTimeDiff().AsFloat();

    ezMat4 ModelRot;
    ModelRot.SetRotationMatrixY(ezAngle::Degree(fRotY));

    ezMat4 Model;
    Model.SetIdentity();
    Model.SetScalingFactors(ezVec3(0.75f));
    
    
    ezMat4 View;
    View.SetIdentity();
    View.SetLookAtMatrix(ezVec3(0.5f, 1.5f, 2.0f), ezVec3(0.0f, 0.5f, 0.0f), ezVec3(0.0f, 1.0f, 0.0f));

    ezMat4 Proj;
    Proj.SetIdentity();
    Proj.SetPerspectiveProjectionMatrixFromFovY(ezAngle::Degree(80.0f), (float)g_uiWindowWidth / (float)g_uiWindowHeight, 0.1f, 1000.0f, 
#if EZ_ENABLED(DEMO_GL)
      ezProjectionDepthRange::MinusOneToOne
#else
      ezProjectionDepthRange::ZeroToOne
#endif
    );

    TestCB ObjectData;

    ObjectData.mvp = Proj * View * Model * ModelRot;

    pContext->UpdateBuffer(m_hCB, 0, &ObjectData, sizeof(TestCB));

    pContext->SetConstantBuffer(1, m_hCB);

    pContext->DrawIndexed(m_pObj->GetPrimitiveCount() * 3, 0);


    // Readback: Currently not supported for MSAA since Resolve() is not implemented
    if (m_DebugBackBufferDT.IsTransferRequested() && !g_bMSAA)
    {
      ezGALTextureHandle hBBTexture = m_pDevice->GetSwapChain(m_pDevice->GetPrimarySwapChain())->GetBackBufferTexture();
      pContext->ReadbackTexture(hBBTexture);

      ezArrayPtr<ezUInt8> pImageContent = EZ_DEFAULT_NEW_ARRAY(ezUInt8, 4 * g_uiWindowWidth * g_uiWindowHeight);

      ezGALSystemMemoryDescription MemDesc;
      MemDesc.m_pData = pImageContent.GetPtr();
      MemDesc.m_uiRowPitch = 4 * g_uiWindowWidth;
      MemDesc.m_uiSlicePitch = 4 * g_uiWindowWidth * g_uiWindowHeight;

      ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);

      pContext->CopyTextureReadbackResult(hBBTexture, &SysMemDescs);

      ezDataTransferObject DataObject(m_DebugBackBufferDT, "Back Buffer", "image/rgba8", "rgba");
      DataObject.GetWriter() << g_uiWindowWidth;
      DataObject.GetWriter() << g_uiWindowHeight;
      DataObject.GetWriter().WriteBytes(pImageContent.GetPtr(), 4 * g_uiWindowWidth * g_uiWindowHeight);
      m_DebugBackBufferDT.Transfer(DataObject);

      EZ_DEFAULT_DELETE_ARRAY(pImageContent);
    }


    m_pDevice->Present(m_pDevice->GetPrimarySwapChain());

    m_pDevice->EndFrame();

    return ezApplication::Continue;
  }

  void BeforeEngineShutdown() EZ_OVERRIDE
  {
    EZ_DEFAULT_DELETE(m_pObj);

    m_pDevice->Shutdown();

    EZ_DEFAULT_DELETE(m_pDevice);

    m_pWindow->Destroy();
    EZ_DEFAULT_DELETE(m_pWindow);
  }

private:

  TestWindow* m_pWindow;

  ezGALDevice* m_pDevice;

  ezGALRenderTargetConfigHandle m_hBBRT;

  ezGALBufferHandle m_hCB;

  ezGALShaderHandle m_hShader;

  ezGALVertexDeclarationHandle m_hVertexDeclaration;

  ezGALRasterizerStateHandle m_hRasterizerState;

  ezGALDepthStencilStateHandle m_hDepthStencilState;

  ezGALTextureHandle m_hTexture;

  ezGALResourceViewHandle m_hTexView;

  ezGALSamplerStateHandle m_hSamplerState;


  DontUse::MayaObj* m_pObj;

  ezDataTransfer m_DebugBackBufferDT;
};

EZ_CONSOLEAPP_ENTRY_POINT(BasicRendering);
