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
    camera.LookAt(ezVec3(3.904461f, 0.021061f, -9.827855f), ezVec3(3.713979f, -0.038782f, -8.84799f), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    //camera.LookAt(ezVec3(0, 0, -20), ezVec3(0, 0, 0), ezVec3(0, 1, 0));
    //camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
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
}

EZ_CREATE_RASTERIZER_TEST(BROKEN)
{
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

    // Debug: count covered pixels
    ezDynamicArray<ezColorLinearUB> buffer;
    buffer.SetCount(uiSize * uiSize);
    view.ReadBackFrame(buffer);
    ezUInt32 uiCovered = 0;
    for (const auto& pixel : buffer)
    {
      if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0)
        ++uiCovered;
    }
    // Check column coverage: for each x, find min/max y with content
    for (ezUInt32 x = 0; x < uiSize; x += 8)
    {
      ezInt32 minY = -1, maxY = -1;
      for (ezUInt32 y = 0; y < uiSize; ++y)
      {
        const auto& p = buffer[y * uiSize + x];
        if (p.r != 0 || p.g != 0 || p.b != 0)
        {
          if (minY < 0) minY = y;
          maxY = y;
        }
      }
      if (minY >= 0)
        ezTestFramework::Output(ezTestOutput::Details, "x=%u: y=[%d,%d]", x, minY, maxY);
    }
    ezTestFramework::Output(ezTestOutput::Details, "HasRasterized: %s", view.HasRasterizedAnyOccluders() ? "true" : "false");
    // Check raw pixel values at (0,50)
    const auto& px = buffer[50 * uiSize + 0];
    ezTestFramework::Output(ezTestOutput::Details, "Pixel (0,50): r=%u g=%u b=%u a=%u", px.r, px.g, px.b, px.a);
    ezTestFramework::Output(ezTestOutput::Details, "Total covered: %u", uiCovered);

    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(99, 200);
  }
}

EZ_CREATE_RASTERIZER_TEST(BROKEN_AGAIN)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Custom orientation")
  {
    constexpr ezUInt32 uiSize = 256;
    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);
    ezCamera camera;
    camera.LookAt(ezVec3(-0.185162f, -0.141942f, -4.621447f), ezVec3(0.341782f, -0.142796f, -3.771547f), ezVec3(0, 1, 0));
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
    view.SetCamera(&camera);
    auto pBox = ezRasterizerObject::CreateBox(ezVec3(4, 4, 4));
    view.BeginScene();
    view.AddObject(pBox.Borrow(), ezTransform::MakeIdentity());
    view.AddObject(pBox.Borrow(), ezTransform(ezVec3(5, 0, 0)));
    view.EndScene();

    // Diagnostic: render each cube separately and compare
    {
      // Cube 1 alone
      ezRasterizerView view1;
      view1.SetResolution(uiSize, uiSize, 0.0f);
      view1.SetCamera(&camera);
      view1.BeginScene();
      view1.AddObject(pBox.Borrow(), ezTransform::MakeIdentity());
      view1.EndScene();

      ezDynamicArray<ezColorLinearUB> buf1;
      buf1.SetCount(uiSize * uiSize);
      view1.ReadBackFrame(buf1);

      // Cube 2 alone
      ezRasterizerView view2;
      view2.SetResolution(uiSize, uiSize, 0.0f);
      view2.SetCamera(&camera);
      view2.BeginScene();
      view2.AddObject(pBox.Borrow(), ezTransform(ezVec3(5, 0, 0)));
      view2.EndScene();

      ezDynamicArray<ezColorLinearUB> buf2;
      buf2.SetCount(uiSize * uiSize);
      view2.ReadBackFrame(buf2);

      // Combined
      ezDynamicArray<ezColorLinearUB> bufCombined;
      bufCombined.SetCount(uiSize * uiSize);
      view.ReadBackFrame(bufCombined);

      ezUInt32 uiCov1 = 0, uiCov2 = 0, uiCovCombined = 0, uiCovUnion = 0;
      ezUInt32 uiMissingInCombined = 0;
      for (ezUInt32 i = 0; i < uiSize * uiSize; ++i)
      {
        bool b1 = buf1[i].r != 0 || buf1[i].g != 0 || buf1[i].b != 0;
        bool b2 = buf2[i].r != 0 || buf2[i].g != 0 || buf2[i].b != 0;
        bool bc = bufCombined[i].r != 0 || bufCombined[i].g != 0 || bufCombined[i].b != 0;
        if (b1) ++uiCov1;
        if (b2) ++uiCov2;
        if (bc) ++uiCovCombined;
        if (b1 || b2) ++uiCovUnion;
        if ((b1 || b2) && !bc) ++uiMissingInCombined;
      }

      ezLog::Warning("Cube1 alone: {} covered", uiCov1);
      ezLog::Warning("Cube2 alone: {} covered", uiCov2);
      ezLog::Warning("Combined: {} covered", uiCovCombined);
      ezLog::Warning("Union of separate: {} covered", uiCovUnion);
      ezLog::Warning("Missing in combined (union - combined): {}", uiMissingInCombined);

      // Find bounding box of missing pixels
      ezInt32 minMX = 9999, maxMX = -1, minMY = 9999, maxMY = -1;
      for (ezUInt32 y = 0; y < uiSize; ++y)
      {
        for (ezUInt32 x = 0; x < uiSize; ++x)
        {
          ezUInt32 i = y * uiSize + x;
          bool b1 = buf1[i].r != 0 || buf1[i].g != 0 || buf1[i].b != 0;
          bool b2 = buf2[i].r != 0 || buf2[i].g != 0 || buf2[i].b != 0;
          bool bc = bufCombined[i].r != 0 || bufCombined[i].g != 0 || bufCombined[i].b != 0;
          if ((b1 || b2) && !bc)
          {
            if ((int)x < minMX) minMX = x;
            if ((int)x > maxMX) maxMX = x;
            if ((int)y < minMY) minMY = y;
            if ((int)y > maxMY) maxMY = y;
          }
        }
      }
      ezLog::Warning("Missing pixel bounds: x=[{},{}] y=[{},{}]", minMX, maxMX, minMY, maxMY);

      // Check which cube's pixels are missing
      ezUInt32 missFrom1 = 0, missFrom2 = 0;
      for (ezUInt32 i = 0; i < uiSize * uiSize; ++i)
      {
        bool b1 = buf1[i].r != 0 || buf1[i].g != 0 || buf1[i].b != 0;
        bool b2 = buf2[i].r != 0 || buf2[i].g != 0 || buf2[i].b != 0;
        bool bc = bufCombined[i].r != 0 || bufCombined[i].g != 0 || bufCombined[i].b != 0;
        if (b1 && !bc) ++missFrom1;
        if (b2 && !bc) ++missFrom2;
      }
      ezLog::Warning("Missing from cube1: {}, from cube2: {}", missFrom1, missFrom2);

      // Sample some missing pixels — check raw depth values
      ezUInt32 dumped = 0;
      for (ezUInt32 y = 0; y < uiSize && dumped < 5; ++y)
      {
        for (ezUInt32 x = 0; x < uiSize && dumped < 5; ++x)
        {
          ezUInt32 i = y * uiSize + x;
          bool b2 = buf2[i].r != 0 || buf2[i].g != 0 || buf2[i].b != 0;
          bool bc = bufCombined[i].r != 0 || bufCombined[i].g != 0 || bufCombined[i].b != 0;
          if (b2 && !bc)
          {
            ezLog::Warning("Missing pixel ({},{}) combined=({},{},{},{}) cube2=({},{},{},{}) block=({},{})",
              x, y,
              bufCombined[i].r, bufCombined[i].g, bufCombined[i].b, bufCombined[i].a,
              buf2[i].r, buf2[i].g, buf2[i].b, buf2[i].a,
              x / 8, y / 8);
            ++dumped;
          }
        }
      }
    }

    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(99, 10);
  }
}

EZ_CREATE_RASTERIZER_TEST(BROKEN_AGAIN_2)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Custom orientation")
  {
    constexpr ezUInt32 uiSize = 256;
    constexpr ezUInt32 uiNumBoxes = 100;
    constexpr ezUInt32 uiIterations = 100;

    ezRasterizerView view;
    view.SetResolution(uiSize, uiSize, 0.0f);

    ezCamera camera;
    camera.LookAt(ezVec3(3.904461f, 0.021061f, -9.827855f), ezVec3(3.713979f, -0.038782f, -8.84799f), ezVec3(0, 1, 0));
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

    // Diagnostics: find which cube is entirely missing from combined render
    {
      ezDynamicArray<ezColorLinearUB> bufCombined;
      bufCombined.SetCount(uiSize * uiSize);
      view.ReadBackFrame(bufCombined);

      ezUInt32 uiCombinedCov = 0;
      for (const auto& p : bufCombined)
        if (p.r != 0 || p.g != 0 || p.b != 0) ++uiCombinedCov;
      ezLog::Warning("Combined coverage: {}", uiCombinedCov);

      // For each cube: render alone, check how many of its pixels appear in the combined output
      for (ezUInt32 i = 0; i < uiNumBoxes; ++i)
      {
        ezRasterizerView viewSingle;
        viewSingle.SetResolution(uiSize, uiSize, 0.0f);
        viewSingle.SetCamera(&camera);
        viewSingle.BeginScene();
        viewSingle.AddObject(pBox.Borrow(), transforms[i]);
        viewSingle.EndScene();

        ezDynamicArray<ezColorLinearUB> bufSingle;
        bufSingle.SetCount(uiSize * uiSize);
        viewSingle.ReadBackFrame(bufSingle);

        ezUInt32 uiSingleCov = 0, uiInCombined = 0;
        for (ezUInt32 p = 0; p < uiSize * uiSize; ++p)
        {
          bool bs = bufSingle[p].r != 0 || bufSingle[p].g != 0 || bufSingle[p].b != 0;
          bool bc = bufCombined[p].r != 0 || bufCombined[p].g != 0 || bufCombined[p].b != 0;
          if (bs) ++uiSingleCov;
          if (bs && bc) ++uiInCombined;
        }

        // Report cubes that render alone but have zero overlap with combined
        if (uiSingleCov > 0 && uiInCombined == 0)
          ezLog::Warning("Box {} ENTIRELY MISSING: {} pixels alone, 0 in combined", i, uiSingleCov);
        else if (uiSingleCov > 0 && uiInCombined < uiSingleCov / 2)
          ezLog::Warning("Box {} MOSTLY MISSING: {} alone, {} in combined", i, uiSingleCov, uiInCombined);
      }
    }

    // Image comparison of final frame (higher threshold due to many overlapping edge differences)
    ezRasterizerTestGroup::SetImage(view);
    EZ_TEST_LINE_IMAGE(1, 200);
  }
}
