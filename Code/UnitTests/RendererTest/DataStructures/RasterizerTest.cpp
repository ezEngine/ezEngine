#include <RendererTest/RendererTestPCH.h>

#include <RendererTest/DataStructures/RasterizerTest.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>

static ezRasterizerTest g_RasterizerTest;

static const char* GetImplementationName(ezRasterizerImplementation implementation)
{
  switch (implementation)
  {
    case ezRasterizerImplementation::ThirdParty:
      return "ThirdParty";
    case ezRasterizerImplementation::MaskedOcclusionCullingSSE2:
      return "MOC SSE2";
    case ezRasterizerImplementation::MaskedOcclusionCullingSSE41:
      return "MOC SSE4.1";
    case ezRasterizerImplementation::MaskedOcclusionCullingAVX2:
      return "MOC AVX2";
    case ezRasterizerImplementation::MaskedOcclusionCullingAVX512:
      return "MOC AVX-512";
    case ezRasterizerImplementation::GenericMOC:
      return "GenericMOC";
  }

  return "Unknown";
}

void ezRasterizerTest::SetupSubTests()
{
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  const ezCpuFeatures& cpuFeatures = ezSystemInformation::Get().GetCpuFeatures();
  if (cpuFeatures.IsAvx2Available() && cpuFeatures.HW_FMA3)
    AddSubTest("01 - ThirdParty CityPerformance", ST_ThirdPartyCityPerformance);
#endif

#if defined(EZ_RENDERERTEST_MOC_SUPPORT)
  auto AddMOCSubTest = [this](const char* szName, SubTests subTest)
  {
    const ezRasterizerImplementation implementation = GetImplementation(subTest);
    ezRasterizerView view(implementation);
    view.SetResolution(32, 8, 0.0f);
    if (view.GetMOCInstructionSet() == implementation)
      AddSubTest(szName, subTest);
  };

  AddMOCSubTest("02 - MOC SSE2 CityPerformance", ST_MOCSSE2CityPerformance);
  AddMOCSubTest("03 - MOC SSE4.1 CityPerformance", ST_MOCSSE41CityPerformance);
  AddMOCSubTest("04 - MOC AVX2 CityPerformance", ST_MOCAVX2CityPerformance);
  AddMOCSubTest("05 - MOC AVX-512 CityPerformance", ST_MOCAVX512CityPerformance);
  AddSubTest("06 - GenericMOC CityPerformance", ST_GenericMOCCityPerformance);
#endif
}

ezRasterizerImplementation ezRasterizerTest::GetImplementation(ezInt32 iIdentifier)
{
  switch (iIdentifier)
  {
    case ST_ThirdPartyCityPerformance:
      return ezRasterizerImplementation::ThirdParty;
    case ST_MOCSSE2CityPerformance:
      return ezRasterizerImplementation::MaskedOcclusionCullingSSE2;
    case ST_MOCSSE41CityPerformance:
      return ezRasterizerImplementation::MaskedOcclusionCullingSSE41;
    case ST_MOCAVX2CityPerformance:
      return ezRasterizerImplementation::MaskedOcclusionCullingAVX2;
    case ST_MOCAVX512CityPerformance:
      return ezRasterizerImplementation::MaskedOcclusionCullingAVX512;
    case ST_GenericMOCCityPerformance:
      return ezRasterizerImplementation::GenericMOC;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return ezRasterizerImplementation::ThirdParty;
  }
}

ezTestAppRun ezRasterizerTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  EZ_IGNORE_UNUSED(uiInvocationCount);
  const ezRasterizerImplementation implementation = GetImplementation(iIdentifier);

  auto pMaxOccluders = static_cast<ezCVarInt*>(ezCVar::FindCVarByName("Spatial.Occlusion.MaxOccluders"));
  EZ_TEST_BOOL_MSG(pMaxOccluders != nullptr, "The rasterizer occluder limit CVar must be registered.");
  if (pMaxOccluders == nullptr)
    return ezTestAppRun::Quit;

  const ezInt32 iPreviousMaxOccluders = *pMaxOccluders;
  *pMaxOccluders = 500;

  constexpr ezUInt32 uiResolution = 1024;
  constexpr ezUInt32 uiRasterizationIterations = 1000;
  constexpr ezUInt32 uiQueryIterations = 1000;
  constexpr ezUInt32 uiGridSize = 10;
  constexpr float fBlockSize = 4.0f;
  constexpr float fStreetWidth = 2.0f;
  constexpr float fCellSize = fBlockSize + fStreetWidth;
  constexpr float fGroundSize = uiGridSize * fCellSize + fStreetWidth;
  constexpr float fGroundHalf = fGroundSize * 0.5f;
  constexpr float fFacadeWidth = fGroundSize - 9.0f;
  constexpr float fShrinkStep = 0.001f;
  constexpr float fMaxShrink = 0.999f;

  struct Building
  {
    ezTransform m_Transform;
    ezVec3 m_vHalfExtents;
    ezSharedPtr<const ezRasterizerObject> m_pMesh;
  };

  auto pGround = ezRasterizerObject::CreateBox(ezVec3(fGroundSize, fGroundSize, 0.5f));
  auto pFacade = ezRasterizerObject::CreateBox(ezVec3(2.0f, fFacadeWidth, 12.0f));

  ezRandom rng;
  rng.Initialize(123);

  ezHybridArray<Building, 128> buildings;
  for (ezUInt32 gx = 0; gx < uiGridSize; ++gx)
  {
    for (ezUInt32 gy = 0; gy < uiGridSize; ++gy)
    {
      const float fHeight = 2.0f + static_cast<float>(rng.DoubleMinMax(0.0, 8.0));
      const float fWidth = fBlockSize * (0.6f + static_cast<float>(rng.DoubleMinMax(0.0, 0.4)));
      const float fDepth = fBlockSize * (0.6f + static_cast<float>(rng.DoubleMinMax(0.0, 0.4)));

      const float x = -fGroundHalf + fStreetWidth + gx * fCellSize + fBlockSize * 0.5f;
      const float y = -fGroundHalf + fStreetWidth + gy * fCellSize + fBlockSize * 0.5f;
      const ezVec3 vExtents(fWidth, fDepth, fHeight);

      Building& building = buildings.ExpandAndGetRef();
      building.m_Transform = ezTransform(ezVec3(x, y, fHeight * 0.5f));
      building.m_vHalfExtents = vExtents * 0.5f;
      building.m_pMesh = ezRasterizerObject::CreateBox(vExtents);
    }
  }

  ezRasterizerView view(implementation);
  view.SetResolution(uiResolution, uiResolution, 0.0f);

  ezCamera camera;
  camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 200.0f);
  camera.LookAt(ezVec3(-80.0f, 0.0f, 7.0f), ezVec3(0.0f, 0.0f, 5.0f), ezVec3(0.0f, 0.0f, 1.0f));
  view.SetCamera(&camera);

  auto rasterizeCity = [&]()
  {
    view.BeginScene();
    view.AddObject(pGround.Borrow(), ezTransform(ezVec3(0.0f, 0.0f, -0.25f)));
    view.AddObject(pFacade.Borrow(), ezTransform(ezVec3(-31.0f, 0.0f, 5.0f)));
    for (const Building& building : buildings)
      view.AddObject(building.m_pMesh.Borrow(), building.m_Transform);
    view.EndScene();
  };

  rasterizeCity();

  const ezTime tRasterizationStart = ezTime::Now();
  for (ezUInt32 i = 0; i < uiRasterizationIterations; ++i)
    rasterizeCity();
  const double fRasterizationMs = (ezTime::Now() - tRasterizationStart).GetMilliseconds();
  EZ_TEST_BOOL_MSG(view.HasRasterizedAnyOccluders(), "The side view must rasterize city occluders.");

  auto countVisibleBuildings = [&](float fShrink)
  {
    ezUInt32 uiVisibleCount = 0;
    for (const Building& building : buildings)
    {
      const ezVec3 vQueryHalfExtents = building.m_vHalfExtents * (1.0f - fShrink);
      const ezVec3 vMin = building.m_Transform.m_vPosition - vQueryHalfExtents;
      const ezVec3 vMax = building.m_Transform.m_vPosition + vQueryHalfExtents;

      ezSimdBBox bounds;
      bounds.m_Min.Load<3>(vMin.GetData());
      bounds.m_Max.Load<3>(vMax.GetData());
      bounds.m_Min.SetW(1.0f);
      bounds.m_Max.SetW(1.0f);

      uiVisibleCount += view.IsVisible(bounds) ? 1 : 0;
    }
    return uiVisibleCount;
  };

  float fMinimumShrink = 0.0f;
  bool bFoundShrink = false;
  ezUInt32 uiVisibleCountAtMaxShrink = 0;
  EZ_TEST_BOOL_MSG(countVisibleBuildings(0.0f) > 0, "Full-size building AABBs should extend beyond the foreground facade.");
  constexpr ezUInt32 uiShrinkSteps = static_cast<ezUInt32>(fMaxShrink / fShrinkStep);
  for (ezUInt32 i = 0; i <= uiShrinkSteps; ++i)
  {
    const float fShrink = i * fShrinkStep;
    const ezUInt32 uiVisibleCount = countVisibleBuildings(fShrink);
    if (i == uiShrinkSteps)
      uiVisibleCountAtMaxShrink = uiVisibleCount;

    if (uiVisibleCount == 0)
    {
      fMinimumShrink = fShrink;
      bFoundShrink = true;
      break;
    }
  }

  EZ_TEST_BOOL_MSG(bFoundShrink, "No AABB size reduction in the search range occluded every city building. See details for the remaining visible count.");
  if (!bFoundShrink)
  {
    ezTestFramework::Output(ezTestOutput::Details, "At maximum shrink %.3f, %u of %u building AABBs remain visible.", fMaxShrink, uiVisibleCountAtMaxShrink, buildings.GetCount());
    for (ezUInt32 i = 0; i < buildings.GetCount(); ++i)
    {
      const Building& building = buildings[i];
      const ezVec3 vQueryHalfExtents = building.m_vHalfExtents * (1.0f - fMaxShrink);
      const ezVec3 vMin = building.m_Transform.m_vPosition - vQueryHalfExtents;
      const ezVec3 vMax = building.m_Transform.m_vPosition + vQueryHalfExtents;
      ezSimdBBox bounds;
      bounds.m_Min.Load<3>(vMin.GetData());
      bounds.m_Max.Load<3>(vMax.GetData());
      bounds.m_Min.SetW(1.0f);
      bounds.m_Max.SetW(1.0f);
      if (view.IsVisible(bounds))
      {
        ezTestFramework::Output(ezTestOutput::Details, "Visible building %u at (%.2f, %.2f, %.2f).", i,
          building.m_Transform.m_vPosition.x, building.m_Transform.m_vPosition.y, building.m_Transform.m_vPosition.z);
        if (i > 10)
          break;
      }
    }
    *pMaxOccluders = iPreviousMaxOccluders;
    return ezTestAppRun::Quit;
  }

  ezHybridArray<ezSimdBBox, 128> queryBounds;
  queryBounds.Reserve(buildings.GetCount());
  for (const Building& building : buildings)
  {
    const ezVec3 vQueryHalfExtents = building.m_vHalfExtents * (1.0f - fMinimumShrink);
    const ezVec3 vMin = building.m_Transform.m_vPosition - vQueryHalfExtents;
    const ezVec3 vMax = building.m_Transform.m_vPosition + vQueryHalfExtents;

    ezSimdBBox& bounds = queryBounds.ExpandAndGetRef();
    bounds.m_Min.Load<3>(vMin.GetData());
    bounds.m_Max.Load<3>(vMax.GetData());
    bounds.m_Min.SetW(1.0f);
    bounds.m_Max.SetW(1.0f);
  }

  bool bAllQueriesOccluded = true;
  const ezTime tQueryStart = ezTime::Now();
  for (ezUInt32 i = 0; i < uiQueryIterations; ++i)
  {
    for (const ezSimdBBox& bounds : queryBounds)
      bAllQueriesOccluded &= !view.IsVisible(bounds);
  }
  const double fQueryMs = (ezTime::Now() - tQueryStart).GetMilliseconds();

  EZ_TEST_BOOL_MSG(bAllQueriesOccluded, "At the selected inset, every building AABB must be occluded.");

  *pMaxOccluders = iPreviousMaxOccluders;

  const ezRasterizerImplementation activeImplementation = view.GetMOCInstructionSet();
  ezTestFramework::Output(ezTestOutput::Details,
    "%s (active: %s) side city: rasterized %u objects x %u iterations in %.2f ms (%.3f ms/iteration); queried %u building AABBs x %u iterations in %.2f ms (%.3f ms/iteration); minimum AABB shrink %.2f%%",
    GetImplementationName(implementation), GetImplementationName(activeImplementation), buildings.GetCount() + 2, uiRasterizationIterations, fRasterizationMs,
    fRasterizationMs / uiRasterizationIterations, queryBounds.GetCount(), uiQueryIterations, fQueryMs,
    fQueryMs / uiQueryIterations, fMinimumShrink * 100.0f);

  {
    *pMaxOccluders = 2;
    ezCamera budgetCamera;
    budgetCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 200.0f);
    budgetCamera.LookAt(ezVec3::MakeZero(), ezVec3(1.0f, 0.0f, 0.0f), ezVec3(0.0f, 0.0f, 1.0f));
    view.SetCamera(&budgetCamera);

    auto pFront = ezRasterizerObject::CreateBox(ezVec3(2.0f, 4.0f, 4.0f));
    auto pBox = ezRasterizerObject::CreateBox(ezVec3(2.0f));
    const ezQuat qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0.0f, 0.0f, 1.0f), ezAngle::MakeFromDegree(45.0f));
    const ezTransform farTransform(ezVec3(30.0f, 15.0f, 0.0f), qRotation, ezVec3(2.0f, 0.5f, 1.0f));

    // Reverse insertion order checks sorting; the hidden middle object must not consume the two-object budget.
    view.BeginScene();
    view.AddObject(pBox.Borrow(), farTransform);
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3(20.0f, 0.0f, 0.0f), qRotation, ezVec3(2.0f, 0.5f, 1.0f)));
    view.AddObject(pFront.Borrow(), ezTransform(ezVec3(10.0f, 0.0f, 0.0f)));
    view.EndScene();

    const ezVec3 vQueryCenter = farTransform.m_vPosition;
    ezSimdBBox farBounds;
    farBounds.m_Min = ezSimdVec4f(vQueryCenter.x - 0.1f, vQueryCenter.y - 0.1f, vQueryCenter.z - 0.1f, 1.0f);
    farBounds.m_Max = ezSimdVec4f(vQueryCenter.x + 0.1f, vQueryCenter.y + 0.1f, vQueryCenter.z + 0.1f, 1.0f);
    EZ_TEST_BOOL_MSG(!view.IsVisible(farBounds), "Occluded objects must not consume the rasterization budget.");

    for (float fOffset : {-4.0f, 0.0f, 4.0f})
    {
      ezSimdBBox hiddenBounds;
      hiddenBounds.m_Min = ezSimdVec4f(29.9f, fOffset - 0.1f, fOffset - 0.1f, 0.0f);
      hiddenBounds.m_Max = ezSimdVec4f(30.1f, fOffset + 0.1f, fOffset + 0.1f, 0.0f);
      EZ_TEST_BOOL_MSG(!view.IsVisible(hiddenBounds), "Bounds behind the front box must be occluded across all projected corners.");

      ezSimdBBox visibleBounds;
      visibleBounds.m_Min = ezSimdVec4f(29.9f, fOffset - 0.1f, 9.9f, 0.0f);
      visibleBounds.m_Max = ezSimdVec4f(30.1f, fOffset + 0.1f, 10.1f, 0.0f);
      EZ_TEST_BOOL_MSG(view.IsVisible(visibleBounds), "Bounds above the occluders must remain visible.");
    }

    if (ezIsMaskedOcclusionCullingImplementation(implementation))
    {
      ezSimdBBox nearBounds;
      nearBounds.m_Min = ezSimdVec4f(0.05f, -0.01f, -0.01f, 0.0f);
      nearBounds.m_Max = ezSimdVec4f(0.15f, 0.01f, 0.01f, 0.0f);
      EZ_TEST_BOOL_MSG(view.IsVisible(nearBounds), "Bounds crossing the near plane must remain visible.");
      nearBounds.m_Min.SetX(-0.05f);
      EZ_TEST_BOOL_MSG(view.IsVisible(nearBounds), "Bounds crossing the camera plane must remain visible.");
    }

    view.BeginScene();
    view.EndScene();
    EZ_TEST_BOOL_MSG(view.IsVisible(farBounds), "The next scene must not reuse the previous scene's occlusion.");
    *pMaxOccluders = iPreviousMaxOccluders;
  }

  return ezTestAppRun::Quit;
}
