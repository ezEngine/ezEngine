#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <RendererTest/Basics/PipelineStates.h>

ezResult ezRendererTestPipelineStates::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(SetupRenderer());
  EZ_SUCCEED_OR_RETURN(CreateWindow(320, 240));
  m_hMostBasicTriangleShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/MostBasicTriangle.ezShader");
  m_hNDCPositionOnlyShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/NDCPositionOnly.ezShader");
  {
    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreams(3);

    if (ezClipSpaceYMode::RenderToTextureDefault == ezClipSpaceYMode::Flipped)
    {
      desc.SetVertexData<ezVec3>(0, 0, ezVec3(1.f, 1.f, 0.0f));
      desc.SetVertexData<ezVec3>(0, 1, ezVec3(-1.f, 1.f, 0.0f));
      desc.SetVertexData<ezVec3>(0, 2, ezVec3(0.f, -1.f, 0.0f));
    }
    else
    {
      desc.SetVertexData<ezVec3>(0, 0, ezVec3(1.f, -1.f, 0.0f));
      desc.SetVertexData<ezVec3>(0, 1, ezVec3(-1.f, -1.f, 0.0f));
      desc.SetVertexData<ezVec3>(0, 2, ezVec3(0.f, 1.f, 0.0f));
    }

    m_hTriangleMesh = ezResourceManager::CreateResource<ezMeshBufferResource>("UnitTest-TriangleMesh", std::move(desc), "TriangleMesh");
  }
  {
    ezGeometry geom;
    geom.AddSphere(0.5f, 16, 16);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    m_hSphereMesh = ezResourceManager::CreateResource<ezMeshBufferResource>("UnitTest-SphereMesh", std::move(desc), "SphereMesh");
  }
  return EZ_SUCCESS;
}

ezResult ezRendererTestPipelineStates::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_hTriangleMesh.Invalidate();
  m_hSphereMesh.Invalidate();
  m_hMostBasicTriangleShader.Invalidate();
  m_hNDCPositionOnlyShader.Invalidate();

  DestroyWindow();
  ShutdownRenderer();
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::DeInitializeSubTest(iIdentifier));
  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestPipelineStates::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_pDevice->BeginFrame();
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);

  switch (iIdentifier)
  {
    case SubTests::ST_MostBasicShader:
      MostBasicTriangleTest();
      break;
    case SubTests::ST_ViewportScissor:
      ViewportScissorTest();
      break;
    case SubTests::ST_VertexBuffer:
      VertexBufferTest();
      break;
    case SubTests::ST_IndexBuffer:
      IndexBufferTest();
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  ezRenderContext::GetDefaultInstance()->ResetContextState();
  m_pDevice->EndPipeline(m_hSwapChain);
  m_pDevice->EndFrame();

  ezTaskSystem::FinishFrameTasks();

  return uiInvocationCount < 120 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

void ezRendererTestPipelineStates::RenderBlock(ezMeshBufferResourceHandle mesh, ezColor clearColor, ezUInt32 uiRenderTargetClearMask, ezRectFloat* pViewport, ezRectU32* pScissor)
{
  m_pPass = m_pDevice->BeginPass("MostBasicTriangle");
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
    {
      static ezHashedString sClipSpaceFlipped = ezMakeHashedString("CLIP_SPACE_FLIPPED");
      static ezHashedString sTrue = ezMakeHashedString("TRUE");
      static ezHashedString sFalse = ezMakeHashedString("FALSE");
      ezClipSpaceYMode::Enum bla = ezClipSpaceYMode::RenderToTextureDefault;
      ezRenderContext::GetDefaultInstance()->SetShaderPermutationVariable(sClipSpaceFlipped, bla == ezClipSpaceYMode::Flipped ? sTrue : sFalse);

      ezRectU32 scissor = ezRectU32(0, 0, m_pWindow->GetClientAreaSize().width, m_pWindow->GetClientAreaSize().height);
      if (pScissor)
      {
        scissor = *pScissor;
      }
      pCommandEncoder->SetScissorRect(scissor);
      if (mesh.IsValid())
      {
        ezRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
        ezRenderContext::GetDefaultInstance()->BindMeshBuffer(mesh);
        ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();
      }
      else
      {
        ezRenderContext::GetDefaultInstance()->BindShader(m_hMostBasicTriangleShader);
        ezRenderContext::GetDefaultInstance()->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
        ezRenderContext::GetDefaultInstance()->DrawMeshBuffer(1).AssertSuccess();
      }
    }
    ezRenderContext::GetDefaultInstance()->EndRendering();
    m_pWindow->ProcessWindowMessages();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void ezRendererTestPipelineStates::MostBasicTriangleTest()
{
  RenderBlock({}, ezColor::RebeccaPurple);
}

void ezRendererTestPipelineStates::ViewportScissorTest()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
  RenderBlock({}, ezColor::CornflowerBlue, 0xFFFFFFFF, &viewport);

  viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
  RenderBlock({}, ezColor::Green, 0, &viewport);

  viewport = ezRectFloat(0, 0, fElementWidth, fHeight);
  ezRectU32 scissor = ezRectU32(0, (ezUInt32)fElementHeight, (ezUInt32)fElementWidth, (ezUInt32)fElementHeight);
  RenderBlock({}, ezColor::Green, 0, &viewport, &scissor);

  viewport = ezRectFloat(0, 0, fWidth, fHeight);
  scissor = ezRectU32((ezUInt32)fElementWidth, 0, (ezUInt32)fElementWidth, (ezUInt32)fElementHeight);
  RenderBlock({}, ezColor::Green, 0, &viewport, &scissor);
}

void ezRendererTestPipelineStates::VertexBufferTest()
{
  RenderBlock(m_hTriangleMesh, ezColor::RebeccaPurple);
}

void ezRendererTestPipelineStates::IndexBufferTest()
{
  RenderBlock(m_hSphereMesh, ezColor::Orange);
}

static ezRendererTestPipelineStates g_PipelineStatesTest;
