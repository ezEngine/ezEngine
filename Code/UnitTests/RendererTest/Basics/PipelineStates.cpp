#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <RendererTest/Basics/PipelineStates.h>

#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestConstants.h>
#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestInstancing.h>

ezResult ezRendererTestPipelineStates::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(SetupRenderer());
  EZ_SUCCEED_OR_RETURN(CreateWindow(320, 240));
  m_hMostBasicTriangleShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/MostBasicTriangle.ezShader");
  m_hNDCPositionOnlyShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/NDCPositionOnly.ezShader");
  m_hConstantBufferShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ConstantBuffer.ezShader");
  m_hInstancingShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Instancing.ezShader");

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
  m_hTestColorsConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezTestColors>();
  m_hTestPositionsConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezTestPositions>();
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezTestShaderData);
    desc.m_uiTotalSize = 8 * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hInstancingData = m_pDevice->CreateBuffer(desc);
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestPipelineStates::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_hTriangleMesh.Invalidate();
  m_hSphereMesh.Invalidate();

  m_hMostBasicTriangleShader.Invalidate();
  m_hNDCPositionOnlyShader.Invalidate();
  m_hConstantBufferShader.Invalidate();
  m_hInstancingShader.Invalidate();

  m_hTestColorsConstantBuffer.Invalidate();
  m_hTestPositionsConstantBuffer.Invalidate();

  if (!m_hInstancingData.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingData);
    m_hInstancingData.Invalidate();
  }

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
    case SubTests::ST_ConstantBuffer:
      ConstantBufferTest();
      break;
    case SubTests::ST_StructuredBuffer:
      StructuredBufferTest();
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
    ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(clearColor, uiRenderTargetClearMask, pViewport, pScissor);
    {

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
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

ezGALRenderCommandEncoder* ezRendererTestPipelineStates::BeginRendering(ezColor clearColor, ezUInt32 uiRenderTargetClearMask, ezRectFloat* pViewport, ezRectU32* pScissor)
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

  static ezHashedString sClipSpaceFlipped = ezMakeHashedString("CLIP_SPACE_FLIPPED");
  static ezHashedString sTrue = ezMakeHashedString("TRUE");
  static ezHashedString sFalse = ezMakeHashedString("FALSE");
  ezClipSpaceYMode::Enum clipSpace = ezClipSpaceYMode::RenderToTextureDefault;
  ezRenderContext::GetDefaultInstance()->SetShaderPermutationVariable(sClipSpaceFlipped, clipSpace == ezClipSpaceYMode::Flipped ? sTrue : sFalse);
  return pCommandEncoder;
}

void ezRendererTestPipelineStates::EndRendering()
{
  ezRenderContext::GetDefaultInstance()->EndRendering();
  m_pWindow->ProcessWindowMessages();
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


ezTransform CreateTransform(const ezUInt32 uiColumns, const ezUInt32 uiRows, ezUInt32 x, ezUInt32 y)
{
  ezTransform t = ezTransform::IdentityTransform();
  t.m_vScale = ezVec3(1.0f / float(uiColumns), 1.0f / float(uiRows), 1);
  t.m_vPosition = ezVec3(ezMath::Lerp(-1.f, 1.f, (float(x) + 0.5f) / float(uiColumns)), ezMath::Lerp(1.f, -1.f, (float(y) + 0.5f) / float(uiRows)), 0);
  if (ezClipSpaceYMode::RenderToTextureDefault == ezClipSpaceYMode::Flipped)
  {
    ezTransform flipY = ezTransform::IdentityTransform();
    flipY.m_vScale.y *= -1.0f;
    t = flipY * t;
  }
  return t;
}

void ezRendererTestPipelineStates::ConstantBufferTest()
{
  const ezUInt32 uiColumns = 4;
  const ezUInt32 uiRows = 2;

  m_pPass = m_pDevice->BeginPass("ConstantBufferTest");
  {
    ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(ezColor::CornflowerBlue, 0xFFFFFFFF);
    ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
    {
      pContext->BindConstantBuffer("ezTestColors", m_hTestColorsConstantBuffer);
      pContext->BindConstantBuffer("ezTestPositions", m_hTestPositionsConstantBuffer);
      pContext->BindShader(m_hConstantBufferShader);
      pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

      for (ezUInt32 x = 0; x < uiColumns; ++x)
      {
        for (ezUInt32 y = 0; y < uiRows; ++y)
        {
          {
            auto constants = ezRenderContext::GetConstantBufferData<ezTestColors>(m_hTestColorsConstantBuffer);
            constants->VertexColor = ezColor::GetPaletteColor(x * uiRows + y).GetAsVec4();
          }
          {
            ezTransform t = CreateTransform(uiColumns, uiRows, x, y);
            auto constants = ezRenderContext::GetConstantBufferData<ezTestPositions>(m_hTestPositionsConstantBuffer);
            constants->Vertex0 = (t * ezVec3(1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex1 = (t * ezVec3(-1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex2 = (t * ezVec3(-0.f, 1.f, 0.0f)).GetAsVec4(1.0f);
          }
          pContext->DrawMeshBuffer(1).AssertSuccess();
        }
      }
    }
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

EZ_DEFINE_AS_POD_TYPE(ezTestShaderData);

void ezRendererTestPipelineStates::StructuredBufferTest()
{
  const ezUInt32 uiColumns = 4;
  const ezUInt32 uiRows = 2;

  ezHybridArray<ezTestShaderData, 8> instanceData;
  instanceData.SetCountUninitialized(8);
  for (ezUInt32 x = 0; x < uiColumns; ++x)
  {
    for (ezUInt32 y = 0; y < uiRows; ++y)
    {
      ezTestShaderData& instance = instanceData[x * uiRows + y];
      instance.InstanceColor = ezColor::GetPaletteColor(x * uiRows + y).GetAsVec4();
      ezTransform t = CreateTransform(uiColumns, uiRows, x, y);
      instance.InstanceTransform = t;
    }
  }

  m_pPass = m_pDevice->BeginPass("InstancingTest");
  {
    ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(ezColor::CornflowerBlue, 0xFFFFFFFF);

    pCommandEncoder->UpdateBuffer(m_hInstancingData, 0, instanceData.GetArrayPtr().ToByteArray());

    ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
    {
      pContext->BindShader(m_hInstancingShader);
      pContext->BindMeshBuffer(m_hTriangleMesh);
      pContext->BindBuffer("instancingData", m_pDevice->GetDefaultResourceView(m_hInstancingData));

      pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
    }
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

static ezRendererTestPipelineStates g_PipelineStatesTest;
