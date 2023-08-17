#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <RendererTest/Advanced/AdvancedFeatures.h>

ezResult ezRendererTestAdvancedFeatures::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  if (iIdentifier == ST_ReadRenderTarget)
  {
    // Texture2D
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, ezGALResourceFormat::BGRAUByteNormalizedsRGB, ezGALMSAASampleCount::None);
    m_hTexture2D = m_pDevice->CreateTexture(desc);

    ezGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = m_hTexture2D;
    viewDesc.m_uiMipLevelsToUse = 1;
    for (ezUInt32 i = 0; i < 4; i++)
    {
      viewDesc.m_uiMostDetailedMipLevel = 0;
      m_hTexture2DMips[i] = m_pDevice->CreateResourceView(viewDesc);
    }

    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColor.ezShader");
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
  }

  if (iIdentifier == ST_VertexShaderRenderTargetArrayIndex)
  {
    if (!m_pDevice->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
    {
      ezTestFramework::GetInstance()->Output(ezTestOutput::Warning, "VertexShaderRenderTargetArrayIndex capability not supported, skipping test.");
      return EZ_SUCCESS;
    }
    // Texture2DArray
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(320 / 2, 240, ezGALResourceFormat::BGRAUByteNormalizedsRGB, ezGALMSAASampleCount::None);
    desc.m_uiArraySize = 2;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Stereo.ezShader");
    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/StereoPreview.ezShader");
  }

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestAdvancedFeatures::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_hShader2.Invalidate();

  if (!m_hTexture2D.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2D);
    m_hTexture2D.Invalidate();
  }

  if (!m_hTexture2DArray.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2DArray);
    m_hTexture2DArray.Invalidate();
  }

  DestroyWindow();
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::DeInitializeSubTest(iIdentifier));
  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestAdvancedFeatures::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;
  BeginFrame();

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      ReadRenderTarget();
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      if (!m_pDevice->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
        return ezTestAppRun::Quit;
      VertexShaderRenderTargetArrayIndex();
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

void ezRendererTestAdvancedFeatures::ReadRenderTarget()
{
  BeginPass("Offscreen");
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2D));
    renderingSetup.m_ClearColor = ezColor::RebeccaPurple;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

    ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
    ezGALRenderCommandEncoder* pCommandEncoder = ezRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndPass();


  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginPass("Texture2D");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2DMips[0]);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
  }
  EndPass();
}

void ezRendererTestAdvancedFeatures::VertexShaderRenderTargetArrayIndex()
{
  m_bCaptureImage = true;
  const ezMat4 mMVP = CreateSimpleMVP((m_pWindow->GetClientAreaSize().width / 2.0f) / (float)m_pWindow->GetClientAreaSize().height);
  BeginPass("Offscreen Stereo");
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2DArray));
    renderingSetup.m_ClearColor = ezColor::RebeccaPurple;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

    ezRectFloat viewport = ezRectFloat(0, 0, m_pWindow->GetClientAreaSize().width / 2.0f, (float)m_pWindow->GetClientAreaSize().height);
    ezGALRenderCommandEncoder* pCommandEncoder = ezRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader, ezShaderBindFlags::None);
    ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mMVP;
    ocb->m_Color = ezColor(1, 1, 1, 1);
    ezRenderContext::GetDefaultInstance()->BindConstantBuffer("PerObject", m_hObjectTransformCB);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer(0xFFFFFFFF, 0, 2).IgnoreResult();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndPass();


  BeginPass("Texture2DArray");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

    ezRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_pDevice->GetDefaultResourceView(m_hTexture2DArray));

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }

    EndRendering();
  }
  EndPass();
}

static ezRendererTestAdvancedFeatures g_AdvancedFeaturesTest;
