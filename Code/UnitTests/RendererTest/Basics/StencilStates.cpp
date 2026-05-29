#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <RendererTest/Basics/StencilStates.h>

static ezRendererTestStencilStates g_StencilStatesTest;

ezResult ezRendererTestStencilStates::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;

  if (ezGraphicsTest::InitializeSubTest(iIdentifier).Failed())
    return EZ_FAILURE;

  if (CreateWindow().Failed())
    return EZ_FAILURE;

  // Create a simple quad mesh for stencil testing
  {
    ezGeometry geom;
    geom.AddRect(ezVec2(2.0f, 2.0f), 1, 1);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezMeshVertexStreamType::Position);
    desc.AddStream(ezMeshVertexStreamType::Color0);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    m_hQuadMesh = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>("StencilTestQuad", std::move(desc), "StencilTestQuad");
  }

  m_hStencilShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/StencilColor.ezShader");

  return EZ_SUCCESS;
}

ezResult ezRendererTestStencilStates::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_hQuadMesh.Invalidate();
  m_hStencilShader.Invalidate();

  DestroyWindow();

  if (ezGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

void ezRendererTestStencilStates::RenderQuad(const ezMat4& mTransform, const ezColor& color, ezBitflags<ezShaderBindFlags> ShaderBindFlags)
{
  // Bind our stencil shader (not the base class shader) with the provided flags
  ezRenderContext::GetDefaultInstance()->BindShader(m_hStencilShader, ShaderBindFlags);

  ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
  ocb->m_MVP = mTransform;
  ocb->m_Color = color;

  ezBindGroupBuilder& bindGroupTest = ezRenderContext::GetDefaultInstance()->GetBindGroup();
  bindGroupTest.BindBuffer("PerObject", m_hObjectTransformCB);

  ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hQuadMesh);
  ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();
}

ezTestAppRun ezRendererTestStencilStates::SubtestStencilOperations()
{
  BeginFrame();
  BeginCommands("StencilOperations");
  TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);

  // Setup camera/projection
  ezCamera cam;
  cam.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(ezVec3(0, 0, 0), ezVec3(0, 0, -1), ezVec3(0, 1, 0));
  ezMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  ezMat4 mView = cam.GetViewMatrix();

  // Create depth-stencil state that writes to stencil using Replace operation
  ezGALDepthStencilStateCreationDescription depthStencilDescWrite;
  depthStencilDescWrite.m_bDepthEnable = false;
  depthStencilDescWrite.m_bDepthWrite = false;
  depthStencilDescWrite.m_bStencilEnable = true;
  depthStencilDescWrite.m_uiStencilReadMask = 0xFF;
  depthStencilDescWrite.m_uiStencilWriteMask = 0xFF;

  // Create depth-stencil state that tests against stencil
  ezGALDepthStencilStateCreationDescription depthStencilDescTest;
  depthStencilDescTest.m_bDepthEnable = false;
  depthStencilDescTest.m_bDepthWrite = false;
  depthStencilDescTest.m_bStencilEnable = true;
  depthStencilDescTest.m_uiStencilReadMask = 0xFF;
  depthStencilDescTest.m_uiStencilWriteMask = 0x00; // Don't write to stencil during test

  ezRenderContext* pRenderContext = ezRenderContext::GetDefaultInstance();

  if (m_iFrame == 1)
  {
    // Test: StencilOp::Replace
    // Draw a small quad to write value 1 to stencil using Replace
    // Then draw a larger quad that only passes where stencil == 1
    BeginRendering(ezColor::Black);

    // First pass: Write to stencil with Replace operation
    depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = ezGALStencilOp::Replace;
    depthStencilDescWrite.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Always;
    depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

    ezGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(1);

    // Draw small quad in center
    ezMat4 mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Second pass: Test stencil == 1
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Equal;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    ezGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(1);

    // Draw larger quad - should only appear where stencil was written, so the blue quad should turn red.
    mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, ezColor::Red, ezShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 2)
  {
    // Test: StencilOp::Increment
    // Draw multiple overlapping quads, stencil value increases with each
    // Then test for specific stencil values
    BeginRendering(ezColor::Black);

    depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = ezGALStencilOp::IncrementSaturated;
    depthStencilDescWrite.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Always;
    depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

    ezGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(0);

    // Draw overlapping quads - each increments stencil
    ezMat4 mTransform = ezMat4::MakeTranslation(ezVec3(-0.2f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.5f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    mTransform = ezMat4::MakeTranslation(ezVec3(0.2f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.5f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Test: Draw where stencil == 2 (overlapping region)
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Equal;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    ezGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(2);

    // Draw fullscreen - only the overlap area should be visible
    mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(1.0f));
    RenderQuad(mProj * mView * mTransform, ezColor::Green, ezShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 3)
  {
    // Test: StencilOp::Zero
    // Write to stencil, then zero part of it
    BeginRendering(ezColor::Black);

    // First: Fill stencil with 1
    depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = ezGALStencilOp::Replace;
    depthStencilDescWrite.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Always;
    depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

    ezGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(1);

    ezMat4 mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Zero a smaller region
    depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = ezGALStencilOp::Zero;
    depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

    m_pDevice->DestroyDepthStencilState(hWriteState);
    hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);

    mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Test: Draw where stencil == 1 (non-zeroed region)
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Equal;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    ezGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(1);

    mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(1.0f));
    RenderQuad(mProj * mView * mTransform, ezColor::Yellow, ezShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
  EZ_TEST_IMAGE(m_iFrame, 100);
  EndCommands();
  EndFrame();

  return m_iFrame < 3 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

ezTestAppRun ezRendererTestStencilStates::SubtestStencilCompareFunctions()
{
  BeginFrame();
  BeginCommands("StencilCompareFunctions");
  TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);

  // Setup camera/projection
  ezCamera cam;
  cam.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(ezVec3(0, 0, 0), ezVec3(0, 0, -1), ezVec3(0, 1, 0));
  ezMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  ezMat4 mView = cam.GetViewMatrix();

  ezGALDepthStencilStateCreationDescription depthStencilDescWrite;
  depthStencilDescWrite.m_bDepthEnable = false;
  depthStencilDescWrite.m_bDepthWrite = false;
  depthStencilDescWrite.m_bStencilEnable = true;
  depthStencilDescWrite.m_uiStencilReadMask = 0xFF;
  depthStencilDescWrite.m_uiStencilWriteMask = 0xFF;
  depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = ezGALStencilOp::Replace;
  depthStencilDescWrite.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Always;
  depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

  ezGALDepthStencilStateCreationDescription depthStencilDescTest;
  depthStencilDescTest.m_bDepthEnable = false;
  depthStencilDescTest.m_bDepthWrite = false;
  depthStencilDescTest.m_bStencilEnable = true;
  depthStencilDescTest.m_uiStencilReadMask = 0xFF;
  depthStencilDescTest.m_uiStencilWriteMask = 0x00;

  ezRenderContext* pRenderContext = ezRenderContext::GetDefaultInstance();

  if (m_iFrame == 1)
  {
    // Test: CompareFunc::NotEqual
    BeginRendering(ezColor::Black);

    ezGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(1);

    // Write stencil value of 1 to center
    ezMat4 mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.4f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Test NotEqual - should pass everywhere except center
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::NotEqual;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    ezGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(1);

    mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, ezColor::Green, ezShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 2)
  {
    // Test: CompareFunc::Greater
    BeginRendering(ezColor::Black);

    ezGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(5);

    // Write stencil value of 5 to center
    ezMat4 mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.5f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Test Greater with ref value 3: ref (3) > stencil (5) should not pass, while outside the blue sqaure (ref (3) > stencil 0) should pass
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Greater;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    ezGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(3);

    mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, ezColor::Red, ezShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 3)
  {
    // Test: CompareFunc::LessEqual
    BeginRendering(ezColor::Black);

    // Write stencil value 2 to right half, value 4 to left half
    ezGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);

    pRenderContext->SetStencilRefValue(2);
    ezMat4 mTransform = ezMat4::MakeTranslation(ezVec3(-0.3f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.4f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    pRenderContext->SetStencilRefValue(4);
    mTransform = ezMat4::MakeTranslation(ezVec3(0.3f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.4f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Test LessEqual with ref value 3 - should fail on the right (3 <= 2) but pass on the left (3 <= 4)
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::LessEqual;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    ezGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(3);

    mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(1.0f));
    RenderQuad(mProj * mView * mTransform, ezColor::Yellow, ezShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 4)
  {
    // Test: CompareFunc::Never - should never pass
    BeginRendering(ezColor::Black);

    ezGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);
    pRenderContext->SetStencilRefValue(1);

    ezMat4 mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.5f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Never compare - nothing should be drawn
    depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Never;
    depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

    ezGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(1);

    mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, ezColor::Red, ezShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
  EZ_TEST_IMAGE(m_iFrame, 100);
  EndCommands();
  EndFrame();

  return m_iFrame < 4 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

ezTestAppRun ezRendererTestStencilStates::SubtestStencilRefValue()
{
  if (m_iFrame == 1)
  {
    BeginFrame();
    EndFrame();
  }


  BeginFrame();
  BeginCommands("StencilRefValue");
  TransitionTexture(GetBackbuffer(), ezGALResourceState::RenderTarget);

  // Setup camera/projection
  ezCamera cam;
  cam.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(ezVec3(0, 0, 0), ezVec3(0, 0, -1), ezVec3(0, 1, 0));
  ezMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  ezMat4 mView = cam.GetViewMatrix();

  ezGALDepthStencilStateCreationDescription depthStencilDescWrite;
  depthStencilDescWrite.m_bDepthEnable = false;
  depthStencilDescWrite.m_bDepthWrite = false;
  depthStencilDescWrite.m_bStencilEnable = true;
  depthStencilDescWrite.m_uiStencilReadMask = 0xFF;
  depthStencilDescWrite.m_uiStencilWriteMask = 0xFF;
  depthStencilDescWrite.m_FrontFaceStencilOp.m_PassOp = ezGALStencilOp::Replace;
  depthStencilDescWrite.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Always;
  depthStencilDescWrite.m_BackFaceStencilOp = depthStencilDescWrite.m_FrontFaceStencilOp;

  ezGALDepthStencilStateCreationDescription depthStencilDescTest;
  depthStencilDescTest.m_bDepthEnable = false;
  depthStencilDescTest.m_bDepthWrite = false;
  depthStencilDescTest.m_bStencilEnable = true;
  depthStencilDescTest.m_uiStencilReadMask = 0xFF;
  depthStencilDescTest.m_uiStencilWriteMask = 0x00;
  depthStencilDescTest.m_FrontFaceStencilOp.m_StencilFunc = ezGALCompareFunc::Equal;
  depthStencilDescTest.m_BackFaceStencilOp = depthStencilDescTest.m_FrontFaceStencilOp;

  ezRenderContext* pRenderContext = ezRenderContext::GetDefaultInstance();

  if (m_iFrame == 1)
  {
    // Test: Using SetStencilRefValue to write and test different reference values
    BeginRendering(ezColor::Black);

    ezGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);

    // Write stencil value 10 to upper right
    pRenderContext->SetStencilRefValue(10);
    ezMat4 mTransform = ezMat4::MakeTranslation(ezVec3(-0.3f, 0.3f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Write stencil value 20 to upper left
    pRenderContext->SetStencilRefValue(20);
    mTransform = ezMat4::MakeTranslation(ezVec3(0.3f, 0.3f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Write stencil value 30 to lower right
    pRenderContext->SetStencilRefValue(30);
    mTransform = ezMat4::MakeTranslation(ezVec3(-0.3f, -0.3f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Write stencil value 40 to lower left
    pRenderContext->SetStencilRefValue(40);
    mTransform = ezMat4::MakeTranslation(ezVec3(0.3f, -0.3f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.3f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Now test each quadrant
    ezGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);

    // Test for value 10 - upper right should be red
    pRenderContext->SetStencilRefValue(10);
    mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(1.0f));
    RenderQuad(mProj * mView * mTransform, ezColor::Red, ezShaderBindFlags::NoDepthStencilState);

    // Test for value 20 - upper left should be green
    pRenderContext->SetStencilRefValue(20);
    RenderQuad(mProj * mView * mTransform, ezColor::Green, ezShaderBindFlags::NoDepthStencilState);

    // Test for value 30 - lower right should be yellow
    pRenderContext->SetStencilRefValue(30);
    RenderQuad(mProj * mView * mTransform, ezColor::Yellow, ezShaderBindFlags::NoDepthStencilState);

    // Test for value 40 - lower left should be cyan
    pRenderContext->SetStencilRefValue(40);
    RenderQuad(mProj * mView * mTransform, ezColor::Cyan, ezShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  if (m_iFrame == 2)
  {
    // Test: Stencil mask functionality
    BeginRendering(ezColor::Black);

    // Use a write mask that only writes to lower 4 bits
    depthStencilDescWrite.m_uiStencilWriteMask = 0x0F;

    ezGALDepthStencilStateHandle hWriteState = m_pDevice->CreateDepthStencilState(depthStencilDescWrite);
    pRenderContext->SetDepthStencilState(hWriteState);

    // Write 0xFF - but only lower 4 bits should be written (0x0F)
    pRenderContext->SetStencilRefValue(0xFF);
    ezMat4 mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.5f));
    RenderQuad(mProj * mView * mTransform, ezColor::Blue, ezShaderBindFlags::NoDepthStencilState);

    // Test: Read only lower 4 bits and compare to 0x0F
    depthStencilDescTest.m_uiStencilReadMask = 0x0F;

    ezGALDepthStencilStateHandle hTestState = m_pDevice->CreateDepthStencilState(depthStencilDescTest);
    pRenderContext->SetDepthStencilState(hTestState);
    pRenderContext->SetStencilRefValue(0x0F);

    mTransform = ezMat4::MakeTranslation(ezVec3(0.0f, 0.0f, -2.0f)) * ezMat4::MakeScaling(ezVec3(0.8f));
    RenderQuad(mProj * mView * mTransform, ezColor::Green, ezShaderBindFlags::NoDepthStencilState);

    EndRendering();

    m_pDevice->DestroyDepthStencilState(hWriteState);
    m_pDevice->DestroyDepthStencilState(hTestState);
  }

  TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
  EZ_TEST_IMAGE(m_iFrame, 100);
  EndCommands();
  EndFrame();

  return m_iFrame < 2 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}
