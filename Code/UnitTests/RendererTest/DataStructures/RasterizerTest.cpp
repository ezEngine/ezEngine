#include <RendererTest/RendererTestPCH.h>

#include <RendererTest/DataStructures/RasterizerTest.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <Texture/Image/Image.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <cstdlib>
#  include <fcntl.h>
#  include <unistd.h>
#endif

static ezRasterizerTest g_RasterizerTest;

/// Enables Linux perf recording only around the measured loops.
///
/// RunPerfRasterizer.sh starts 'perf record --delay=-1 --control=fifo:...' and passes the FIFO paths in
/// EZ_PERF_CTL_FIFO and EZ_PERF_ACK_FIFO. Without these variables, or on other platforms, this does nothing.
class ezPerfCaptureScope
{
public:
  ezPerfCaptureScope() { SendCommand("enable\n"); }
  ~ezPerfCaptureScope() { SendCommand("disable\n"); }

private:
  static void SendCommand(const char* szCommand)
  {
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
    static int s_iCtlFd = -2;
    static int s_iAckFd = -1;
    if (s_iCtlFd == -2)
    {
      s_iCtlFd = -1;
      const char* szCtlFifo = std::getenv("EZ_PERF_CTL_FIFO");
      const char* szAckFifo = std::getenv("EZ_PERF_ACK_FIFO");
      if (szCtlFifo != nullptr && szAckFifo != nullptr)
      {
        // perf keeps both FIFOs open read/write, so these calls don't block.
        s_iCtlFd = open(szCtlFifo, O_WRONLY | O_CLOEXEC);
        s_iAckFd = open(szAckFifo, O_RDONLY | O_CLOEXEC);
      }
    }

    if (s_iCtlFd < 0 || s_iAckFd < 0)
      return;

    if (write(s_iCtlFd, szCommand, ezStringUtils::GetStringElementCount(szCommand)) < 0)
      return;

    // Wait for perf to apply the command, so the capture window matches the scope.
    char szAck[16];
    EZ_IGNORE_UNUSED(read(s_iAckFd, szAck, sizeof(szAck)));
#else
    EZ_IGNORE_UNUSED(szCommand);
#endif
  }
};

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
    case ezRasterizerImplementation::MaskedOcclusionCullingEZ:
      return "GenericMOC";
  }

  return "Unknown";
}

namespace
{
  struct Building
  {
    ezTransform m_Transform;
    ezVec3 m_vHalfExtents;
    ezSharedPtr<const ezRasterizerObject> m_pMesh;
  };

  ezSimdBBox MakeBuildingBounds(const Building& building, float fShrink)
  {
    const ezVec3 vQueryHalfExtents = building.m_vHalfExtents * (1.0f - fShrink);
    const ezVec3 vMin = building.m_Transform.m_vPosition - vQueryHalfExtents;
    const ezVec3 vMax = building.m_Transform.m_vPosition + vQueryHalfExtents;

    ezSimdBBox bounds;
    bounds.m_Min.Load<3>(vMin.GetData());
    bounds.m_Max.Load<3>(vMax.GetData());
    bounds.m_Min.SetW(1.0f);
    bounds.m_Max.SetW(1.0f);
    return bounds;
  }

  /// Checks that the rows [0, uiEmptyRowsEnd) contain no rasterized pixel and the rows [uiCoveredRowsBegin, height) are fully covered.
  ///
  /// Only the MaskedOcclusionCulling implementations can read back raw depth, for all others this does nothing.
  void CheckDepthCoverage(const ezRasterizerView& view, ezArrayPtr<const float> depth, ezUInt32 uiEmptyRowsEnd, ezUInt32 uiCoveredRowsBegin)
  {
    const ezUInt32 uiWidth = view.GetResolutionX();
    const ezUInt32 uiHeight = view.GetResolutionY();

    ezUInt32 uiCoveredInEmptyRows = 0;
    for (ezUInt32 i = 0; i < uiEmptyRowsEnd * uiWidth; ++i)
      uiCoveredInEmptyRows += depth[i] >= 0.0f ? 1 : 0;

    ezUInt32 uiEmptyInCoveredRows = 0;
    for (ezUInt32 i = uiCoveredRowsBegin * uiWidth; i < uiHeight * uiWidth; ++i)
      uiEmptyInCoveredRows += depth[i] < 0.0f ? 1 : 0;

    EZ_TEST_INT(uiCoveredInEmptyRows, 0);
    EZ_TEST_INT(uiEmptyInCoveredRows, 0);
  }

  /// Writes the depth buffer as a PNG to the test output folder, closer pixels are brighter and empty pixels are sky blue.
  void WriteDepthImage(const ezRasterizerView& view, ezArrayPtr<const float> depth, ezStringView sName)
  {
    const ezUInt32 uiWidth = view.GetResolutionX();
    const ezUInt32 uiHeight = view.GetResolutionY();

    ezImageHeader header;
    header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
    header.SetWidth(uiWidth);
    header.SetHeight(uiHeight);

    ezImage image;
    image.ResetAndAlloc(header);

    // Logarithmic distance mapping between the closest and farthest covered pixel, so near and far geometry are both distinguishable.
    float fMinLog = ezMath::HighValue<float>();
    float fMaxLog = -ezMath::HighValue<float>();
    for (float fDepth : depth)
    {
      if (fDepth > 0.0f)
      {
        fMinLog = ezMath::Min(fMinLog, ezMath::Ln(1.0f / fDepth));
        fMaxLog = ezMath::Max(fMaxLog, ezMath::Ln(1.0f / fDepth));
      }
    }
    const float fLogRange = ezMath::Max(fMaxLog - fMinLog, 0.001f);

    for (ezUInt32 y = 0; y < uiHeight; ++y)
    {
      ezColorLinearUB* pRow = image.GetPixelPointer<ezColorLinearUB>(0, 0, 0, 0, y);
      for (ezUInt32 x = 0; x < uiWidth; ++x)
      {
        const float fDepth = depth[y * uiWidth + x];
        if (fDepth < 0.0f)
        {
          pRow[x] = ezColorLinearUB(110, 160, 230, 255);
          continue;
        }

        const float fLog = fDepth > 0.0f ? ezMath::Ln(1.0f / fDepth) : fMaxLog;
        const float fBrightness = 1.0f - ezMath::Clamp((fLog - fMinLog) / fLogRange, 0.0f, 1.0f);
        const ezUInt8 uiValue = static_cast<ezUInt8>(30.0f + fBrightness * 225.0f);
        pRow[x] = ezColorLinearUB(uiValue, uiValue, uiValue, 255);
      }
    }

    ezStringBuilder sPath(":imgout/Rasterizer/", sName, ".png");
    if (image.SaveTo(sPath).Succeeded())
      ezTestFramework::Output(ezTestOutput::Details, "Wrote depth image '%s'.", sPath.GetData());
    else
      ezTestFramework::Output(ezTestOutput::Warning, "Failed to write depth image '%s'.", sPath.GetData());
  }

  void TestOccluderBudget(ezRasterizerView& view, ezCVarInt* pMaxOccluders)
  {
    const bool bIsMOC = ezIsMaskedOcclusionCullingImplementation(view.GetImplementation());
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

    if (bIsMOC)
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

    // ThirdParty doesn't clip occluders at the camera near plane.
    if (bIsMOC)
    {
      ezCamera nearCamera;
      nearCamera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 1.0f, 200.0f);
      nearCamera.LookAt(ezVec3::MakeZero(), ezVec3(1.0f, 0.0f, 0.0f), ezVec3(0.0f, 0.0f, 1.0f));
      view.SetCamera(&nearCamera);

      auto pWall = ezRasterizerObject::CreateBox(ezVec3(0.2f, 8.0f, 8.0f));
      ezSimdBBox behindWallBounds;
      behindWallBounds.m_Min = ezSimdVec4f(9.9f, -0.1f, -0.1f, 1.0f);
      behindWallBounds.m_Max = ezSimdVec4f(10.1f, 0.1f, 0.1f, 1.0f);

      view.BeginScene();
      view.AddObject(pWall.Borrow(), ezTransform(ezVec3(0.5f, 0.0f, 0.0f)));
      view.EndScene();
      EZ_TEST_BOOL_MSG(view.IsVisible(behindWallBounds), "Occluders in front of the camera near plane must not occlude anything.");

      view.BeginScene();
      view.AddObject(pWall.Borrow(), ezTransform(ezVec3(1.5f, 0.0f, 0.0f)));
      view.EndScene();
      EZ_TEST_BOOL_MSG(!view.IsVisible(behindWallBounds), "Occluders just behind the camera near plane must occlude.");
    }
  }
} // namespace

void ezRasterizerTest::SetupSubTests()
{
  m_SubTestNames.Clear();

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  const ezCpuFeatures& cpuFeatures = ezSystemInformation::Get().GetCpuFeatures();
  if (cpuFeatures.IsAvx2Available() && cpuFeatures.HW_FMA3)
    AddSceneSubTests(ezRasterizerImplementation::ThirdParty);
#endif

#if defined(EZ_RENDERERTEST_MOC_SUPPORT)
  for (ezRasterizerImplementation implementation : ezMOCInstructionSets)
  {
    // Requesting an unsupported instruction set falls back to a lower one, which already has its own sub-tests.
    ezRasterizerView view(implementation);
    view.SetResolution(32, 8, 0.0f);
    if (view.GetMOCInstructionSet() == implementation)
      AddSceneSubTests(implementation);
  }

  AddSceneSubTests(ezRasterizerImplementation::MaskedOcclusionCullingEZ);
#endif
}

void ezRasterizerTest::AddSceneSubTests(ezRasterizerImplementation implementation)
{
  static constexpr const char* s_SceneNames[] = {"Side Facade", "Behind Facade", "Top Angled", "Side Sky"};
  static_assert(EZ_ARRAY_SIZE(s_SceneNames) == static_cast<ezUInt32>(Scene::Count));

  for (ezInt32 iScene = 0; iScene < static_cast<ezInt32>(Scene::Count); ++iScene)
  {
    // AddSubTest() only stores the name pointer, ezDeque keeps the strings at stable addresses.
    ezString& sName = m_SubTestNames.ExpandAndGetRef();
    sName = ezStringBuilder(GetImplementationName(implementation), " - ", s_SceneNames[iScene]);
    AddSubTest(sName.GetData(), static_cast<ezInt32>(implementation) * static_cast<ezInt32>(Scene::Count) + iScene);
  }
}

ezTestAppRun ezRasterizerTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  EZ_IGNORE_UNUSED(uiInvocationCount);
  const ezRasterizerImplementation implementation = static_cast<ezRasterizerImplementation>(iIdentifier / static_cast<ezInt32>(Scene::Count));
  const Scene scene = static_cast<Scene>(iIdentifier % static_cast<ezInt32>(Scene::Count));

  auto pMaxOccluders = static_cast<ezCVarInt*>(ezCVar::FindCVarByName("Spatial.Occlusion.MaxOccluders"));
  EZ_TEST_BOOL_MSG(pMaxOccluders != nullptr, "The rasterizer occluder limit CVar must be registered.");
  if (pMaxOccluders == nullptr)
    return ezTestAppRun::Quit;

  const ezInt32 iPreviousMaxOccluders = *pMaxOccluders;
  EZ_SCOPE_EXIT(*pMaxOccluders = iPreviousMaxOccluders);
  *pMaxOccluders = 500;

  constexpr ezUInt32 uiResolution = 1024;
  constexpr ezUInt32 uiRasterizationIterations = 50000;
  constexpr ezUInt32 uiQueryIterations = 1000;
  constexpr ezUInt32 uiGridSize = 10;
  constexpr float fBlockSize = 4.0f;
  constexpr float fStreetWidth = 2.0f;
  constexpr float fCellSize = fBlockSize + fStreetWidth;
  constexpr float fGroundSize = uiGridSize * fCellSize + fStreetWidth;
  constexpr float fGroundHalf = fGroundSize * 0.5f;
  constexpr float fFacadeWidth = fGroundSize - 9.0f;
  constexpr float fLandscapeSize = 4000.0f;
  constexpr float fShrinkStep = 0.001f;
  constexpr float fMaxShrink = 0.999f;

  const bool bWithFacade = scene == Scene::SideFacade || scene == Scene::BehindFacade;
  auto pGround = bWithFacade ? ezRasterizerObject::CreateBox(ezVec3(fGroundSize, fGroundSize, 0.5f)) : ezRasterizerObject::CreateBox(ezVec3(fLandscapeSize, fLandscapeSize, 1.0f));
  const ezTransform groundTransform(ezVec3(0.0f, 0.0f, bWithFacade ? -0.25f : -0.5f));
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

  // Rows [0, uiEmptyRowsEnd) must stay empty, rows [uiCoveredRowsBegin, uiResolution) must be fully covered.
  ezUInt32 uiEmptyRowsEnd = 0;
  ezUInt32 uiCoveredRowsBegin = uiResolution;

  ezCamera camera;
  camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 200.0f);
  switch (scene)
  {
    case Scene::SideFacade:
      camera.LookAt(ezVec3(-80.0f, 0.0f, 7.0f), ezVec3(0.0f, 0.0f, 5.0f), ezVec3(0.0f, 0.0f, 1.0f));
      break;
    case Scene::BehindFacade:
      // The facade's front side is 5 units away and covers more than the 90 degree field of view.
      camera.LookAt(ezVec3(-37.0f, 0.0f, 5.0f), ezVec3(0.0f, 0.0f, 5.0f), ezVec3(0.0f, 0.0f, 1.0f));
      uiCoveredRowsBegin = 0;
      break;
    case Scene::TopAngled:
      // Looks down at 60 degrees, so the whole city is in view and the landscape reaches the top edge.
      camera.LookAt(ezVec3(-27.5f, 0.0f, 47.63f), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));
      uiCoveredRowsBegin = 0;
      break;
    case Scene::SideSky:
      // Horizontal view above the highest building: the horizon is (almost) at the vertical center.
      camera.LookAt(ezVec3(-45.0f, 0.0f, 12.0f), ezVec3(0.0f, 0.0f, 12.0f), ezVec3(0.0f, 0.0f, 1.0f));
      uiEmptyRowsEnd = uiResolution / 2 - 1;
      uiCoveredRowsBegin = uiResolution / 2 + 8;
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return ezTestAppRun::Quit;
  }
  view.SetCamera(&camera);

  auto rasterizeScene = [&]()
  {
    view.BeginScene();
    view.AddObject(pGround.Borrow(), groundTransform);
    if (bWithFacade)
      view.AddObject(pFacade.Borrow(), ezTransform(ezVec3(-31.0f, 0.0f, 5.0f)));
    for (const Building& building : buildings)
      view.AddObject(building.m_pMesh.Borrow(), building.m_Transform);
    view.EndScene();
  };

  rasterizeScene();
  EZ_TEST_BOOL_MSG(view.HasRasterizedAnyOccluders(), "The city must rasterize occluders.");

  ezDynamicArray<float> firstFrameDepth;
  firstFrameDepth.SetCountUninitialized(uiResolution * uiResolution);
  const bool bHasDepth = view.ReadBackDepth(firstFrameDepth).Succeeded();
  if (bHasDepth)
  {
    CheckDepthCoverage(view, firstFrameDepth, uiEmptyRowsEnd, uiCoveredRowsBegin);

    if (ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-rasterizerDepthImages"))
      WriteDepthImage(view, firstFrameDepth, ezTestFramework::GetInstance()->GetCurrentSubTest()->m_szSubTestName);
  }

  double fRasterizationMs = 0.0;
  {
    ezPerfCaptureScope perfCapture;
    const ezTime tRasterizationStart = ezTime::Now();
    for (ezUInt32 i = 0; i < uiRasterizationIterations; ++i)
      rasterizeScene();
    fRasterizationMs = (ezTime::Now() - tRasterizationStart).GetMilliseconds();
  }

  if (bHasDepth)
  {
    // Only the first frame starts from a fully cleared buffer, later frames depend on BeginScene() resetting everything the previous frame wrote.
    ezDynamicArray<float> lastFrameDepth;
    lastFrameDepth.SetCountUninitialized(uiResolution * uiResolution);
    EZ_TEST_BOOL(view.ReadBackDepth(lastFrameDepth).Succeeded());
    EZ_TEST_BOOL_MSG(lastFrameDepth.GetArrayPtr() == firstFrameDepth.GetArrayPtr(), "Rasterizing the same scene again must produce the same depth buffer.");
  }

  auto countVisibleBuildings = [&](float fShrink)
  {
    ezUInt32 uiVisibleCount = 0;
    for (const Building& building : buildings)
      uiVisibleCount += view.IsVisible(MakeBuildingBounds(building, fShrink)) ? 1 : 0;
    return uiVisibleCount;
  };

  // The side facade view searches the smallest AABB inset at which every building is occluded, all other views query the full-size AABBs.
  float fQueryShrink = 0.0f;
  if (scene == Scene::SideFacade)
  {
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
        fQueryShrink = fShrink;
        bFoundShrink = true;
        break;
      }
    }

    EZ_TEST_BOOL_MSG(bFoundShrink, "No AABB size reduction in the search range occluded every city building. See details for the remaining visible count.");
    if (!bFoundShrink)
    {
      ezTestFramework::Output(ezTestOutput::Details, "At maximum shrink %.3f, %u of %u building AABBs remain visible.", fMaxShrink, uiVisibleCountAtMaxShrink, buildings.GetCount());
      return ezTestAppRun::Quit;
    }
  }

  ezHybridArray<ezSimdBBox, 128> queryBounds;
  queryBounds.Reserve(buildings.GetCount());
  for (const Building& building : buildings)
    queryBounds.PushBack(MakeBuildingBounds(building, fQueryShrink));

  ezUInt32 uiVisibleCount = 0;
  double fQueryMs = 0.0;
  {
    // ezPerfCaptureScope perfCapture;
    const ezTime tQueryStart = ezTime::Now();
    for (ezUInt32 i = 0; i < uiQueryIterations; ++i)
    {
      uiVisibleCount = 0;
      for (const ezSimdBBox& bounds : queryBounds)
        uiVisibleCount += view.IsVisible(bounds) ? 1 : 0;
    }
    fQueryMs = (ezTime::Now() - tQueryStart).GetMilliseconds();
  }

  switch (scene)
  {
    case Scene::SideFacade:
      EZ_TEST_INT_MSG(uiVisibleCount, 0, "At the selected inset, every building AABB must be occluded.");
      break;
    case Scene::BehindFacade:
      EZ_TEST_INT_MSG(uiVisibleCount, 0, "The facade must hide every building.");
      break;
    case Scene::TopAngled:
    case Scene::SideSky:
      EZ_TEST_BOOL_MSG(uiVisibleCount >= buildings.GetCount() * 7 / 10, "Most buildings must be visible.");
      break;
    default:
      break;
  }

  ezTestFramework::Output(ezTestOutput::Details,
    "%s (active: %s): rasterized %u objects x %u iterations in %.2f ms (%.3f ms/iteration); queried %u building AABBs (shrink %.2f%%, %u visible) x %u iterations in %.2f ms (%.3f ms/iteration)",
    GetImplementationName(implementation), GetImplementationName(view.GetMOCInstructionSet()), buildings.GetCount() + (bWithFacade ? 2 : 1), uiRasterizationIterations,
    fRasterizationMs, fRasterizationMs / uiRasterizationIterations, queryBounds.GetCount(), fQueryShrink * 100.0f, uiVisibleCount, uiQueryIterations, fQueryMs,
    fQueryMs / uiQueryIterations);

  if (scene == Scene::SideFacade)
    TestOccluderBudget(view, pMaxOccluders);

  return ezTestAppRun::Quit;
}
