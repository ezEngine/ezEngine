#include <RendererTest/RendererTestPCH.h>

#include <RendererTest/DataStructures/RasterizerTest.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>

// Two global instances: one per rasterizer implementation.
static ezRasterizerTest g_RasterizerTestOptimized("Rasterizer_Optimized", true);
static ezRasterizerTest g_RasterizerTestGeneric("Rasterizer_Generic", false);

// --- Construction ---

ezRasterizerTest::ezRasterizerTest(const char* szName, bool bOptimized)
  : m_bOptimized(bOptimized)
  , m_sTestName(szName)
{
}

// --- Image helpers ---

ezResult ezRasterizerTest::GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber)
{
  ref_img.ResetAndMove(std::move(m_Image));
  return EZ_SUCCESS;
}

void ezRasterizerTest::MapImageNumberToString(const char* szTestName, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber, ezStringBuilder& out_sString) const
{
  // Both Optimized and Generic map to the same "Rasterizer" reference images
  out_sString.SetFormat("Rasterizer_{0}_{1}", GetSubTestName(subTest.m_iSubTestIdentifier), ezArgI(uiImageNumber, 3, true));
  out_sString.ReplaceAll(" ", "_");
}

ezUniquePtr<ezRasterizerView> ezRasterizerTest::CreateView(ezUInt32 uiWidth, ezUInt32 uiHeight) const
{
  auto pView = EZ_DEFAULT_NEW(ezRasterizerView, m_bOptimized);
  pView->SetResolution(uiWidth, uiHeight, 0.0f);
  return pView;
}

void ezRasterizerTest::SetCoverageImage(ezRasterizerView& view)
{
  const ezUInt32 uiWidth = view.GetResolutionX();
  const ezUInt32 uiHeight = view.GetResolutionY();

  ezDynamicArray<ezColorLinearUB> buffer;
  buffer.SetCount(uiWidth * uiHeight);
  view.ReadBackFrame(buffer);

  for (auto& pixel : buffer)
  {
    if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0)
    {
      pixel = ezColorLinearUB(255, 255, 255, 255);
    }
    else
    {
      pixel = ezColorLinearUB(0, 0, 0, 0);
    }
  }

  ezImageHeader header;
  header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  m_Image.ResetAndAlloc(header);

  ezMemoryUtils::Copy(m_Image.GetPixelPointer<ezColorLinearUB>(), buffer.GetData(), buffer.GetCount());
}

void ezRasterizerTest::SetDepthImage(ezRasterizerView& view)
{
  const ezUInt32 uiWidth = view.GetResolutionX();
  const ezUInt32 uiHeight = view.GetResolutionY();

  ezDynamicArray<ezColorLinearUB> buffer;
  buffer.SetCount(uiWidth * uiHeight);
  view.ReadBackFrame(buffer);

  ezImageHeader header;
  header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  m_Image.ResetAndAlloc(header);

  ezMemoryUtils::Copy(m_Image.GetPixelPointer<ezColorLinearUB>(), buffer.GetData(), buffer.GetCount());
}

// --- Sub-test registration ---

void ezRasterizerTest::SetupSubTests()
{
  AddSubTest("01 - SetResolution", ST_SetResolution);
  AddSubTest("02 - EmptyScene", ST_EmptyScene);
  AddSubTest("03 - ReadBackEmpty", ST_ReadBackEmpty);
  AddSubTest("04 - IsVisibleEmpty", ST_IsVisibleEmpty);
  AddSubTest("05 - CreateObjects", ST_CreateObjects);
  AddSubTest("06 - RasterizeAndQuery", ST_RasterizeAndQuery);
  AddSubTest("07 - MultipleResolutions", ST_MultipleResolutions);
  AddSubTest("08 - NonSquare", ST_NonSquare);
  AddSubTest("09 - FullScreenRasterize", ST_FullScreenRasterize);
  AddSubTest("10 - SingleBox", ST_SingleBox);
  AddSubTest("11 - TwoBoxes", ST_TwoBoxes);
  AddSubTest("12 - RotatedBox", ST_RotatedBox);
  AddSubTest("13 - SingleBoxDepth", ST_SingleBoxDepth);
  AddSubTest("14 - TwoBoxesDepth", ST_TwoBoxesDepth);
  AddSubTest("15 - RotatedBoxDepth", ST_RotatedBoxDepth);
  AddSubTest("16 - Performance", ST_Performance);
  AddSubTest("17 - OrientX30", ST_OrientX30);
  AddSubTest("18 - OrientXY60", ST_OrientXY60);
  AddSubTest("19 - OrientArbitrary", ST_OrientArbitrary);
  AddSubTest("20 - OrientOffset", ST_OrientOffset);
  AddSubTest("21 - DepthArbitrary", ST_DepthArbitrary);
  AddSubTest("22 - OccludedBoxVisibility", ST_OccludedBoxVisibility);
  AddSubTest("23 - PartiallyVisibleBox", ST_PartiallyVisibleBox);
  AddSubTest("24 - BoxBesideOccluder", ST_BoxBesideOccluder);
  AddSubTest("25 - BoxInFrontOfOccluder", ST_BoxInFrontOfOccluder);
  AddSubTest("26 - SmallOccluderLargeQuery", ST_SmallOccluderLargeQuery);
  AddSubTest("27 - MultipleOccluders", ST_MultipleOccluders);
}

// --- Sub-test execution ---

ezTestAppRun ezRasterizerTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case ST_SetResolution:
    {
      auto pView = CreateView(128, 128);
      EZ_TEST_INT(pView->GetResolutionX(), 128);
      EZ_TEST_INT(pView->GetResolutionY(), 128);
      break;
    }

    case ST_EmptyScene:
    {
      auto pView = CreateView(64, 64);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      pView->BeginScene();
      pView->EndScene();

      EZ_TEST_BOOL(!pView->HasRasterizedAnyOccluders());
      break;
    }

    case ST_ReadBackEmpty:
    {
      constexpr ezUInt32 uiSize = 64;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      pView->BeginScene();
      pView->EndScene();

      ezDynamicArray<ezColorLinearUB> buffer;
      buffer.SetCount(uiSize * uiSize);
      pView->ReadBackFrame(buffer);

      bool bAllZero = true;
      for (const auto& pixel : buffer)
      {
        if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0)
        {
          bAllZero = false;
          break;
        }
      }
      EZ_TEST_BOOL(bAllZero);
      break;
    }

    case ST_IsVisibleEmpty:
    {
      auto pView = CreateView(128, 128);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      pView->BeginScene();
      pView->EndScene();

      ezSimdBBox testBox;
      testBox.m_Min = ezSimdVec4f(-1, -1, -1, 1);
      testBox.m_Max = ezSimdVec4f(1, 1, 1, 1);
      EZ_TEST_BOOL(pView->IsVisible(testBox));
      break;
    }

    case ST_CreateObjects:
    {
      auto pBox = ezRasterizerObject::CreateBox(ezVec3(2, 2, 2));
      EZ_TEST_BOOL(pBox != nullptr);

      auto pBox2 = ezRasterizerObject::CreateBox(ezVec3(2, 2, 2));
      EZ_TEST_BOOL(pBox == pBox2);

      auto pQuad = ezRasterizerObject::CreateQuadX(ezVec2(2, 2));
      EZ_TEST_BOOL(pQuad != nullptr);
      break;
    }

    case ST_RasterizeAndQuery:
    {
      auto pView = CreateView(128, 128);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pOccluder = ezRasterizerObject::CreateBox(ezVec3(20, 20, 0.5f));

      pView->BeginScene();
      pView->AddObject(pOccluder.Borrow(), ezTransform::MakeIdentity());
      pView->EndScene();

      EZ_TEST_BOOL(pView->HasRasterizedAnyOccluders());
      break;
    }

    case ST_MultipleResolutions:
    {
      for (ezUInt32 size : {32u, 64u, 128u, 256u})
      {
        auto pView = CreateView(size, size);

        ezCamera camera;
        camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
        camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
        pView->SetCamera(&camera);

        pView->BeginScene();
        pView->EndScene();

        EZ_TEST_INT(pView->GetResolutionX(), size);
        EZ_TEST_INT(pView->GetResolutionY(), size);
      }
      break;
    }

    case ST_NonSquare:
    {
      auto pView = CreateView(128, 64);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      pView->BeginScene();
      pView->EndScene();

      EZ_TEST_INT(pView->GetResolutionX(), 128);
      EZ_TEST_INT(pView->GetResolutionY(), 64);
      break;
    }

    case ST_FullScreenRasterize:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(20, 20, 0.5f));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform::MakeIdentity());
      pView->EndScene();

      EZ_TEST_BOOL(pView->HasRasterizedAnyOccluders());

      ezDynamicArray<ezColorLinearUB> buffer;
      buffer.SetCount(uiSize * uiSize);
      pView->ReadBackFrame(buffer);

      ezUInt32 uiNonBlack = 0;
      for (const auto& pixel : buffer)
      {
        if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0)
          ++uiNonBlack;
      }

      EZ_TEST_BOOL(uiNonBlack > 0);
      break;
    }

    case ST_SingleBox:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform::MakeIdentity());
      pView->EndScene();

      SetCoverageImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_TwoBoxes:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(3, 3, 3));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3(-2, 0, 0)));
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3(2, 0, 0)));
      pView->EndScene();

      SetCoverageImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_RotatedBox:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));
      ezQuat rotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(45));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rotation));
      pView->EndScene();

      SetCoverageImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_SingleBoxDepth:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform::MakeIdentity());
      pView->EndScene();

      SetDepthImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_TwoBoxesDepth:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(3, 3, 3));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3(-2, 0, 0)));
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3(2, 0, 0)));
      pView->EndScene();

      SetDepthImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_RotatedBoxDepth:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));
      ezQuat rotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(45));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rotation));
      pView->EndScene();

      SetDepthImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_Performance:
    {
      // Ensure all 100 boxes can be rasterized
      if (auto pCVar = static_cast<ezCVarInt*>(ezCVar::FindCVarByName("Spatial.Occlusion.MaxOccluders")))
        *pCVar = 200;

      constexpr ezUInt32 uiSize = 256;
      constexpr ezUInt32 uiNumBoxes = 100;
      constexpr ezUInt32 uiIterations = 10000;

      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(3.904461f, 0.021061f, -9.827855f), ezVec3(3.713979f, -0.038782f, -8.84799f), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(2, 2, 2));

      ezRandom rng;
      rng.Initialize(42);

      ezHybridArray<ezTransform, 128> transforms;
      transforms.SetCount(uiNumBoxes);

      for (ezUInt32 i = 0; i < uiNumBoxes; ++i)
      {
        const float x = (float)rng.DoubleMinMax(-8.0, 8.0);
        const float y = (float)rng.DoubleMinMax(-8.0, 8.0);
        const float z = (float)rng.DoubleMinMax(-5.0, 5.0);
        const float ax = (float)rng.DoubleMinMax(0.0, 360.0);
        const float ay = (float)rng.DoubleMinMax(0.0, 360.0);
        const float az = (float)rng.DoubleMinMax(0.0, 360.0);
        ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(ax), ezAngle::MakeFromDegree(ay), ezAngle::MakeFromDegree(az));
        transforms[i] = ezTransform(ezVec3(x, y, z), rot);
      }

      // Warm up
      pView->BeginScene();
      for (ezUInt32 i = 0; i < uiNumBoxes; ++i)
        pView->AddObject(pBox.Borrow(), transforms[i]);
      pView->EndScene();

      // Timed iterations
      const ezTime tStart = ezTime::Now();
      for (ezUInt32 iter = 0; iter < uiIterations; ++iter)
      {
        pView->BeginScene();
        for (ezUInt32 i = 0; i < uiNumBoxes; ++i)
          pView->AddObject(pBox.Borrow(), transforms[i]);
        pView->EndScene();
      }
      const ezTime tEnd = ezTime::Now();
      const double fTotalMs = (tEnd - tStart).GetMilliseconds();
      const double fPerIterMs = fTotalMs / uiIterations;

      ezTestFramework::Output(ezTestOutput::Details,
        "Rasterized %u boxes x %u iterations in %.2f ms (%.3f ms/iter, %.1f us/box)",
        uiNumBoxes, uiIterations, fTotalMs, fPerIterMs, fPerIterMs * 1000.0 / uiNumBoxes);

      SetCoverageImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_OrientX30:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));
      ezQuat rot = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(30));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rot));
      pView->EndScene();

      SetCoverageImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_OrientXY60:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));
      ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(60), ezAngle::MakeFromDegree(60), ezAngle::MakeFromDegree(0));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rot));
      pView->EndScene();

      SetCoverageImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_OrientArbitrary:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));
      ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(25), ezAngle::MakeFromDegree(70), ezAngle::MakeFromDegree(15));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rot));
      pView->EndScene();

      SetCoverageImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_OrientOffset:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));
      ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(40), ezAngle::MakeFromDegree(55), ezAngle::MakeFromDegree(20));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3(2, 1, 0), rot));
      pView->EndScene();

      SetCoverageImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_DepthArbitrary:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));
      ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(25), ezAngle::MakeFromDegree(70), ezAngle::MakeFromDegree(15));

      pView->BeginScene();
      pView->AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rot));
      pView->EndScene();

      SetDepthImage(*pView);
      EZ_TEST_IMAGE(0, 200);
      break;
    }

    case ST_OccludedBoxVisibility:
    {
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      // Large front occluder that fills the screen
      auto pFrontWall = ezRasterizerObject::CreateBox(ezVec3(20, 20, 0.5f));

      // Small box behind the wall
      auto pBackBox = ezRasterizerObject::CreateBox(ezVec3(2, 2, 2));

      // Rasterize only the front wall as an occluder
      pView->BeginScene();
      pView->AddObject(pFrontWall.Borrow(), ezTransform(ezVec3(0, 0, -2))); // wall at z=-2, closer to camera
      pView->EndScene();

      EZ_TEST_BOOL(pView->HasRasterizedAnyOccluders());

      // Query visibility of a box behind the wall (at z=+5, behind the wall)
      ezSimdBBox behindBox;
      behindBox.m_Min = ezSimdVec4f(-1, -1, 4, 1);
      behindBox.m_Max = ezSimdVec4f(1, 1, 6, 1);

      const bool bVisible = pView->IsVisible(behindBox);
      EZ_TEST_BOOL_MSG(!bVisible, "Box behind a full-screen occluder should be reported as OCCLUDED");

      // Also verify a box in front of the wall IS visible
      ezSimdBBox frontBox;
      frontBox.m_Min = ezSimdVec4f(-1, -1, -5, 1);
      frontBox.m_Max = ezSimdVec4f(1, 1, -3, 1);

      const bool bFrontVisible = pView->IsVisible(frontBox);
      EZ_TEST_BOOL_MSG(bFrontVisible, "Box in front of occluder should be reported as VISIBLE");

      break;
    }

    case ST_PartiallyVisibleBox:
    {
      // A box that is only partially behind a small occluder should be visible.
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      // Small occluder centered in the view
      auto pSmallWall = ezRasterizerObject::CreateBox(ezVec3(4, 4, 0.5f));

      pView->BeginScene();
      pView->AddObject(pSmallWall.Borrow(), ezTransform(ezVec3(0, 0, 0)));
      pView->EndScene();

      // Query a large box behind the wall — extends beyond the wall's edges
      ezSimdBBox largeBox;
      largeBox.m_Min = ezSimdVec4f(-5, -5, 2, 1);
      largeBox.m_Max = ezSimdVec4f(5, 5, 4, 1);

      EZ_TEST_BOOL_MSG(pView->IsVisible(largeBox), "Box partially behind a small occluder should be VISIBLE");
      break;
    }

    case ST_BoxBesideOccluder:
    {
      // A box next to (not behind) an occluder should be visible.
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pWall = ezRasterizerObject::CreateBox(ezVec3(20, 20, 0.5f));

      pView->BeginScene();
      pView->AddObject(pWall.Borrow(), ezTransform(ezVec3(0, 0, 0)));
      pView->EndScene();

      // Box at the same depth but off to the side (still in front of the wall)
      ezSimdBBox sideBox;
      sideBox.m_Min = ezSimdVec4f(3, 3, -2, 1);
      sideBox.m_Max = ezSimdVec4f(5, 5, 0, 1);

      EZ_TEST_BOOL_MSG(pView->IsVisible(sideBox), "Box beside the occluder (in front) should be VISIBLE");

      // Box at the same depth behind the wall but off to where the wall covers
      ezSimdBBox behindSideBox;
      behindSideBox.m_Min = ezSimdVec4f(3, 3, 2, 1);
      behindSideBox.m_Max = ezSimdVec4f(5, 5, 4, 1);

      EZ_TEST_BOOL_MSG(!pView->IsVisible(behindSideBox), "Box behind a full-screen occluder should be OCCLUDED");
      break;
    }

    case ST_BoxInFrontOfOccluder:
    {
      // A box in front of an occluder should always be visible.
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pWall = ezRasterizerObject::CreateBox(ezVec3(20, 20, 0.5f));

      pView->BeginScene();
      pView->AddObject(pWall.Borrow(), ezTransform(ezVec3(0, 0, 0)));
      pView->EndScene();

      // Small box directly in front of the wall
      ezSimdBBox frontBox;
      frontBox.m_Min = ezSimdVec4f(-1, -1, -3, 1);
      frontBox.m_Max = ezSimdVec4f(1, 1, -1, 1);

      EZ_TEST_BOOL_MSG(pView->IsVisible(frontBox), "Box in front of occluder should be VISIBLE");

      // Box very close to the camera
      ezSimdBBox nearBox;
      nearBox.m_Min = ezSimdVec4f(-0.5f, -0.5f, -9, 1);
      nearBox.m_Max = ezSimdVec4f(0.5f, 0.5f, -8, 1);

      EZ_TEST_BOOL_MSG(pView->IsVisible(nearBox), "Box near the camera should be VISIBLE");
      break;
    }

    case ST_SmallOccluderLargeQuery:
    {
      // A tiny occluder should not occlude a large query box that extends around it.
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pTinyWall = ezRasterizerObject::CreateBox(ezVec3(1, 1, 0.5f));

      pView->BeginScene();
      pView->AddObject(pTinyWall.Borrow(), ezTransform(ezVec3(0, 0, 0)));
      pView->EndScene();

      // Large box behind the tiny wall — most of it is not covered
      ezSimdBBox largeBox;
      largeBox.m_Min = ezSimdVec4f(-8, -8, 2, 1);
      largeBox.m_Max = ezSimdVec4f(8, 8, 4, 1);

      EZ_TEST_BOOL_MSG(pView->IsVisible(largeBox), "Large box behind a tiny occluder should be VISIBLE");
      break;
    }

    case ST_MultipleOccluders:
    {
      // Two occluders side by side that together cover a query box behind them.
      constexpr ezUInt32 uiSize = 128;
      auto pView = CreateView(uiSize, uiSize);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&camera);

      auto pWall = ezRasterizerObject::CreateBox(ezVec3(20, 20, 0.5f));

      // Two walls side by side, together covering the entire screen
      pView->BeginScene();
      pView->AddObject(pWall.Borrow(), ezTransform(ezVec3(-10, 0, 0)));
      pView->AddObject(pWall.Borrow(), ezTransform(ezVec3(10, 0, 0)));
      pView->EndScene();

      // Box behind both walls
      ezSimdBBox behindBox;
      behindBox.m_Min = ezSimdVec4f(-1, -1, 4, 1);
      behindBox.m_Max = ezSimdVec4f(1, 1, 6, 1);

      EZ_TEST_BOOL_MSG(!pView->IsVisible(behindBox), "Box behind two combined occluders should be OCCLUDED");

      // Box that extends beyond the occluders' coverage is visible
      // (only if the walls don't fully tile - but 20-wide walls at ±10 DO fully tile, so this should be occluded)
      ezSimdBBox centerBox;
      centerBox.m_Min = ezSimdVec4f(-0.5f, -0.5f, 4, 1);
      centerBox.m_Max = ezSimdVec4f(0.5f, 0.5f, 6, 1);

      EZ_TEST_BOOL_MSG(!pView->IsVisible(centerBox), "Small box behind two tiled occluders should be OCCLUDED");
      break;
    }

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return ezTestAppRun::Quit;
}
