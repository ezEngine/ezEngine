#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Texture/TexConv/TexConvProcessor.h>

namespace
{
  /// Ray marches the height field from every texel through every higher texel. Used for ezTexConvUsage::ConeMap.
  namespace ConeMapPrecise
  {
    /// Depth values (0 = top, 1 = deepest) of a height map, sampled with wrap around addressing.
    ///
    /// The texture is also divided into blocks, which store depth bounds to skip work that can't change a cone ratio.
    /// These bounds only skip candidates that can't produce a smaller ratio, so the result is the same as without them.
    struct DepthField
    {
      ezDynamicArray<float> m_Depth;
      ezInt32 m_iWidth = 0;
      ezInt32 m_iHeight = 0;

      struct Block
      {
        EZ_DECLARE_POD_TYPE();

        float m_fMinDepth; ///< smallest depth of the texels in the block
        float m_fMaxDepth; ///< largest depth of the texels in the block and in the next row and column, which bilinear samples within the block also read

        // largest absolute depth difference between horizontal / vertical neighbors, that bilinear samples within the block interpolate between.
        // These bound the gradient of the interpolated surface.
        float m_fMaxSlopeX;
        float m_fMaxSlopeY;
      };

      ezInt32 m_iMaskX = -1; ///< width - 1 for power of two widths, otherwise -1
      ezInt32 m_iMaskY = -1;

      ezInt32 m_iBlockSize = 1;
      ezInt32 m_iBlockShift = 0;
      ezInt32 m_iNumBlocksX = 0;
      ezInt32 m_iNumBlocksY = 0;
      ezDynamicArray<Block> m_Blocks;

      EZ_ALWAYS_INLINE static ezInt32 Wrap(ezInt32 i, ezInt32 iSize)
      {
        i %= iSize;
        return i + ((i < 0) ? iSize : 0);
      }

      EZ_ALWAYS_INLINE ezInt32 WrapX(ezInt32 x) const { return (m_iMaskX >= 0) ? (x & m_iMaskX) : Wrap(x, m_iWidth); }
      EZ_ALWAYS_INLINE ezInt32 WrapY(ezInt32 y) const { return (m_iMaskY >= 0) ? (y & m_iMaskY) : Wrap(y, m_iHeight); }

      EZ_ALWAYS_INLINE float Get(ezInt32 x, ezInt32 y) const { return m_Depth[WrapY(y) * m_iWidth + WrapX(x)]; }

      /// Bilinear sample between texel (x, y) and (x + 1, y + 1). x and y must already be wrapped into the texture.
      EZ_ALWAYS_INLINE float Sample(ezInt32 x, ezInt32 y, float fTx, float fTy) const
      {
        const ezInt32 x1 = (x + 1 < m_iWidth) ? x + 1 : 0;
        const float* pRow0 = &m_Depth[y * m_iWidth];
        const float* pRow1 = &m_Depth[((y + 1 < m_iHeight) ? y + 1 : 0) * m_iWidth];

        const float d0 = ezMath::Lerp(pRow0[x], pRow0[x1], fTx);
        const float d1 = ezMath::Lerp(pRow1[x], pRow1[x1], fTx);
        return ezMath::Lerp(d0, d1, fTy);
      }

      /// The block that contains texel (x, y), which must already be wrapped into the texture.
      EZ_ALWAYS_INLINE const Block& GetBlock(ezInt32 x, ezInt32 y) const { return m_Blocks[(y >> m_iBlockShift) * m_iNumBlocksX + (x >> m_iBlockShift)]; }

      void Init()
      {
        m_iMaskX = ezMath::IsPowerOf2(static_cast<ezUInt32>(m_iWidth)) ? m_iWidth - 1 : -1;
        m_iMaskY = ezMath::IsPowerOf2(static_cast<ezUInt32>(m_iHeight)) ? m_iHeight - 1 : -1;

        // the blocks need to tile the texture, otherwise every block is a single texel
        m_iBlockShift = (m_iWidth % 8 == 0 && m_iHeight % 8 == 0) ? 3 : 0;
        m_iBlockSize = 1 << m_iBlockShift;
        m_iNumBlocksX = m_iWidth / m_iBlockSize;
        m_iNumBlocksY = m_iHeight / m_iBlockSize;
        m_Blocks.SetCountUninitialized(m_iNumBlocksX * m_iNumBlocksY);

        for (ezInt32 by = 0; by < m_iNumBlocksY; ++by)
        {
          for (ezInt32 bx = 0; bx < m_iNumBlocksX; ++bx)
          {
            Block& block = m_Blocks[by * m_iNumBlocksX + bx];
            block.m_fMinDepth = 1.0f;
            block.m_fMaxDepth = 0.0f;
            block.m_fMaxSlopeX = 0.0f;
            block.m_fMaxSlopeY = 0.0f;

            for (ezInt32 y = by * m_iBlockSize; y <= (by + 1) * m_iBlockSize; ++y)
            {
              for (ezInt32 x = bx * m_iBlockSize; x <= (bx + 1) * m_iBlockSize; ++x)
              {
                const float d = Get(x, y);
                block.m_fMaxDepth = ezMath::Max(block.m_fMaxDepth, d);

                const bool bInsideX = x < (bx + 1) * m_iBlockSize;
                const bool bInsideY = y < (by + 1) * m_iBlockSize;

                if (bInsideX && bInsideY)
                  block.m_fMinDepth = ezMath::Min(block.m_fMinDepth, d);

                if (bInsideX)
                  block.m_fMaxSlopeX = ezMath::Max(block.m_fMaxSlopeX, ezMath::Abs(Get(x + 1, y) - d));

                if (bInsideY)
                  block.m_fMaxSlopeY = ezMath::Max(block.m_fMaxSlopeY, ezMath::Abs(Get(x, y + 1) - d));
              }
            }
          }
        }
      }
    };

    constexpr float s_fStepLength = 0.25f;

    /// Advances the ray march by several steps without sampling. Returns false, if the march ends during these steps.
    ///
    /// The ray depth is accumulated step by step, the same way as when every step is sampled, because exit points close to the surface are sensitive to the rounding.
    /// The break conditions of the ray march only get more strict along the ray, so they don't need to be checked for every skipped step.
    /// If they are already met at the step after the skipped ones, there is no further sample and the march can end right away.
    EZ_ALWAYS_INLINE bool SkipSteps(ezInt32 iNumSteps, float& ref_fWalked, float& ref_fRayDepth, float fDepthPerStep, float fDist, float fSrcDepth, float fMaxDistPerDepth)
    {
      if (iNumSteps <= 0)
        return true;

      // lower bound for the accumulated depth at the step after the skipped ones, the margin covers the rounding of the individual additions
      const float fEndRayDepth = ref_fRayDepth + (iNumSteps + 1) * fDepthPerStep - 1e-5f;
      const float fEndWalked = ref_fWalked + (iNumSteps + 1) * s_fStepLength;

      if (fEndRayDepth >= fSrcDepth || fDist + fEndWalked >= fMaxDistPerDepth * (fSrcDepth - fEndRayDepth))
        return false;

      ref_fWalked += iNumSteps * s_fStepLength; // exact, since multiples of a quarter are representable

      for (ezInt32 i = 0; i < iNumSteps; ++i)
      {
        ref_fRayDepth += fDepthPerStep;
      }

      return true;
    }

    /// Computes the relaxed cone ratio of a single texel.
    ///
    /// A ray is shot from the texel's position on the top plane (depth 0) through every other texel that lies higher than the source texel.
    /// The ray is followed through the solid part of the height field until it leaves it again. The cone (apex at the source texel)
    /// must not contain that exit point, which limits the cone ratio. Since the exit point is always further away than the destination
    /// texel and higher than the source texel, the loops stop as soon as no closer exit point is possible anymore.
    /// Destinations are visited in rings of blocks of increasing distance.
    float ComputeRelaxedConeRatio(const DepthField& field, ezInt32 iSrcX, ezInt32 iSrcY, float fMaxDim)
    {
      const float fSrcDepth = field.Get(iSrcX, iSrcY);

      // cone ratios are stored in [0; 1], 1 means a 45 degree cone (with depth 1 being as deep as the texture is wide)
      float fRatio = 1.0f;

      if (fSrcDepth <= 0.0f)
        return fRatio;

      const ezInt32 iBlockSize = field.m_iBlockSize;
      const ezInt32 iSrcBlockX = iSrcX / iBlockSize;
      const ezInt32 iSrcBlockY = iSrcY / iBlockSize;
      const ezInt32 iMaxRing = static_cast<ezInt32>(fMaxDim) / iBlockSize + 1;

      for (ezInt32 iRing = 0; iRing <= iMaxRing; ++iRing)
      {
        // every texel in this ring of blocks is at least this far away, so it can't produce a smaller ratio
        const float fRingMinDist = (iRing == 0) ? 0.0f : static_cast<float>((iRing - 1) * iBlockSize + 1);
        if (fRingMinDist >= fRatio * fSrcDepth * fMaxDim)
          break;

        // iterate over the square ring of blocks with Chebyshev distance iRing
        const ezInt32 iNumRingBlocks = (iRing == 0) ? 1 : 8 * iRing;
        for (ezInt32 b = 0; b < iNumRingBlocks; ++b)
        {
          ezInt32 bx = 0, by = 0;
          if (iRing > 0)
          {
            const ezInt32 side = b / (2 * iRing);
            const ezInt32 offset = b % (2 * iRing);
            switch (side)
            {
              case 0:
                bx = -iRing + offset;
                by = -iRing;
                break;
              case 1:
                bx = iRing;
                by = -iRing + offset;
                break;
              case 2:
                bx = iRing - offset;
                by = iRing;
                break;
              default:
                bx = -iRing;
                by = iRing - offset;
                break;
            }
          }

          // block position without wrap around, so that distances are measured to the closest repetition of the texture
          const ezInt32 iBlockX = (iSrcBlockX + bx) * iBlockSize;
          const ezInt32 iBlockY = (iSrcBlockY + by) * iBlockSize;

          const ezInt32 iWrappedBlockX = DepthField::Wrap(iSrcBlockX + bx, field.m_iNumBlocksX);
          const ezInt32 iWrappedBlockY = DepthField::Wrap(iSrcBlockY + by, field.m_iNumBlocksY);
          const DepthField::Block& block = field.m_Blocks[iWrappedBlockY * field.m_iNumBlocksX + iWrappedBlockX];

          if (block.m_fMinDepth >= fSrcDepth)
            continue;

          // no texel in this block can produce a smaller ratio
          const float fNearX = static_cast<float>(ezMath::Clamp(iSrcX, iBlockX, iBlockX + iBlockSize - 1) - iSrcX);
          const float fNearY = static_cast<float>(ezMath::Clamp(iSrcY, iBlockY, iBlockY + iBlockSize - 1) - iSrcY);
          if (ezMath::Sqrt(fNearX * fNearX + fNearY * fNearY) / fMaxDim >= fRatio * (fSrcDepth - block.m_fMinDepth))
            continue;

          for (ezInt32 y = iBlockY; y < iBlockY + iBlockSize; ++y)
          {
            for (ezInt32 x = iBlockX; x < iBlockX + iBlockSize; ++x)
            {
              const ezInt32 dx = x - iSrcX;
              const ezInt32 dy = y - iSrcY;

              if (dx == 0 && dy == 0)
                continue;

              const float fDstDepth = field.Get(x, y);

              if (fDstDepth >= fSrcDepth)
                continue;

              const float fDist = ezMath::Sqrt(static_cast<float>(dx * dx + dy * dy));

              // lower bound for the ratio that this destination can produce
              if (fDist / fMaxDim >= fRatio * (fSrcDepth - fDstDepth))
                continue;

              if (fDstDepth <= 0.0f)
              {
                // the ray runs along the top plane, the destination texel is the exit point
                fRatio = ezMath::Min(fRatio, (fDist / fMaxDim) / fSrcDepth);
                continue;
              }

              // walk along the ray in steps of a quarter texel, starting at the destination
              constexpr float fStepLength = s_fStepLength;
              const float fStepX = dx / fDist;
              const float fStepY = dy / fDist;
              const float fInvStepX = (dx != 0) ? 1.0f / fStepX : 0.0f;
              const float fInvStepY = (dy != 0) ? 1.0f / fStepY : 0.0f;
              const float fDepthPerStep = fStepLength * fDstDepth / fDist;

              // how much the ray descends per texel
              const float fRayDescent = fDstDepth / fDist;

              float fWalked = 0.0f;
              float fRayDepth = fDstDepth;

              while (true)
              {
                fWalked += fStepLength;
                fRayDepth += fDepthPerStep;

                // the exit point would be at or below the source texel, which doesn't restrict the cone
                if (fRayDepth >= fSrcDepth)
                  break;

                // an exit point here or further along the ray is further away and deeper, so it can't produce a smaller ratio
                if (fDist + fWalked >= fRatio * fMaxDim * (fSrcDepth - fRayDepth))
                  break;

                const float fSampleX = iSrcX + dx + fStepX * fWalked;
                const float fSampleY = iSrcY + dy + fStepY * fWalked;
                const float fFloorX = ezMath::Floor(fSampleX);
                const float fFloorY = ezMath::Floor(fSampleY);
                const ezInt32 iSampleX = field.WrapX(static_cast<ezInt32>(fFloorX));
                const ezInt32 iSampleY = field.WrapY(static_cast<ezInt32>(fFloorY));
                const DepthField::Block& sampleBlock = field.GetBlock(iSampleX, iSampleY);

                // the number of following steps that sample inside the same block, with a margin of one step for rounding errors
                ezInt32 iStepsInBlock = 0;
                {
                  const float fBlockStartX = fFloorX - static_cast<float>(iSampleX & (iBlockSize - 1));
                  const float fBlockStartY = fFloorY - static_cast<float>(iSampleY & (iBlockSize - 1));

                  float fExitDist = static_cast<float>(iBlockSize) * 2.0f;
                  if (fStepX > 0.0f)
                    fExitDist = ezMath::Min(fExitDist, (fBlockStartX + iBlockSize - fSampleX) * fInvStepX);
                  else if (fStepX < 0.0f)
                    fExitDist = ezMath::Min(fExitDist, (fBlockStartX - fSampleX) * fInvStepX);
                  if (fStepY > 0.0f)
                    fExitDist = ezMath::Min(fExitDist, (fBlockStartY + iBlockSize - fSampleY) * fInvStepY);
                  else if (fStepY < 0.0f)
                    fExitDist = ezMath::Min(fExitDist, (fBlockStartY - fSampleY) * fInvStepY);

                  iStepsInBlock = ezMath::Max(0, static_cast<ezInt32>(fExitDist / fStepLength) - 1);
                }

                // the margins below make sure that skipped steps are not affected by rounding errors in the interpolation
                constexpr float fMargin = 1e-5f;

                // the ray is deeper than any surface depth that a sample in this block could return
                if (fRayDepth > sampleBlock.m_fMaxDepth + fMargin)
                {
                  if (!SkipSteps(iStepsInBlock, fWalked, fRayDepth, fDepthPerStep, fDist, fSrcDepth, fRatio * fMaxDim))
                    break;

                  continue;
                }

                const float fSurfaceDepth = field.Sample(iSampleX, iSampleY, fSampleX - fFloorX, fSampleY - fFloorY);

                if (fSurfaceDepth > fRayDepth)
                {
                  // the ray left the solid part of the height field
                  fRatio = ezMath::Min(fRatio, ((fDist + fWalked) / fMaxDim) / (fSrcDepth - fRayDepth));
                  break;
                }

                // within the block, the surface descends at most this much per texel along the ray,
                // so the distance between the ray and the surface shrinks at most by the difference to the ray's descent
                const float fSurfaceDescent = ezMath::Abs(fStepX) * sampleBlock.m_fMaxSlopeX + ezMath::Abs(fStepY) * sampleBlock.m_fMaxSlopeY;
                const float fShrinkPerStep = (fSurfaceDescent - fRayDescent) * fStepLength;
                const float fDistToSurface = fRayDepth - fSurfaceDepth;

                // the number of following steps that are known to stay inside the solid part
                ezInt32 iSkipSteps = 0;
                if (fShrinkPerStep <= 0.0f)
                {
                  if (fDistToSurface + fShrinkPerStep > fMargin)
                    iSkipSteps = iStepsInBlock;
                }
                else if (fDistToSurface > fMargin)
                {
                  iSkipSteps = ezMath::Min(iStepsInBlock, static_cast<ezInt32>((fDistToSurface - fMargin) / fShrinkPerStep));
                }

                if (!SkipSteps(iSkipSteps, fWalked, fRayDepth, fDepthPerStep, fDist, fSrcDepth, fRatio * fMaxDim))
                  break;
              }
            }
          }
        }
      }

      return fRatio;
    }
  } // namespace ConeMapPrecise

  /// Finds exit points with a local slope test instead of ray marching. Used for ezTexConvUsage::ConeMapFast.
  ///
  /// The slope test uses the larger of the forward and backward differences, so it treats every small dip in a noisy height map
  /// as a point where rays leave the solid part. That makes the cones narrower than with ConeMapPrecise, especially for noisy height maps.
  namespace ConeMapFast
  {
    /// Depth values (0 = top, 1 = deepest) of a height map with wrap around addressing, plus the data needed to find exit points.
    ///
    /// A ray that starts on the top plane leaves the solid part of the height field at a point where the surface descends faster
    /// along the ray direction than the ray itself. The slope of the surface along a direction is estimated per axis from the
    /// differences to both neighbors, using the larger descent of the two. That over-estimates the slope near edges, which makes
    /// the cones narrower, never wider.
    struct DepthField
    {
      ezInt32 m_iWidth = 0;
      ezInt32 m_iHeight = 0;
      ezDynamicArray<float> m_Depth;

      // per texel: largest depth increase per texel when moving in +X / -X / +Y / -Y direction (negative values mean the surface rises)
      ezDynamicArray<float> m_SlopePosX;
      ezDynamicArray<float> m_SlopeNegX;
      ezDynamicArray<float> m_SlopePosY;
      ezDynamicArray<float> m_SlopeNegY;

      /// Blocks of m_iBlockSize x m_iBlockSize texels, to skip regions that can't restrict a cone.
      struct Block
      {
        EZ_DECLARE_POD_TYPE();

        float m_fMinDepth;
        float m_fMaxSlope; ///< upper bound for the slope of any texel in the block, in any direction
      };

      ezInt32 m_iBlockSize = 1;
      ezInt32 m_iNumBlocksX = 0;
      ezInt32 m_iNumBlocksY = 0;
      ezDynamicArray<Block> m_Blocks;

      EZ_ALWAYS_INLINE ezInt32 Index(ezInt32 x, ezInt32 y) const
      {
        x %= m_iWidth;
        y %= m_iHeight;
        x += (x < 0) ? m_iWidth : 0;
        y += (y < 0) ? m_iHeight : 0;
        return y * m_iWidth + x;
      }

      void Init()
      {
        const ezUInt32 uiNumTexels = m_iWidth * m_iHeight;
        m_SlopePosX.SetCountUninitialized(uiNumTexels);
        m_SlopeNegX.SetCountUninitialized(uiNumTexels);
        m_SlopePosY.SetCountUninitialized(uiNumTexels);
        m_SlopeNegY.SetCountUninitialized(uiNumTexels);

        for (ezInt32 y = 0; y < m_iHeight; ++y)
        {
          for (ezInt32 x = 0; x < m_iWidth; ++x)
          {
            const ezInt32 i = y * m_iWidth + x;
            const float d = m_Depth[i];
            const float fNextX = m_Depth[Index(x + 1, y)] - d;
            const float fPrevX = d - m_Depth[Index(x - 1, y)];
            const float fNextY = m_Depth[Index(x, y + 1)] - d;
            const float fPrevY = d - m_Depth[Index(x, y - 1)];

            m_SlopePosX[i] = ezMath::Max(fNextX, fPrevX);
            m_SlopeNegX[i] = -ezMath::Min(fNextX, fPrevX);
            m_SlopePosY[i] = ezMath::Max(fNextY, fPrevY);
            m_SlopeNegY[i] = -ezMath::Min(fNextY, fPrevY);
          }
        }

        // the blocks need to tile the texture, otherwise every block is a single texel
        m_iBlockSize = (m_iWidth % 8 == 0 && m_iHeight % 8 == 0) ? 8 : 1;
        m_iNumBlocksX = m_iWidth / m_iBlockSize;
        m_iNumBlocksY = m_iHeight / m_iBlockSize;
        m_Blocks.SetCountUninitialized(m_iNumBlocksX * m_iNumBlocksY);

        for (ezInt32 by = 0; by < m_iNumBlocksY; ++by)
        {
          for (ezInt32 bx = 0; bx < m_iNumBlocksX; ++bx)
          {
            Block& block = m_Blocks[by * m_iNumBlocksX + bx];
            block.m_fMinDepth = 1.0f;
            block.m_fMaxSlope = 0.0f;

            for (ezInt32 y = by * m_iBlockSize; y < (by + 1) * m_iBlockSize; ++y)
            {
              for (ezInt32 x = bx * m_iBlockSize; x < (bx + 1) * m_iBlockSize; ++x)
              {
                const ezInt32 i = y * m_iWidth + x;
                const float fSlopeX = ezMath::Max(ezMath::Abs(m_SlopePosX[i]), ezMath::Abs(m_SlopeNegX[i]));
                const float fSlopeY = ezMath::Max(ezMath::Abs(m_SlopePosY[i]), ezMath::Abs(m_SlopeNegY[i]));

                block.m_fMinDepth = ezMath::Min(block.m_fMinDepth, m_Depth[i]);
                block.m_fMaxSlope = ezMath::Max(block.m_fMaxSlope, ezMath::Sqrt(fSlopeX * fSlopeX + fSlopeY * fSlopeY));
              }
            }
          }
        }
      }
    };

    /// Computes the relaxed cone ratio of a single texel.
    ///
    /// Rays that start on the top plane above the source texel enter the solid part of the height field and may leave it again
    /// at an exit point. The cone (apex at the source texel) must not contain any such exit point that lies above the source texel,
    /// which limits the cone ratio. Exit points are found with a local test (see DepthField), so no ray has to be marched.
    /// Blocks are visited in rings of increasing distance, until no closer exit point is possible anymore.
    float ComputeRelaxedConeRatio(const DepthField& field, ezInt32 iSrcX, ezInt32 iSrcY, float fMaxDim)
    {
      const float fSrcDepth = field.m_Depth[iSrcY * field.m_iWidth + iSrcX];

      // cone ratios are stored in [0; 1], 1 means a 45 degree cone (with depth 1 being as deep as the texture is wide)
      float fRatio = 1.0f;

      if (fSrcDepth <= 0.0f)
        return fRatio;

      const float fInvMaxDim = 1.0f / fMaxDim;
      const ezInt32 iBlockSize = field.m_iBlockSize;
      const ezInt32 iSrcBlockX = iSrcX / iBlockSize;
      const ezInt32 iSrcBlockY = iSrcY / iBlockSize;
      const ezInt32 iMaxRing = static_cast<ezInt32>(fMaxDim) / iBlockSize + 1;

      for (ezInt32 iRing = 0; iRing <= iMaxRing; ++iRing)
      {
        // every texel in this ring of blocks is at least this far away, so it can't produce a smaller ratio
        const float fRingMinDist = (iRing == 0) ? 0.0f : static_cast<float>((iRing - 1) * iBlockSize + 1);
        if (fRingMinDist * fInvMaxDim >= fRatio * fSrcDepth)
          break;

        // iterate over the square ring of blocks with Chebyshev distance iRing
        const ezInt32 iNumRingBlocks = (iRing == 0) ? 1 : 8 * iRing;
        for (ezInt32 b = 0; b < iNumRingBlocks; ++b)
        {
          ezInt32 bx = 0, by = 0;
          if (iRing > 0)
          {
            const ezInt32 side = b / (2 * iRing);
            const ezInt32 offset = b % (2 * iRing);
            switch (side)
            {
              case 0:
                bx = -iRing + offset;
                by = -iRing;
                break;
              case 1:
                bx = iRing;
                by = -iRing + offset;
                break;
              case 2:
                bx = iRing - offset;
                by = iRing;
                break;
              default:
                bx = -iRing;
                by = iRing - offset;
                break;
            }
          }

          // block position without wrap around, so that distances are measured to the closest repetition of the texture
          const ezInt32 iBlockX = (iSrcBlockX + bx) * iBlockSize;
          const ezInt32 iBlockY = (iSrcBlockY + by) * iBlockSize;

          ezInt32 iWrappedBlockX = (iSrcBlockX + bx) % field.m_iNumBlocksX;
          ezInt32 iWrappedBlockY = (iSrcBlockY + by) % field.m_iNumBlocksY;
          iWrappedBlockX += (iWrappedBlockX < 0) ? field.m_iNumBlocksX : 0;
          iWrappedBlockY += (iWrappedBlockY < 0) ? field.m_iNumBlocksY : 0;
          const DepthField::Block& block = field.m_Blocks[iWrappedBlockY * field.m_iNumBlocksX + iWrappedBlockX];

          if (block.m_fMinDepth >= fSrcDepth)
            continue;

          const float fNearX = static_cast<float>(ezMath::Clamp(iSrcX, iBlockX, iBlockX + iBlockSize - 1) - iSrcX);
          const float fNearY = static_cast<float>(ezMath::Clamp(iSrcY, iBlockY, iBlockY + iBlockSize - 1) - iSrcY);
          const float fFarX = static_cast<float>(ezMath::Max(ezMath::Abs(iBlockX - iSrcX), ezMath::Abs(iBlockX + iBlockSize - 1 - iSrcX)));
          const float fFarY = static_cast<float>(ezMath::Max(ezMath::Abs(iBlockY - iSrcY), ezMath::Abs(iBlockY + iBlockSize - 1 - iSrcY)));
          const float fBlockMinDist = ezMath::Sqrt(fNearX * fNearX + fNearY * fNearY);
          const float fBlockMaxDist = ezMath::Sqrt(fFarX * fFarX + fFarY * fFarY);

          // no texel in this block can produce a smaller ratio
          if (fBlockMinDist * fInvMaxDim >= fRatio * (fSrcDepth - block.m_fMinDepth))
            continue;

          // the surface in this block is too flat to contain an exit point and no texel is on the top plane (see the exit test below)
          if (block.m_fMinDepth > 0.0f && block.m_fMaxSlope * fBlockMaxDist <= block.m_fMinDepth)
            continue;

          for (ezInt32 y = iBlockY; y < iBlockY + iBlockSize; ++y)
          {
            for (ezInt32 x = iBlockX; x < iBlockX + iBlockSize; ++x)
            {
              const ezInt32 dx = x - iSrcX;
              const ezInt32 dy = y - iSrcY;

              if (dx == 0 && dy == 0)
                continue;

              const ezInt32 i = field.Index(x, y);
              const float fDstDepth = field.m_Depth[i];

              if (fDstDepth >= fSrcDepth)
                continue;

              const float fDist = ezMath::Sqrt(static_cast<float>(dx * dx + dy * dy));

              // the ratio this exit point would produce
              const float fDstRatio = (fDist * fInvMaxDim) / (fSrcDepth - fDstDepth);
              if (fDstRatio >= fRatio)
                continue;

              // texels on the top plane always restrict the cone: a ray from the top plane above the source runs along the top plane there.
              // Geometrically it only leaves the solid where the plateau ends, but the extra margin compensates for the bilinear filtering
              // of the cone ratios in the shader, between narrow cones in deep areas and the wide cones of the plateau.
              if (fDstDepth > 0.0f)
              {
                // the ray from the top plane above the source to this texel descends by fDstDepth / fDist per texel,
                // it leaves the solid part here, if the surface descends faster along the ray direction
                const float fSlopeX = (dx >= 0) ? field.m_SlopePosX[i] : field.m_SlopeNegX[i];
                const float fSlopeY = (dy >= 0) ? field.m_SlopePosY[i] : field.m_SlopeNegY[i];
                const float fSurfaceDescent = fSlopeX * ezMath::Abs(static_cast<float>(dx)) + fSlopeY * ezMath::Abs(static_cast<float>(dy)); // = slope * fDist

                if (fSurfaceDescent <= fDstDepth)
                  continue;
              }

              fRatio = fDstRatio;
            }
          }
        }
      }

      return fRatio;
    }
  } // namespace ConeMapFast

  /// Fills R with the height and G with the cone ratio of every texel.
  template <typename DepthField>
  void ComputeConeMap(ezImage& inout_img, float (*computeRatio)(const DepthField&, ezInt32, ezInt32, float))
  {
    DepthField field;
    field.m_iWidth = static_cast<ezInt32>(inout_img.GetWidth());
    field.m_iHeight = static_cast<ezInt32>(inout_img.GetHeight());
    field.m_Depth.SetCountUninitialized(field.m_iWidth * field.m_iHeight);

    ezColor* pPixels = inout_img.GetPixelPointer<ezColor>();

    for (ezUInt32 i = 0; i < field.m_Depth.GetCount(); ++i)
    {
      field.m_Depth[i] = 1.0f - ezMath::Clamp(pPixels[i].r, 0.0f, 1.0f);
    }

    field.Init();

    const float fMaxDim = static_cast<float>(ezMath::Max(field.m_iWidth, field.m_iHeight));

    // the cost per row varies a lot, depending on how deep the texels are
    ezParallelForParams params;
    params.m_uiMaxTasksPerThread = 8;

    ezTaskSystem::ParallelForIndexed(0u, static_cast<ezUInt32>(field.m_iHeight), [&](ezUInt32 uiStartRow, ezUInt32 uiEndRow)
      {
        for (ezUInt32 y = uiStartRow; y < uiEndRow; ++y)
        {
          for (ezInt32 x = 0; x < field.m_iWidth; ++x)
          {
            const ezInt32 i = y * field.m_iWidth + x;
            ezColor& pixel = pPixels[i];
            pixel.r = 1.0f - field.m_Depth[i];
            pixel.g = computeRatio(field, x, static_cast<ezInt32>(y), fMaxDim);
            pixel.b = 0.0f;
            pixel.a = 1.0f;
          }
        }
        //
      },
      "GenerateConeMap", ezTaskNesting::Never, params);
  }
} // namespace

ezResult ezTexConvProcessor::GenerateConeMap(ezImage& img) const
{
  EZ_PROFILE_SCOPE("GenerateConeMap");

  if (img.GetImageFormat() != ezImageFormat::R32G32B32A32_FLOAT || img.GetNumMipLevels() != 1 || img.GetNumArrayIndices() != 1 || img.GetNumFaces() != 1 || img.GetDepth() != 1)
  {
    ezLog::Error("Cone maps can only be generated for single 2D images.");
    return EZ_FAILURE;
  }

  if (m_Descriptor.m_Usage == ezTexConvUsage::ConeMapFast)
  {
    ComputeConeMap<ConeMapFast::DepthField>(img, &ConeMapFast::ComputeRelaxedConeRatio);
  }
  else
  {
    ComputeConeMap<ConeMapPrecise::DepthField>(img, &ConeMapPrecise::ComputeRelaxedConeRatio);
  }

  return EZ_SUCCESS;
}
