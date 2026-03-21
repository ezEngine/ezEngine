#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <Texture/Image/Image.h>

class ezRasterizerTest : public ezGraphicsTest
{
public:
  ezRasterizerTest(const char* szName, bool bOptimized);

  virtual const char* GetTestName() const override { return m_sTestName.GetData(); }
  virtual ezResult GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber) override;
  virtual void MapImageNumberToString(const char* szTestName, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber, ezStringBuilder& out_sString) const override;

private:
  enum SubTests
  {
    ST_SetResolution,
    ST_EmptyScene,
    ST_ReadBackEmpty,
    ST_IsVisibleEmpty,
    ST_CreateObjects,
    ST_RasterizeAndQuery,
    ST_MultipleResolutions,
    ST_NonSquare,
    ST_FullScreenRasterize,
    ST_SingleBox,
    ST_TwoBoxes,
    ST_RotatedBox,
    ST_SingleBoxDepth,
    ST_TwoBoxesDepth,
    ST_RotatedBoxDepth,
    ST_Performance,
    ST_OrientX30,
    ST_OrientXY60,
    ST_OrientArbitrary,
    ST_OrientOffset,
    ST_DepthArbitrary,
    ST_OccludedBoxVisibility,
    ST_PartiallyVisibleBox,
    ST_BoxBesideOccluder,
    ST_BoxInFrontOfOccluder,
    ST_SmallOccluderLargeQuery,
    ST_MultipleOccluders,
    ST_CityPerformance,
  };

  virtual void SetupSubTests() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  /// Creates a rasterizer view using the selected implementation.
  ezUniquePtr<ezRasterizerView> CreateView(ezUInt32 uiWidth, ezUInt32 uiHeight) const;

  /// Stores a binary coverage mask (white if any depth, black otherwise) for image comparison.
  void SetCoverageImage(ezRasterizerView& view);

  /// Stores the raw depth readback for image comparison.
  void SetDepthImage(ezRasterizerView& view);

  bool m_bOptimized = false;
  ezString m_sTestName;
  ezImage m_Image;
};
