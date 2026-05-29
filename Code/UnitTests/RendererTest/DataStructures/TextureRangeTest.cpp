#include <RendererTest/RendererTestPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

EZ_CREATE_SIMPLE_RENDERER_TEST(DataStructures, ezGALTextureRange)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default constructed range represents all")
  {
    ezGALTextureRange range;
    EZ_TEST_INT(range.m_uiBaseArraySlice, 0);
    EZ_TEST_INT(range.m_uiArraySlices, EZ_GAL_ALL_ARRAY_SLICES);
    EZ_TEST_INT(range.m_uiBaseMipLevel, 0);
    EZ_TEST_INT(range.m_uiMipLevels, EZ_GAL_ALL_MIP_LEVELS);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeFromMipRange")
  {
    ezGALTextureRange range = ezGALTextureRange::MakeFromMipRange(2, 3);
    EZ_TEST_INT(range.m_uiBaseArraySlice, 0);
    EZ_TEST_INT(range.m_uiArraySlices, 1);
    EZ_TEST_INT(range.m_uiBaseMipLevel, 2);
    EZ_TEST_INT(range.m_uiMipLevels, 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeFromRenderTargetRange")
  {
    ezGALRenderTargetRange rtRange;
    rtRange.m_uiBaseArraySlice = 3;
    rtRange.m_uiArraySlices = 2;
    rtRange.m_uiBaseMipLevel = 1;

    ezGALTextureRange range = ezGALTextureRange::MakeFromRenderTargetRange(rtRange);
    EZ_TEST_INT(range.m_uiBaseArraySlice, 3);
    EZ_TEST_INT(range.m_uiArraySlices, 2);
    EZ_TEST_INT(range.m_uiBaseMipLevel, 1);
    EZ_TEST_INT(range.m_uiMipLevels, 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeFromMipLevel for RenderTargetRange")
  {
    ezGALRenderTargetRange rtRange = ezGALRenderTargetRange::MakeFromMipLevel(3);
    EZ_TEST_INT(rtRange.m_uiBaseArraySlice, 0);
    EZ_TEST_INT(rtRange.m_uiArraySlices, EZ_GAL_ALL_ARRAY_SLICES);
    EZ_TEST_INT(rtRange.m_uiBaseMipLevel, 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Equality operators")
  {
    ezGALTextureRange a = {0, 1, 0, 1};
    ezGALTextureRange b = {0, 1, 0, 1};
    ezGALTextureRange c = {1, 1, 0, 1};

    EZ_TEST_BOOL(a == b);
    EZ_TEST_BOOL(!(a != b));
    EZ_TEST_BOOL(a != c);
    EZ_TEST_BOOL(!(a == c));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Multi-mip multi-layer")
  {
    ezGALTextureRange fullRange = {0, 3, 0, 4};

    EZ_TEST_INT(ezGALTextureRange::ComputeSubResourceIndex(0, 0, fullRange), 0);
    EZ_TEST_INT(ezGALTextureRange::ComputeSubResourceIndex(1, 0, fullRange), 1);
    EZ_TEST_INT(ezGALTextureRange::ComputeSubResourceIndex(3, 0, fullRange), 3);
    EZ_TEST_INT(ezGALTextureRange::ComputeSubResourceIndex(0, 1, fullRange), 4);
    EZ_TEST_INT(ezGALTextureRange::ComputeSubResourceIndex(2, 1, fullRange), 6);
    EZ_TEST_INT(ezGALTextureRange::ComputeSubResourceIndex(0, 2, fullRange), 8);
    EZ_TEST_INT(ezGALTextureRange::ComputeSubResourceIndex(3, 2, fullRange), 11);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Single-mip texture")
  {
    ezGALTextureRange singleMip = {0, 5, 0, 1};
    EZ_TEST_INT(ezGALTextureRange::ComputeSubResourceIndex(0, 0, singleMip), 0);
    EZ_TEST_INT(ezGALTextureRange::ComputeSubResourceIndex(0, 3, singleMip), 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Single-layer texture")
  {
    ezGALTextureRange singleLayer = {0, 1, 0, 8};
    EZ_TEST_INT(ezGALTextureRange::ComputeSubResourceIndex(5, 0, singleLayer), 5);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Identical ranges")
  {
    ezGALTextureRange a = {0, 2, 0, 3};
    EZ_TEST_BOOL(a.Overlaps(a));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Fully contained")
  {
    ezGALTextureRange outer = {0, 4, 0, 4};
    ezGALTextureRange inner = {1, 2, 1, 2};
    EZ_TEST_BOOL(outer.Overlaps(inner));
    EZ_TEST_BOOL(inner.Overlaps(outer));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Partial overlap in both dimensions")
  {
    ezGALTextureRange a = {0, 3, 0, 3};
    ezGALTextureRange b = {2, 3, 2, 3};
    EZ_TEST_BOOL(a.Overlaps(b));
    EZ_TEST_BOOL(b.Overlaps(a));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Adjacent slices do not overlap")
  {
    ezGALTextureRange a = {0, 2, 0, 4};
    ezGALTextureRange b = {2, 2, 0, 4};
    EZ_TEST_BOOL(!a.Overlaps(b));
    EZ_TEST_BOOL(!b.Overlaps(a));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Adjacent mips do not overlap")
  {
    ezGALTextureRange a = {0, 4, 0, 2};
    ezGALTextureRange b = {0, 4, 2, 2};
    EZ_TEST_BOOL(!a.Overlaps(b));
    EZ_TEST_BOOL(!b.Overlaps(a));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Disjoint slices with overlapping mips")
  {
    ezGALTextureRange a = {0, 2, 0, 4};
    ezGALTextureRange b = {3, 2, 1, 2};
    EZ_TEST_BOOL(!a.Overlaps(b));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overlapping slices with disjoint mips")
  {
    ezGALTextureRange a = {0, 4, 0, 2};
    ezGALTextureRange b = {1, 2, 3, 2};
    EZ_TEST_BOOL(!a.Overlaps(b));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Single sub-resource vs single sub-resource")
  {
    ezGALTextureRange a = {1, 1, 2, 1};
    ezGALTextureRange same = {1, 1, 2, 1};
    ezGALTextureRange diffSlice = {2, 1, 2, 1};
    ezGALTextureRange diffMip = {1, 1, 3, 1};

    EZ_TEST_BOOL(a.Overlaps(same));
    EZ_TEST_BOOL(!a.Overlaps(diffSlice));
    EZ_TEST_BOOL(!a.Overlaps(diffMip));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Single sub-resource vs wide range")
  {
    ezGALTextureRange wide = {0, 8, 0, 8};
    ezGALTextureRange single = {3, 1, 5, 1};
    EZ_TEST_BOOL(wide.Overlaps(single));
    EZ_TEST_BOOL(single.Overlaps(wide));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default range overlaps with everything")
  {
    ezGALTextureRange fullRange;
    EZ_TEST_BOOL(fullRange.Overlaps(fullRange));

    ezGALTextureRange origin = {0, 1, 0, 1};
    EZ_TEST_BOOL(fullRange.Overlaps(origin));
    EZ_TEST_BOOL(origin.Overlaps(fullRange));

    ezGALTextureRange high = {100, 1, 50, 1};
    EZ_TEST_BOOL(fullRange.Overlaps(high));
    EZ_TEST_BOOL(high.Overlaps(fullRange));

    ezGALTextureRange maxSingle = {EZ_GAL_ALL_ARRAY_SLICES - 1, 1, EZ_GAL_ALL_MIP_LEVELS - 1, 1};
    EZ_TEST_BOOL(fullRange.Overlaps(maxSingle));
    EZ_TEST_BOOL(maxSingle.Overlaps(fullRange));

    ezGALTextureRange shiftedFull = {5, EZ_GAL_ALL_ARRAY_SLICES, 3, EZ_GAL_ALL_MIP_LEVELS};
    EZ_TEST_BOOL(fullRange.Overlaps(shiftedFull));
    EZ_TEST_BOOL(shiftedFull.Overlaps(fullRange));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sentinel values with non-zero base (overflow edge cases)")
  {
    ezGALTextureRange a = {1, EZ_GAL_ALL_ARRAY_SLICES, 0, 1};
    ezGALTextureRange b = {0, 1, 0, 1};
    EZ_TEST_BOOL(!a.Overlaps(b));
    EZ_TEST_BOOL(!b.Overlaps(a));

    ezGALTextureRange c = {1, 1, 0, 1};
    EZ_TEST_BOOL(a.Overlaps(c));

    ezGALTextureRange d = {0, 1, 1, EZ_GAL_ALL_MIP_LEVELS};
    ezGALTextureRange e = {0, 1, 0, 1};
    EZ_TEST_BOOL(!d.Overlaps(e));
    EZ_TEST_BOOL(!e.Overlaps(d));

    ezGALTextureRange f = {0, 1, 1, 1};
    EZ_TEST_BOOL(d.Overlaps(f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Two shifted sentinel ranges overlap")
  {
    ezGALTextureRange a = {10, EZ_GAL_ALL_ARRAY_SLICES, 5, EZ_GAL_ALL_MIP_LEVELS};
    ezGALTextureRange b = {20, EZ_GAL_ALL_ARRAY_SLICES, 10, EZ_GAL_ALL_MIP_LEVELS};
    EZ_TEST_BOOL(a.Overlaps(b));
    EZ_TEST_BOOL(b.Overlaps(a));
  }
}
