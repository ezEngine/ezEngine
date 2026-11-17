#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Texture/TexConv/TexConvProcessor.h>

namespace
{
  /// Depth values (0 = top, 1 = deepest) of a height map, sampled with wrap around addressing.
  struct DepthField
  {
    ezDynamicArray<float> m_Depth;
    ezInt32 m_iWidth = 0;
    ezInt32 m_iHeight = 0;

    EZ_ALWAYS_INLINE float Get(ezInt32 x, ezInt32 y) const
    {
      x %= m_iWidth;
      y %= m_iHeight;
      x += (x < 0) ? m_iWidth : 0;
      y += (y < 0) ? m_iHeight : 0;
      return m_Depth[y * m_iWidth + x];
    }

    float Sample(float fX, float fY) const
    {
      const float fx = ezMath::Floor(fX);
      const float fy = ezMath::Floor(fY);
      const float tx = fX - fx;
      const float ty = fY - fy;
      const ezInt32 x = static_cast<ezInt32>(fx);
      const ezInt32 y = static_cast<ezInt32>(fy);

      const float d0 = ezMath::Lerp(Get(x, y), Get(x + 1, y), tx);
      const float d1 = ezMath::Lerp(Get(x, y + 1), Get(x + 1, y + 1), tx);
      return ezMath::Lerp(d0, d1, ty);
    }
  };

  /// Computes the relaxed cone ratio of a single texel.
  ///
  /// A ray is shot from the texel's position on the top plane (depth 0) through every other texel that lies higher than the source texel.
  /// The ray is followed through the solid part of the height field until it leaves it again. The cone (apex at the source texel)
  /// must not contain that exit point, which limits the cone ratio. Since the exit point is always further away than the destination
  /// texel and higher than the source texel, both loops stop as soon as no closer exit point is possible anymore.
  float ComputeRelaxedConeRatio(const DepthField& field, ezInt32 iSrcX, ezInt32 iSrcY, float fMaxDim)
  {
    const float fSrcDepth = field.Get(iSrcX, iSrcY);

    // cone ratios are stored in [0; 1], 1 means a 45 degree cone (with depth 1 being as deep as the texture is wide)
    float fRatio = 1.0f;

    if (fSrcDepth <= 0.0f)
      return fRatio;

    const ezInt32 iMaxRadius = static_cast<ezInt32>(fMaxDim);

    for (ezInt32 r = 1; r <= iMaxRadius; ++r)
    {
      // every texel on this ring is at least 'r' texels away, so it can't produce a smaller ratio
      if (static_cast<float>(r) >= fRatio * fSrcDepth * fMaxDim)
        break;

      // iterate over the square ring with Chebyshev distance r
      const ezInt32 iNumRingTexels = 8 * r;
      for (ezInt32 i = 0; i < iNumRingTexels; ++i)
      {
        ezInt32 dx, dy;
        const ezInt32 side = i / (2 * r);
        const ezInt32 offset = i % (2 * r);
        switch (side)
        {
          case 0:
            dx = -r + offset;
            dy = -r;
            break;
          case 1:
            dx = r;
            dy = -r + offset;
            break;
          case 2:
            dx = r - offset;
            dy = r;
            break;
          default:
            dx = -r;
            dy = r - offset;
            break;
        }

        const float fDstDepth = field.Get(iSrcX + dx, iSrcY + dy);

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
        constexpr float fStepLength = 0.25f;
        const float fStepX = dx / fDist;
        const float fStepY = dy / fDist;
        const float fDepthPerStep = fStepLength * fDstDepth / fDist;

        float fWalked = 0.0f;
        float fRayDepth = fDstDepth;

        while (true)
        {
          fWalked += fStepLength;
          fRayDepth += fDepthPerStep;

          // the exit point would be at or below the source texel, which doesn't restrict the cone
          if (fRayDepth >= fSrcDepth)
            break;

          // no exit point beyond this one can produce a smaller ratio
          if ((fDist + fWalked) / fMaxDim >= fRatio * fSrcDepth)
            break;

          const float fSurfaceDepth = field.Sample(iSrcX + dx + fStepX * fWalked, iSrcY + dy + fStepY * fWalked);

          if (fSurfaceDepth > fRayDepth)
          {
            // the ray left the solid part of the height field
            fRatio = ezMath::Min(fRatio, ((fDist + fWalked) / fMaxDim) / (fSrcDepth - fRayDepth));
            break;
          }
        }
      }
    }

    return fRatio;
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

  DepthField field;
  field.m_iWidth = static_cast<ezInt32>(img.GetWidth());
  field.m_iHeight = static_cast<ezInt32>(img.GetHeight());
  field.m_Depth.SetCountUninitialized(field.m_iWidth * field.m_iHeight);

  ezColor* pPixels = img.GetPixelPointer<ezColor>();

  for (ezUInt32 i = 0; i < field.m_Depth.GetCount(); ++i)
  {
    field.m_Depth[i] = 1.0f - ezMath::Clamp(pPixels[i].r, 0.0f, 1.0f);
  }

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
          ezColor& pixel = pPixels[y * field.m_iWidth + x];
          pixel.r = 1.0f - field.Get(x, y);
          pixel.g = ComputeRelaxedConeRatio(field, x, static_cast<ezInt32>(y), fMaxDim);
          pixel.b = 0.0f;
          pixel.a = 1.0f;
        }
      }
      //
    },
    "GenerateConeMap", ezTaskNesting::Never, params);

  return EZ_SUCCESS;
}
