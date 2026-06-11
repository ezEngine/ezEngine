#include <RendererTest/RendererTestPCH.h>

#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererTest/Basics/IndirectDraw.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>

#include "../../../../Data/UnitTests/RendererTest/Shaders/IndirectArgs.h"

static ezRendererTestIndirectDraw s_IndirectDrawTest;

void ezRendererTestIndirectDraw::SetupSubTests()
{
  AddSubTest("DrawInstancedIndirect", SubTests::ST_DrawInstancedIndirect);
  AddSubTest("DrawIndexedInstancedIndirect", SubTests::ST_DrawIndexedInstancedIndirect);
  AddSubTest("DrawIndexedInstancedIndirect Offset", SubTests::ST_DrawIndexedInstancedIndirectOffset);
  AddSubTest("DispatchIndirect", SubTests::ST_DispatchIndirect);
}

ezResult ezRendererTestIndirectDraw::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(CreateWindow(s_uiRTSize, s_uiRTSize));

  ezGPUResourcePool* pResourcePool = EZ_DEFAULT_NEW(ezGPUResourcePool);
  ezGPUResourcePool::SetDefaultInstance(pResourcePool);

  // Indirect args buffer: 32 bytes is enough for all argument structs (max 5 uint32 = 20 bytes) plus offset tests.
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = 4;
    desc.m_uiTotalSize = 64;
    desc.m_BufferFlags = ezGALBufferUsageFlags::ByteAddressBuffer | ezGALBufferUsageFlags::UnorderedAccess | ezGALBufferUsageFlags::DrawIndirect;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hIndirectArgsBuffer = m_pDevice->CreateBuffer(desc);
    EZ_ASSERT_DEV(!m_hIndirectArgsBuffer.IsInvalidated(), "Failed to create indirect args buffer");
  }

  // Non-indexed triangle mesh: a full-NDC triangle (covers the entire viewport).
  {
    ezGeometry geom;
    geom.AddRect(ezVec2(2.0f, 2.0f), 1, 1);
    m_hTriangleMesh = CreateMesh(geom, "IndirectDrawTriangle");
  }

  // Indexed triangle mesh: same geometry but through the indexed path (CreateMesh already creates index buffers).
  m_hIndexedTriangleMesh = m_hTriangleMesh;

  // Load shaders.
  m_hFillArgsShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/FillIndirectArgs.ezShader");
  m_hDrawShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/StencilColor.ezShader");
  m_hInstancedDrawShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/IndirectDrawInstances.ezShader");
  m_hIndexedInstancedDrawShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/IndirectDrawIndexedInstances.ezShader");
  m_hDispatchWriteShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/IndirectDispatchWrite.ezShader");

  m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);

  // Dispatch output texture (for DispatchIndirect test).
  if (iIdentifier == ST_DispatchIndirect)
  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 16;
    desc.m_uiHeight = 16;
    desc.m_Format = ezGALResourceFormat::RGBAFloat;
    desc.m_TextureFlags = ezGALTextureUsageFlags::UnorderedAccess | ezGALTextureUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hDispatchOutputTexture = m_pDevice->CreateTexture(desc);
    EZ_ASSERT_DEV(!m_hDispatchOutputTexture.IsInvalidated(), "Failed to create dispatch output texture");

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestIndirectDraw::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_hFillArgsShader.Invalidate();
  m_hDrawShader.Invalidate();
  m_hInstancedDrawShader.Invalidate();
  m_hIndexedInstancedDrawShader.Invalidate();
  m_hDispatchWriteShader.Invalidate();
  m_hTriangleMesh.Invalidate();
  m_hIndexedTriangleMesh.Invalidate();

  m_pDevice->DestroyBuffer(m_hIndirectArgsBuffer);
  m_pDevice->DestroyTexture(m_hDispatchOutputTexture);

  ezGPUResourcePool::SetDefaultInstance(nullptr);
  DestroyWindow();
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::DeInitializeSubTest(iIdentifier));
  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestIndirectDraw::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;

  BeginFrame();

  switch (iIdentifier)
  {
    case ST_DrawInstancedIndirect:
      DrawInstancedIndirect();
      break;
    case ST_DrawIndexedInstancedIndirect:
      DrawIndexedInstancedIndirect();
      break;
    case ST_DrawIndexedInstancedIndirectOffset:
      DrawIndexedInstancedIndirectOffset();
      break;
    case ST_DispatchIndirect:
      DispatchIndirect();
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  EndFrame();
  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return ezTestAppRun::Quit;
  }
  return ezTestAppRun::Continue;
}

void ezRendererTestIndirectDraw::DrawInstancedIndirect()
{
  ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();

  BeginCommands("DrawInstancedIndirect");

  // Compute pass: fill indirect args with {vertexCount=6, instanceCount=4, startVertex=0, startInstance=0}
  FillIndirectArgsViaCompute(6, 4, 0, 0);

  TransitionBuffer(m_hIndirectArgsBuffer, ezGALResourceState::DrawIndirect);
  TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);

  {
    BeginRendering(ezColor::Black);

    pContext->BindShader(m_hInstancedDrawShader);
    pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
    pContext->ApplyContextStates().AssertSuccess();
    m_pEncoder->DrawInstancedIndirect(m_hIndirectArgsBuffer, 0).AssertSuccess();

    EndRendering();
  }

  CaptureImage();
  EndCommands();
}

void ezRendererTestIndirectDraw::DrawIndexedInstancedIndirect()
{
  ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();

  BeginCommands("DrawIndexedInstancedIndirect");

  // Compute pass: fill indexed indirect args {indexCount=6, instanceCount=4, startIndex=0, baseVertex=0, startInstance=0}
  FillIndirectArgsViaCompute(6, 4, 0, 0, 0);

  TransitionBuffer(m_hIndirectArgsBuffer, ezGALResourceState::DrawIndirect);
  TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);

  {
    BeginRendering(ezColor::Black);

    pContext->BindShader(m_hIndexedInstancedDrawShader);
    pContext->BindMeshBuffer(m_hIndexedTriangleMesh);
    pContext->ApplyContextStates().AssertSuccess();
    m_pEncoder->DrawIndexedInstancedIndirect(m_hIndirectArgsBuffer, 0).AssertSuccess();

    EndRendering();
  }

  CaptureImage();
  EndCommands();
}

void ezRendererTestIndirectDraw::DrawIndexedInstancedIndirectOffset()
{
  ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
  ezMat4 mMVP = ezMat4::MakeIdentity();
  if (ezClipSpaceYMode::RenderToTextureDefault == ezClipSpaceYMode::Flipped)
  {
    mMVP = ezMat4::MakeScaling(ezVec3(1.0f, -1.0f, 1.0f)) * mMVP;
  }

  BeginCommands("DrawIndexedIndirectOffset");

  // Zero arguments at offset 0 must leave the cleared target untouched.
  FillIndirectArgsViaCompute(0, 0, 0, 0, 0);

  TransitionBuffer(m_hIndirectArgsBuffer, ezGALResourceState::DrawIndirect);
  TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);

  {
    const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
    ezRectFloat viewport = ezRectFloat(0, 0, fWidth * 0.5f, fHeight);

    BeginRendering(ezColor::Black, 0xFFFFFFFF, &viewport);

    ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mMVP;
    ocb->m_Color = ezColor(1.0f, 0.0f, 1.0f, 1.0f);
    pContext->GetBindGroup().BindBuffer("PerObject", m_hObjectTransformCB);
    pContext->BindShader(m_hDrawShader);
    pContext->BindMeshBuffer(m_hIndexedTriangleMesh);
    pContext->ApplyContextStates().AssertSuccess();
    m_pEncoder->DrawIndexedInstancedIndirect(m_hIndirectArgsBuffer, 0).AssertSuccess();

    EndRendering();
  }

  EndCommands();

  BeginCommands("DrawIndexedIndirectOffset");

  // Fill valid args at byte offset 20 using UpdateBuffer from CPU.
  // DrawIndexedInstanced args: {indexCount=6, instanceCount=1, startIndex=0, baseVertex=0, startInstance=0}
  {
    ezUInt32 args[5] = {6, 1, 0, 0, 0};
    m_pEncoder->UpdateBuffer(m_hIndirectArgsBuffer, 20, ezMakeArrayPtr(reinterpret_cast<const ezUInt8*>(args), sizeof(args)), ezGALUpdateMode::AheadOfTime);
  }

  TransitionBuffer(m_hIndirectArgsBuffer, ezGALResourceState::DrawIndirect);
  TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);

  {
    const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
    ezRectFloat viewport = ezRectFloat(fWidth * 0.5f, 0, fWidth * 0.5f, fHeight);

    BeginRendering(ezColor::Black, 0, &viewport);

    ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mMVP;
    ocb->m_Color = ezColor(1.0f, 1.0f, 0.0f, 1.0f);
    pContext->GetBindGroup().BindBuffer("PerObject", m_hObjectTransformCB);
    pContext->BindShader(m_hDrawShader);
    pContext->BindMeshBuffer(m_hIndexedTriangleMesh);
    pContext->ApplyContextStates().AssertSuccess();
    // Use offset 20 into the buffer.
    m_pEncoder->DrawIndexedInstancedIndirect(m_hIndirectArgsBuffer, 20).AssertSuccess();

    EndRendering();
  }

  CaptureImage();
  EndCommands();
}

void ezRendererTestIndirectDraw::DispatchIndirect()
{
  ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();

  BeginCommands("DispatchIndirect");

  // Fill dispatch args: {threadGroupCountX=2, threadGroupCountY=2, threadGroupCountZ=1}
  // The compute shader uses [numthreads(8,8,1)], so 2*2 groups = 16x16 threads = fills the 16x16 texture.
  FillIndirectArgsViaCompute(2, 2, 1, 0);

  TransitionBuffer(m_hIndirectArgsBuffer, ezGALResourceState::DrawIndirect);
  TransitionTexture(m_hDispatchOutputTexture, ezGALResourceState::UnorderedAccess);

  // Dispatch pass.
  {
    pContext->BeginCompute("IndirectDispatch");
    pContext->BindShader(m_hDispatchWriteShader);

    ezBindGroupBuilder& bg = pContext->GetBindGroup();
    bg.BindTexture("OutputTexture", m_hDispatchOutputTexture);
    pContext->ApplyContextStates().AssertSuccess();
    m_pEncoder->DispatchIndirect(m_hIndirectArgsBuffer, 0).AssertSuccess();

    pContext->EndCompute();
  }

  TransitionTexture(m_hDispatchOutputTexture, ezGALResourceState::ShaderResource);

  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  ezRectFloat viewport = ezRectFloat(0, 0, fWidth, fHeight);
  const ezMat4 mTextureMVP = CreateSimpleMVP(fWidth / fHeight);
  m_bCaptureImage = m_ImgCompFrames.Contains(m_iFrame);
  RenderCube(viewport, mTextureMVP, 0xFFFFFFFF, m_hDispatchOutputTexture);
  EndCommands();
}

// ============================================================
// Helper: Fill indirect args via a compute dispatch
// ============================================================

void ezRendererTestIndirectDraw::FillIndirectArgsViaCompute(ezUInt32 arg0, ezUInt32 arg1, ezUInt32 arg2, ezUInt32 arg3, ezUInt32 arg4)
{
  ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();

  TransitionBuffer(m_hIndirectArgsBuffer, ezGALResourceState::UnorderedAccess);

  pContext->BeginCompute("FillIndirectArgs");
  pContext->BindShader(m_hFillArgsShader);

  ezIndirectArgs constants;
  constants.Arg0 = arg0;
  constants.Arg1 = arg1;
  constants.Arg2 = arg2;
  constants.Arg3 = arg3;
  constants.Arg4 = arg4;
  pContext->SetPushConstants("ezIndirectArgs", constants);

  ezBindGroupBuilder& bg = pContext->GetBindGroup();
  bg.BindBuffer("IndirectArgsBuffer", m_hIndirectArgsBuffer);
  pContext->Dispatch(1).AssertSuccess();
  pContext->EndCompute();
}

void ezRendererTestIndirectDraw::CaptureImage()
{
  if (m_ImgCompFrames.Contains(m_iFrame))
  {
    TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
    EZ_TEST_IMAGE(m_iFrame, 100);
  }
}
