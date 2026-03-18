#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <RendererCore/Rasterizer/Generic/OccluderGeneric.h>

void OccluderGeneric::Bake(const ezSimdVec4f* pVertices, ezUInt32 uiNumVertices, const ezSimdVec4f& vRefMin, const ezSimdVec4f& vRefMax)
{
  EZ_ASSERT_DEV(uiNumVertices % 16 == 0, "Vertex count must be a multiple of 16");

  // Simple k-means clustering by normal direction to improve backface culling efficiency
  ezTempHybridArray<ezSimdVec4f, 32> quadNormals;
  for (ezUInt32 i = 0; i < uiNumVertices; i += 4)
  {
    const ezSimdVec4f v0 = pVertices[i + 0];
    const ezSimdVec4f v1 = pVertices[i + 1];
    const ezSimdVec4f v2 = pVertices[i + 2];
    const ezSimdVec4f v3 = pVertices[i + 3];

    // normal(a,b,c) = cross(b-a, c-a)
    const ezSimdVec4f n012 = (v1 - v0).CrossRH(v2 - v0);
    const ezSimdVec4f n023 = (v2 - v0).CrossRH(v3 - v0);
    ezSimdVec4f combined = n012 + n023;
    combined.NormalizeIfNotZero<3>(ezSimdVec4f(1, 0, 0, 0));
    quadNormals.PushBack(combined);
  }

  // 6 axis-aligned initial centroids
  ezTempHybridArray<ezSimdVec4f, 8> centroids;
  centroids.PushBack(ezSimdVec4f(+1, 0, 0, 0));
  centroids.PushBack(ezSimdVec4f(0, +1, 0, 0));
  centroids.PushBack(ezSimdVec4f(0, 0, +1, 0));
  centroids.PushBack(ezSimdVec4f(0, -1, 0, 0));
  centroids.PushBack(ezSimdVec4f(0, 0, -1, 0));
  centroids.PushBack(ezSimdVec4f(-1, 0, 0, 0));

  ezTempHybridArray<ezUInt32, 32> centroidAssignment;
  centroidAssignment.SetCount(uiNumVertices / 4);

  bool bAnyChanged = true;
  for (int iter = 0; iter < 10 && bAnyChanged; ++iter)
  {
    bAnyChanged = false;

    for (ezUInt32 j = 0; j < quadNormals.GetCount(); ++j)
    {
      const ezSimdVec4f n = quadNormals[j];

      float fBestDistance = -ezMath::Infinity<float>();
      ezUInt32 uiBestCentroid = 0;
      for (ezUInt32 k = 0; k < centroids.GetCount(); ++k)
      {
        const float fDistance = centroids[k].Dot<3>(n);
        if (fDistance >= fBestDistance)
        {
          fBestDistance = fDistance;
          uiBestCentroid = k;
        }
      }

      if (centroidAssignment[j] != uiBestCentroid)
      {
        centroidAssignment[j] = uiBestCentroid;
        bAnyChanged = true;
      }
    }

    for (ezUInt32 k = 0; k < centroids.GetCount(); ++k)
    {
      centroids[k] = ezSimdVec4f::MakeZero();
    }

    for (ezUInt32 j = 0; j < quadNormals.GetCount(); ++j)
    {
      const ezUInt32 k = centroidAssignment[j];
      centroids[k] = centroids[k] + quadNormals[j];
    }

    for (ezUInt32 k = 0; k < centroids.GetCount(); ++k)
    {
      centroids[k].NormalizeIfNotZero<3>(ezSimdVec4f(1, 0, 0, 0));
    }
  }

  // Reorder vertices by cluster assignment
  ezTempHybridArray<ezSimdVec4f, 64> orderedVertices;
  for (ezUInt32 k = 0; k < centroids.GetCount(); ++k)
  {
    for (ezUInt32 j = 0; j < uiNumVertices / 4; ++j)
    {
      if (centroidAssignment[j] == k)
      {
        orderedVertices.PushBack(pVertices[4 * j + 0]);
        orderedVertices.PushBack(pVertices[4 * j + 1]);
        orderedVertices.PushBack(pVertices[4 * j + 2]);
        orderedVertices.PushBack(pVertices[4 * j + 3]);
      }
    }
  }

  // Quantize vertices to 11/11/10 bit packed uint32

  const ezSimdVec4f vInvExtents = (vRefMax - vRefMin).GetReciprocal<ezMathAcc::FULL>();

  m_uiPacketCount = 0;
  m_VertexData.SetCountUninitialized(orderedVertices.GetCount());

  // Process 16 vertices (4 quads) at a time
  for (ezUInt32 i = 0; i < orderedVertices.GetCount(); i += 16)
  {
    for (ezUInt32 j = 0; j < 4; ++j)
    {
      // Transform set of 4 vertices (one from each quad) into [0,1] space relative to bounding box
      ezSimdVec4f v0 = (orderedVertices[i + j + 0] - vRefMin).CompMul(vInvExtents);
      ezSimdVec4f v1 = (orderedVertices[i + j + 4] - vRefMin).CompMul(vInvExtents);
      ezSimdVec4f v2 = (orderedVertices[i + j + 8] - vRefMin).CompMul(vInvExtents);
      ezSimdVec4f v3 = (orderedVertices[i + j + 12] - vRefMin).CompMul(vInvExtents);

      // We need x,y,z from each vertex, transposed so we can quantize per-axis
      // v0 = {v0.x, v0.y, v0.z, v0.w}, v1 = {v1.x, v1.y, v1.z, v1.w}, etc.
      // After transpose: v0 = {v0.x, v1.x, v2.x, v3.x} (all X), v1 = all Y, v2 = all Z

      // Manual 4x4 transpose using swizzles
      // Interleave pairs: lo01 = {v0.x, v1.x, v0.y, v1.y}, hi01 = {v0.z, v1.z, v0.w, v1.w}
      const ezSimdVec4f lo01 = v0.GetCombined<ezSwizzle::XXYY>(v1);  // wrong, need interleave
      // Actually, we need the standard 4x4 transpose pattern. Let's just extract and recompose.

      float f[16];
      v0.Store<4>(f + 0);
      v1.Store<4>(f + 4);
      v2.Store<4>(f + 8);
      v3.Store<4>(f + 12);

      // Transposed: X = {f[0], f[4], f[8], f[12]}, Y = {f[1], f[5], f[9], f[13]}, Z = {f[2], f[6], f[10], f[14]}
      ezSimdVec4f vAllX(f[0], f[4], f[8], f[12]);
      ezSimdVec4f vAllY(f[1], f[5], f[9], f[13]);
      ezSimdVec4f vAllZ(f[2], f[6], f[10], f[14]);

      // Scale and add 0.5 for rounding, then truncate to int
      const ezSimdVec4f vHalf(0.5f);
      const ezSimdVec4f vScaleX(2047.0f);
      const ezSimdVec4f vScaleY(2047.0f);
      const ezSimdVec4f vScaleZ(1023.0f);

      ezSimdVec4i X = ezSimdVec4i::Truncate(ezSimdVec4f::MulAdd(vAllX, vScaleX, vHalf)) - ezSimdVec4i(1024);
      ezSimdVec4i Y = ezSimdVec4i::Truncate(ezSimdVec4f::MulAdd(vAllY, vScaleY, vHalf));
      ezSimdVec4i Z = ezSimdVec4i::Truncate(ezSimdVec4f::MulAdd(vAllZ, vScaleZ, vHalf));

      // Pack to 11/11/10 format: X in bits [31:21], Y in bits [20:10], Z in bits [9:0]
      ezSimdVec4i XYZ = (X << 21) | (Y << 10) | Z;

      // Store 4 packed vertices
      XYZ.Store<4>(reinterpret_cast<ezInt32*>(m_VertexData.GetData() + m_uiPacketCount));
      m_uiPacketCount += 4;
    }
  }

  m_vRefMin = vRefMin;
  m_vRefMax = vRefMax;

  // Compute overall bounds from all input vertices
  ezSimdVec4f vMin(ezMath::Infinity<float>());
  ezSimdVec4f vMax(-ezMath::Infinity<float>());

  for (ezUInt32 i = 0; i < uiNumVertices; ++i)
  {
    vMin = vMin.CompMin(pVertices[i]);
    vMax = vMax.CompMax(pVertices[i]);
  }

  // Set W = 1 — expected by frustum culling code
  vMin.SetW(ezSimdFloat(1.0f));
  vMax.SetW(ezSimdFloat(1.0f));

  m_vBoundsMin = vMin;
  m_vBoundsMax = vMax;
  m_vCenter = (vMax + vMin) * ezSimdFloat(0.5f);
}
