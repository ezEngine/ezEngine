#include <RendererTest/RendererTestPCH.h>

#include <RendererTest/DataStructures/RasterizerTest2.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererFoundation/Resources/Texture.h>

static ezRasterizerTest2 s_RasterizerTest2;

void ezRasterizerTest2::SetupSubTests()
{
  static constexpr const char* s_ImplementationNames[] = {
    "ThirdParty", "MOC SSE2", "MOC SSE4_1", "MOC AVX2", "GenericMOC"};
  static_assert(EZ_ARRAY_SIZE(s_ImplementationNames) == static_cast<ezUInt32>(ezRasterizerImplementation::Auto));

  for (ezInt32 i = 0; i < static_cast<ezInt32>(ezRasterizerImplementation::Auto); ++i)
  {
    if (ezIsRasterizerImplementationSupported(static_cast<ezRasterizerImplementation>(i)))
      AddSubTest(s_ImplementationNames[i], i);
  }
}

ezResult ezRasterizerTest2::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));

  constexpr ezUInt32 uiWidth = 1024;
  constexpr ezUInt32 uiHeight = 512;
  EZ_SUCCEED_OR_RETURN(CreateWindow(uiWidth, uiHeight));

  m_pView = EZ_DEFAULT_NEW(ezRasterizerView, static_cast<ezRasterizerImplementation>(iIdentifier));
  m_pView->SetResolution(uiWidth, uiHeight, 0.0f);

  m_Camera.LookAt(ezVec3(-27.5f, 0.0f, 47.63f), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 70.0f, 0.1f, 200.0f);
  m_pView->SetCamera(&m_Camera);

  CreateCity();

  ezGALTextureCreationDescription textureDesc;
  textureDesc.m_uiWidth = uiWidth;
  textureDesc.m_uiHeight = uiHeight;
  textureDesc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
  textureDesc.m_ResourceAccess.m_bImmutable = false;
  m_hTexture = m_pDevice->CreateTexture(textureDesc);
  if (m_hTexture.IsInvalidated())
  {
    ezLog::Error("Failed to create the rasterizer display texture.");
    return EZ_FAILURE;
  }
  m_Pixels.SetCountUninitialized(uiWidth * uiHeight);
  m_hLineBox = CreateLineBox(1.0f, 1.0f, 1.0f);
  m_hLineShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/StencilColor.ezShader");
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Copy.ezShader");
  return EZ_SUCCESS;
}

void ezRasterizerTest2::CreateCity()
{
  constexpr ezUInt32 uiGridSize = 10;
  constexpr float fBlockSize = 4.0f;
  constexpr float fStreetWidth = 2.0f;
  constexpr float fCellSize = fBlockSize + fStreetWidth;
  constexpr float fGroundSize = uiGridSize * fCellSize + fStreetWidth;
  constexpr float fGroundHalf = fGroundSize * 0.5f;

  ezRandom rng;
  rng.Initialize(123);
  for (ezUInt32 x = 0; x < uiGridSize; ++x)
  {
    for (ezUInt32 y = 0; y < uiGridSize; ++y)
    {
      const float fHeight = 2.0f + static_cast<float>(rng.DoubleMinMax(0.0, 8.0));
      const float fWidth = fBlockSize * (0.6f + static_cast<float>(rng.DoubleMinMax(0.0, 0.4)));
      const float fDepth = fBlockSize * (0.6f + static_cast<float>(rng.DoubleMinMax(0.0, 0.4)));
      const float fX = -fGroundHalf + fStreetWidth + x * fCellSize + fBlockSize * 0.5f;
      const float fY = -fGroundHalf + fStreetWidth + y * fCellSize + fBlockSize * 0.5f;

      Building& building = m_Buildings.ExpandAndGetRef();
      building.m_Transform = ezTransform(ezVec3(fX, fY, fHeight * 0.5f));
      building.m_vExtents = ezVec3(fWidth, fDepth, fHeight);
      building.m_pObject = ezRasterizerObject::CreateBox(building.m_vExtents);
    }
  }
  m_pGround = ezRasterizerObject::CreateBox(ezVec3(fGroundSize, fGroundSize, 0.5f));
  m_pFacade = ezRasterizerObject::CreateBox(ezVec3(2.0f, fGroundSize - 9.0f, 12.0f));
}

ezResult ezRasterizerTest2::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_pView.Clear();
  m_Buildings.Clear();
  m_pGround.Clear();
  m_pFacade.Clear();
  m_hLineBox.Invalidate();
  m_hLineShader.Invalidate();
  m_Pixels.Clear();
  if (!m_hTexture.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture);
    m_hTexture.Invalidate();
  }
  DestroyWindow();
  return ezGraphicsTest::DeInitializeSubTest(iIdentifier);
}

void ezRasterizerTest2::UpdateDepthTexture(ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  m_pView->ReadBackFrame(m_Pixels);

  // ReadBackFrame returns bottom-to-top rows, while the display texture uses top-to-bottom rows.
  for (ezUInt32 y = 0; y < uiHeight / 2; ++y)
  {
    for (ezUInt32 x = 0; x < uiWidth; ++x)
      ezMath::Swap(m_Pixels[y * uiWidth + x], m_Pixels[(uiHeight - 1 - y) * uiWidth + x]);
  }

  ezGALSystemMemoryDescription sourceData;
  sourceData.m_pData = ezMakeArrayPtr(reinterpret_cast<const ezUInt8*>(m_Pixels.GetData()), m_Pixels.GetCount() * sizeof(ezColorLinearUB));
  sourceData.m_uiRowPitch = m_pView->GetResolutionX() * sizeof(ezColorLinearUB);
  m_pDevice->UpdateTextureForNextFrame(m_hTexture, sourceData);
}

ezTestAppRun ezRasterizerTest2::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  auto pMaxOccluders = static_cast<ezCVarInt*>(ezCVar::FindCVarByName("Spatial.Occlusion.MaxOccluders"));
  if (!EZ_TEST_BOOL_MSG(pMaxOccluders != nullptr, "The rasterizer occluder limit CVar must be registered."))
    return ezTestAppRun::Quit;
  const ezInt32 iPreviousMaxOccluders = *pMaxOccluders;
  EZ_SCOPE_EXIT(*pMaxOccluders = iPreviousMaxOccluders);
  *pMaxOccluders = 200;

  m_pView->BeginScene();
  m_pView->AddObject(m_pGround.Borrow(), ezTransform(ezVec3(0.0f, 0.0f, -0.25f)));
  m_pView->AddObject(m_pFacade.Borrow(), ezTransform(ezVec3(-31.0f, 0.0f, 5.0f)));
  for (const Building& building : m_Buildings)
    m_pView->AddObject(building.m_pObject.Borrow(), building.m_Transform);
  m_pView->EndScene();
  EZ_TEST_BOOL(m_pView->HasRasterizedAnyOccluders());

  const ezUInt32 uiWidth = m_pView->GetResolutionX();
  const ezUInt32 uiHeight = m_pView->GetResolutionY();
  UpdateDepthTexture(uiWidth, uiHeight);

  m_iFrame = uiInvocationCount;
  BeginFrame();
  {
    BeginCommands("Rasterizer City");
    TransitionTexture(m_hTexture, ezGALResourceState::ShaderResource, {}, ezGALShaderStageFlags::PixelShader);
    BeginRendering(ezColor::Black);
    {
      ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
      pContext->SetShaderPermutationVariable("CAMERA_MODE", "CAMERA_MODE_MONO");
      pContext->BindShader(m_hShader);
      pContext->GetBindGroup().BindTexture("Input", m_hTexture, {}, ezGALResourceFormat::Invalid, ezGALTextureType::Texture2DArray);
      pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
      EZ_TEST_BOOL(pContext->DrawMeshBuffer().Succeeded());

      ezMat4 mProjection;
      m_Camera.GetProjectionMatrix(static_cast<float>(uiWidth) / uiHeight, mProjection);
      const ezMat4 mViewProjection = mProjection * m_Camera.GetViewMatrix();
      pContext->BindShader(m_hLineShader);
      pContext->BindMeshBuffer(m_hLineBox);
      pContext->GetBindGroup().BindBuffer("PerObject", m_hObjectTransformCB);
      for (const Building& building : m_Buildings)
      {
        const ezBoundingBox bounds = ezBoundingBox::MakeFromCenterAndHalfExtents(building.m_Transform.m_vPosition, building.m_vExtents * 0.5f);
        const bool bVisible = m_pView->IsVisible(ezSimdConversion::ToBBox(bounds));
        ObjectCB* pConstants = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
        pConstants->m_MVP = mViewProjection * building.m_Transform.GetAsMat4() * ezMat4::MakeScaling(building.m_vExtents);
        pConstants->m_Color = bVisible ? ezColor::Green : ezColor::Red;
        EZ_TEST_BOOL(pContext->DrawMeshBuffer().Succeeded());
      }
    }
    EndRendering();
    if (uiInvocationCount == 10)
    {
      TransitionTexture(GetBackbuffer(), ezGALResourceState::CopySource);
      EZ_TEST_IMAGE(10, 1);
    }
    EndCommands();
  }
  EndFrame();
  return uiInvocationCount >= 10 ? ezTestAppRun::Quit : ezTestAppRun::Continue;
}
