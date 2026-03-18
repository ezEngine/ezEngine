#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>
#include <Texture/Image/Image.h>

namespace
{
  /// Test group subclass that overrides GetImage to return a CPU-side rasterizer image
  /// instead of reading back from the GPU.
  class ezRasterizerTestGroup : public ezSimpleRendererTestGroup
  {
  public:
    ezRasterizerTestGroup(const char* szName)
      : ezSimpleRendererTestGroup(szName)
    {
    }

    virtual ezResult GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber) override
    {
      ref_img.ResetAndMove(std::move(s_Image));
      return EZ_SUCCESS;
    }

    /// Stores the raw rasterizer depth readback as an ezImage for the next EZ_TEST_IMAGE call.
    /// This preserves actual depth values, allowing full output comparison.
    static void SetDepthImage(ezRasterizerView& view)
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
      s_Image.ResetAndAlloc(header);

      ezMemoryUtils::Copy(s_Image.GetPixelPointer<ezColorLinearUB>(), buffer.GetData(), buffer.GetCount());
    }

    /// Stores the rasterizer readback as an ezImage for the next EZ_TEST_IMAGE call.
    /// Converts to a binary coverage mask: white (255,255,255,255) if any depth was written,
    /// black (0,0,0,0) otherwise. This ignores depth value precision differences between
    /// rasterizer implementations and only tests whether the correct pixels are covered.
    static void SetImage(ezRasterizerView& view)
    {
      const ezUInt32 uiWidth = view.GetResolutionX();
      const ezUInt32 uiHeight = view.GetResolutionY();

      ezDynamicArray<ezColorLinearUB> buffer;
      buffer.SetCount(uiWidth * uiHeight);
      view.ReadBackFrame(buffer);

      // Convert to binary coverage mask
      for (auto& pixel : buffer)
      {
        if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0)
        {
          pixel.r = 255;
          pixel.g = 255;
          pixel.b = 255;
          pixel.a = 255;
        }
        else
        {
          pixel.r = 0;
          pixel.g = 0;
          pixel.b = 0;
          pixel.a = 0;
        }
      }

      ezImageHeader header;
      header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
      header.SetWidth(uiWidth);
      header.SetHeight(uiHeight);
      s_Image.ResetAndAlloc(header);

      ezMemoryUtils::Copy(s_Image.GetPixelPointer<ezColorLinearUB>(), buffer.GetData(), buffer.GetCount());
    }

    static ezImage s_Image;
  };

  ezImage ezRasterizerTestGroup::s_Image;
} // namespace

static ezRasterizerTestGroup g_RasterizerTestGroup("Rasterizer");

// Register tests using the custom group
#define EZ_CREATE_RASTERIZER_TEST(TestName)                                                                                                                  \
  static void ezRasterizerTestFunction__##TestName();                                                                                                        \
  ezRegisterSimpleRendererTestHelper ezRegisterRasterizerTest__##TestName(                                                                                   \
    &g_RasterizerTestGroup, EZ_PP_STRINGIFY(TestName), ezRasterizerTestFunction__##TestName);                                                                \
  static void ezRasterizerTestFunction__##TestName()

EZ_CREATE_RASTERIZER_TEST(Rasterizer)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetResolution")
  {
    ezRasterizerView view;
    view.SetResolution(128, 128, 0.0f);
    EZ_TEST_INT(view.GetResolutionX(), 128);
    EZ_TEST_INT(view.GetResolutionY(), 128);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "BeginScene and EndScene empty")
  {
    ezRasterizerView view;
    view.SetResolution(64, 64, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    view.BeginScene();
    view.EndScene();

    EZ_TEST_BOOL(!view.HasRasterizedAnyOccluders());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadBackFrame empty scene")
  {
    constexpr ezUInt32 uiSize = 64;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    view.BeginScene();
    view.EndScene();

    ezDynamicArray<ezColorLinearUB> buffer;
    buffer.SetCount(uiSize * uiSize);
    view.ReadBackFrame(buffer);

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
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsVisible without occluders")
  {
    ezRasterizerView view;
    view.SetResolution(128, 128, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    view.BeginScene();
    view.EndScene();

    ezSimdBBox testBox;
    testBox.m_Min = ezSimdVec4f(-1, -1, -1, 1);
    testBox.m_Max = ezSimdVec4f(1, 1, 1, 1);
    EZ_TEST_BOOL(view.IsVisible(testBox));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CreateBox and CreateQuadX")
  {
    auto pBox = ezRasterizerObject::CreateBox(ezVec3(2, 2, 2));
    EZ_TEST_BOOL(pBox != nullptr);

    auto pBox2 = ezRasterizerObject::CreateBox(ezVec3(2, 2, 2));
    EZ_TEST_BOOL(pBox == pBox2);

    auto pQuad = ezRasterizerObject::CreateQuadX(ezVec2(2, 2));
    EZ_TEST_BOOL(pQuad != nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Rasterize and query occluded")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pOccluder = ezRasterizerObject::CreateBox(ezVec3(20, 20, 0.5f));
    EZ_TEST_BOOL(pOccluder != nullptr);

    view.BeginScene();
    view.AddObject(pOccluder.Borrow(), ezTransform::MakeIdentity());
    view.EndScene();

    EZ_TEST_BOOL(view.HasRasterizedAnyOccluders());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Multiple resolutions")
  {
    for (ezUInt32 size : {32u, 64u, 128u, 256u})
    {
      ezRasterizerView view;
      view.SetResolution(size, size, 0.0f);

      ezCamera camera;
      camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
      camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      view.SetCamera(&camera);

      view.BeginScene();
      view.EndScene();

      EZ_TEST_INT(view.GetResolutionX(), size);
      EZ_TEST_INT(view.GetResolutionY(), size);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Non-square resolution")
  {
    ezRasterizerView view;
    view.SetResolution(128, 64, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    view.BeginScene();
    view.EndScene();

    EZ_TEST_INT(view.GetResolutionX(), 128);
    EZ_TEST_INT(view.GetResolutionY(), 64);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Full screen rasterize produces non-black output")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    // Create a box large enough to fill the entire screen
    auto pBox = ezRasterizerObject::CreateBox(ezVec3(20, 20, 0.5f));

    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform::MakeIdentity());
    view.EndScene();

    EZ_TEST_BOOL(view.HasRasterizedAnyOccluders());

    ezDynamicArray<ezColorLinearUB> buffer;
    buffer.SetCount(uiSize * uiSize);
    view.ReadBackFrame(buffer);

    ezUInt32 uiNonBlackPixels = 0;
    for (const auto& pixel : buffer)
    {
      if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0)
      {
        ++uiNonBlackPixels;
      }
    }

    ezTestFramework::Output(ezTestOutput::Details, "Non-black pixels: %u / %u", uiNonBlackPixels, uiSize * uiSize);

    EZ_TEST_BOOL(uiNonBlackPixels > 0);
  }

  // --- Image comparison tests ---

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Single box depth image")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform::MakeIdentity());
    view.EndScene();

    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(0, 200);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Two boxes depth image")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(3, 3, 3));

    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3(-2, 0, 0)));
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3(2, 0, 0)));
    view.EndScene();

    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(1, 200);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Rotated box depth image")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

    ezQuat rotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(45));

    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rotation));
    view.EndScene();

    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(2, 200);
  }

  // --- Full depth output tests ---

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Single box depth output")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -5), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform::MakeIdentity());
    view.EndScene();

    ezRasterizerTestGroup::SetDepthImage(view);
    EZ_TEST_LINE_IMAGE(3, 200);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Two boxes depth output")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -10), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(3, 3, 3));

    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3(-2, 0, 0)));
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3(2, 0, 0)));
    view.EndScene();

    ezRasterizerTestGroup::SetDepthImage(view);
    EZ_TEST_LINE_IMAGE(4, 200);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Rotated box depth output")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

    ezQuat rotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(45));

    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rotation));
    view.EndScene();

    ezRasterizerTestGroup::SetDepthImage(view);
    EZ_TEST_LINE_IMAGE(5, 200);
  }

  // --- Performance test ---

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Performance: 100 random cubes")
  {
    constexpr ezUInt32 uiSize = 256;
    constexpr ezUInt32 uiNumBoxes = 100;
    constexpr ezUInt32 uiIterations = 100;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -20), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(2, 2, 2));

    // Generate deterministic random transforms
    ezRandom rng;
    rng.Initialize(42);

    ezHybridArray<ezTransform, 128> transforms;
    transforms.SetCount(uiNumBoxes);

    for (ezUInt32 i = 0; i < uiNumBoxes; ++i)
    {
      const float x = (rng.DoubleMinMax(-8.0, 8.0));
      const float y = (rng.DoubleMinMax(-8.0, 8.0));
      const float z = (rng.DoubleMinMax(-5.0, 5.0));

      const float ax = (rng.DoubleMinMax(0.0, 360.0));
      const float ay = (rng.DoubleMinMax(0.0, 360.0));
      const float az = (rng.DoubleMinMax(0.0, 360.0));

      ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(ax), ezAngle::MakeFromDegree(ay), ezAngle::MakeFromDegree(az));
      transforms[i] = ezTransform(ezVec3(x, y, z), rot);
    }

    // Warm up
    view.BeginScene();
    for (ezUInt32 i = 0; i < uiNumBoxes; ++i)
      view.AddObject(pBox.Borrow(), transforms[i]);
    view.EndScene();

    // Timed iterations
    const ezTime tStart = ezTime::Now();

    for (ezUInt32 iter = 0; iter < uiIterations; ++iter)
    {
      view.BeginScene();
      for (ezUInt32 i = 0; i < uiNumBoxes; ++i)
        view.AddObject(pBox.Borrow(), transforms[i]);
      view.EndScene();
    }

    const ezTime tEnd = ezTime::Now();
    const double fTotalMs = (tEnd - tStart).GetMilliseconds();
    const double fPerIterMs = fTotalMs / uiIterations;

    ezTestFramework::Output(ezTestOutput::Details,
      "Rasterized %u boxes x %u iterations in %.2f ms (%.3f ms/iter, %.1f us/box)",
      uiNumBoxes, uiIterations, fTotalMs, fPerIterMs, fPerIterMs * 1000.0 / uiNumBoxes);

    // Image comparison of final frame (higher threshold due to many overlapping edge differences)
    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(6, 200);
  }

  // --- Orientation tests: single box at various rotations ---

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Box orientation: X-axis 30 degrees")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

    ezQuat rot = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(30));
    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rot));
    view.EndScene();

    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(7, 200);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Box orientation: XY-axis 60 degrees")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

    ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(60), ezAngle::MakeFromDegree(60), ezAngle::MakeFromDegree(0));
    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rot));
    view.EndScene();

    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(8, 200);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Box orientation: arbitrary rotation")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

    ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(25), ezAngle::MakeFromDegree(70), ezAngle::MakeFromDegree(15));
    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rot));
    view.EndScene();

    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(9, 200);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Box orientation: offset + rotation")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

    ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(40), ezAngle::MakeFromDegree(55), ezAngle::MakeFromDegree(20));
    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3(2, 1, 0), rot));
    view.EndScene();

    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(10, 200);
  }

  // Full depth output for the arbitrary rotation
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Box depth: arbitrary rotation")
  {
    constexpr ezUInt32 uiSize = 128;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(0, 0, -8), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);

    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));

    ezQuat rot = ezQuat::MakeFromEulerAngles(ezAngle::MakeFromDegree(25), ezAngle::MakeFromDegree(70), ezAngle::MakeFromDegree(15));
    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3::MakeZero(), rot));
    view.EndScene();

    ezRasterizerTestGroup::SetDepthImage(view);
    EZ_TEST_LINE_IMAGE(11, 200);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Custom orientation")
  {
    constexpr ezUInt32 uiSize = 128;
    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);
    ezCamera camera;
    camera.LookAt(ezVec3(2.67014694f, 0.100851476f, -1.52236271f), ezVec3(2.2101965f, 0.115162954f, -0.634533465f), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);
    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));
    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform::MakeIdentity());
    view.EndScene();
    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(99, 200);
  }
}
