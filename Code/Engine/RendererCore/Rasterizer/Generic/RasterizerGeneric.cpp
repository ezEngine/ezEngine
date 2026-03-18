#include <RendererCore/RendererCorePCH.h>

#include <Foundation/SimdMath/SimdVec4i.h>
#include <RendererCore/Rasterizer/Generic/OccluderGeneric.h>
#include <RendererCore/Rasterizer/Generic/RasterizerGeneric.h>

#include <algorithm>
#include <cmath>

#if 0 // Set to 1 to enable near-clip debug output
#  include <cstdio>
#  define NEARCLIP_DEBUG(...) printf(__VA_ARGS__)
#else
#  define NEARCLIP_DEBUG(...)
#endif

static constexpr float s_fMinEdgeOffset = -0.45f;
static const float s_fMaxInvW = ezMath::Sqrt(ezMath::MaxValue<float>());

static constexpr int OFFSET_QUANTIZATION_BITS = 6;
static constexpr int OFFSET_QUANTIZATION_FACTOR = 1 << OFFSET_QUANTIZATION_BITS;

static constexpr int SLOPE_QUANTIZATION_BITS = 6;
static constexpr int SLOPE_QUANTIZATION_FACTOR = 1 << SLOPE_QUANTIZATION_BITS;

enum PrimitiveMode
{
  Culled = 0,
  Triangle0,
  Triangle1,
  ConcaveRight,
  ConcaveLeft,
  ConcaveCenter,
  Convex
};

// clang-format off

static constexpr int s_ModeTable[256] =
  {
    Convex,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    Culled,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Culled,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Culled,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Culled,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Triangle1,
    Culled,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveCenter,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Culled,
    Culled,
    ConcaveRight,
    Culled,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle1,
    Triangle1,
    Triangle1,
    Triangle1,
    Culled,
    Culled,
    Culled,
    Culled,
    Triangle1,
    Triangle1,
    Triangle1,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Culled,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Culled,
    Culled,
    ConcaveRight,
    Culled,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    Culled,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Culled,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Culled,
    ConcaveLeft,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
};
// clang-format on

// ===== Helpers =====

EZ_ALWAYS_INLINE static ezSimdVec4b IsNegative(const ezSimdVec4f& v)
{
  // TODO: replace with v.IsNegative() once that function is added to SimdMath
  return v < ezSimdVec4f::MakeZero();
}

/// Transposes 4 ezSimdVec4f from SoA to AoS:
/// Input: a = {a0,a1,a2,a3}, b = {b0,b1,b2,b3}, c = {c0,c1,c2,c3}, d = {d0,d1,d2,d3}
/// Output: out[0] = {a0,b0,c0,d0}, out[1] = {a1,b1,c1,d1}, out[2] = {a2,b2,c2,d2}, out[3] = {a3,b3,c3,d3}
static void Transpose4x4(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c, const ezSimdVec4f& d, ezSimdVec4f* pOut)
{
  float fa[4], fb[4], fc[4], fd[4];
  a.Store<4>(fa);
  b.Store<4>(fb);
  c.Store<4>(fc);
  d.Store<4>(fd);

  pOut[0] = ezSimdVec4f(fa[0], fb[0], fc[0], fd[0]);
  pOut[1] = ezSimdVec4f(fa[1], fb[1], fc[1], fd[1]);
  pOut[2] = ezSimdVec4f(fa[2], fb[2], fc[2], fd[2]);
  pOut[3] = ezSimdVec4f(fa[3], fb[3], fc[3], fd[3]);
}

static void Transpose4x4i(const ezSimdVec4i& a, const ezSimdVec4i& b, const ezSimdVec4i& c, const ezSimdVec4i& d, ezSimdVec4i* pOut)
{
  ezInt32 fa[4], fb[4], fc[4], fd[4];
  a.Store<4>(fa);
  b.Store<4>(fb);
  c.Store<4>(fc);
  d.Store<4>(fd);

  pOut[0] = ezSimdVec4i(fa[0], fb[0], fc[0], fd[0]);
  pOut[1] = ezSimdVec4i(fa[1], fb[1], fc[1], fd[1]);
  pOut[2] = ezSimdVec4i(fa[2], fb[2], fc[2], fd[2]);
  pOut[3] = ezSimdVec4i(fa[3], fb[3], fc[3], fd[3]);
}

// ===== Static member =====

ezDynamicArray<ezInt64> RasterizerGeneric::s_PrecomputedRasterTables;

// ===== Constructor =====

RasterizerGeneric::RasterizerGeneric(ezUInt32 uiWidth, ezUInt32 uiHeight)
  : m_uiWidth(uiWidth)
  , m_uiHeight(uiHeight)
  , m_uiBlocksX(uiWidth / 8)
  , m_uiBlocksY(uiHeight / 8)
{
  EZ_ASSERT_DEV(uiWidth % 8 == 0 && uiHeight % 8 == 0, "Rasterizer dimensions must be a multiple of 8");

  // 64 floats per 8x8 block
  m_DepthBuffer.SetCount(m_uiBlocksX * m_uiBlocksY * 64);

  // 1 float Hi-Z per block + padding
  m_HiZ.SetCount(m_uiBlocksX * m_uiBlocksY + 8);

  PrecomputeRasterizationTable();
}

// ===== setModelViewProjection / clear =====

void RasterizerGeneric::SetModelViewProjection(const float* pMatrix)
{
  // Load 4x4 matrix rows
  ezSimdVec4f mat0, mat1, mat2, mat3;
  mat0.Load<4>(pMatrix + 0);
  mat1.Load<4>(pMatrix + 4);
  mat2.Load<4>(pMatrix + 8);
  mat3.Load<4>(pMatrix + 12);

  // Transpose to column-major
  float r0[4], r1[4], r2[4], r3[4];
  mat0.Store<4>(r0);
  mat1.Store<4>(r1);
  mat2.Store<4>(r2);
  mat3.Store<4>(r3);

  // Transposed rows → stored as raw
  m_ModelViewProjectionRaw[0] = r0[0]; m_ModelViewProjectionRaw[1] = r1[0]; m_ModelViewProjectionRaw[2] = r2[0]; m_ModelViewProjectionRaw[3] = r3[0];
  m_ModelViewProjectionRaw[4] = r0[1]; m_ModelViewProjectionRaw[5] = r1[1]; m_ModelViewProjectionRaw[6] = r2[1]; m_ModelViewProjectionRaw[7] = r3[1];
  m_ModelViewProjectionRaw[8] = r0[2]; m_ModelViewProjectionRaw[9] = r1[2]; m_ModelViewProjectionRaw[10] = r2[2]; m_ModelViewProjectionRaw[11] = r3[2];
  m_ModelViewProjectionRaw[12] = r0[3]; m_ModelViewProjectionRaw[13] = r1[3]; m_ModelViewProjectionRaw[14] = r2[3]; m_ModelViewProjectionRaw[15] = r3[3];

  // Bake viewport transform into matrix
  const float fHalfW = m_uiWidth * 0.5f - 4.0f;
  const float fHalfH = m_uiHeight * 0.5f - 4.0f;

  // After transpose: mat0/1/2/3 become columns
  ezSimdVec4f col0(m_ModelViewProjectionRaw[0], m_ModelViewProjectionRaw[1], m_ModelViewProjectionRaw[2], m_ModelViewProjectionRaw[3]);
  ezSimdVec4f col1(m_ModelViewProjectionRaw[4], m_ModelViewProjectionRaw[5], m_ModelViewProjectionRaw[6], m_ModelViewProjectionRaw[7]);
  ezSimdVec4f col2(m_ModelViewProjectionRaw[8], m_ModelViewProjectionRaw[9], m_ModelViewProjectionRaw[10], m_ModelViewProjectionRaw[11]);
  ezSimdVec4f col3(m_ModelViewProjectionRaw[12], m_ModelViewProjectionRaw[13], m_ModelViewProjectionRaw[14], m_ModelViewProjectionRaw[15]);

  // Bake viewport: X = (X + W) * halfW, Y = (Y + W) * halfH, Z = (W - Z) * 0.5
  // (The original AVX2 code uses a special float compression bias for 16-bit depth.
  // Since we use 32-bit float depth, we store Z in [0,1] range directly.)
  col0 = (col0 + col3) * ezSimdFloat(fHalfW);
  col1 = (col1 + col3) * ezSimdFloat(fHalfH);
  col2 = (col3 - col2) * ezSimdFloat(0.5f); // Maps depth to [0,1] where 0=far, 1=near

  // Transpose back to row-major storage for the rasterizer
  float c0[4], c1[4], c2[4], c3[4];
  col0.Store<4>(c0);
  col1.Store<4>(c1);
  col2.Store<4>(c2);
  col3.Store<4>(c3);

  // Store as row-major columns (matching original layout)
  m_ModelViewProjection[0] = c0[0]; m_ModelViewProjection[1] = c1[0]; m_ModelViewProjection[2] = c2[0]; m_ModelViewProjection[3] = c3[0];
  m_ModelViewProjection[4] = c0[1]; m_ModelViewProjection[5] = c1[1]; m_ModelViewProjection[6] = c2[1]; m_ModelViewProjection[7] = c3[1];
  m_ModelViewProjection[8] = c0[2]; m_ModelViewProjection[9] = c1[2]; m_ModelViewProjection[10] = c2[2]; m_ModelViewProjection[11] = c3[2];
  m_ModelViewProjection[12] = c0[3]; m_ModelViewProjection[13] = c1[3]; m_ModelViewProjection[14] = c2[3]; m_ModelViewProjection[15] = c3[3];
}

void RasterizerGeneric::Clear()
{
  // Use -1.0f as cleared sentinel. This is distinct from any valid depth (in [0,1])
  // and matches the AVX2 version's use of uint16(1) as a cleared marker.
  for (ezUInt32 i = 0; i < m_HiZ.GetCount(); ++i)
  {
    m_HiZ[i] = -1.0f;
  }

  // Clear depth buffer so uncovered pixels in partial writes
  // don't contain stale values from previous frames.
  ezMemoryUtils::ZeroFill(m_DepthBuffer.GetData(), m_DepthBuffer.GetCount());
}

// ===== Helpers =====

template <bool PossiblyNearClipped>
EZ_ALWAYS_INLINE void RasterizerGeneric::NormalizeEdge(ezSimdVec4f& inout_vNx, ezSimdVec4f& inout_vNy, const ezSimdVec4f& vEdgeFlipMask)
{
  const ezSimdVec4f vInvLen = (inout_vNx.Abs() + inout_vNy.Abs()).GetReciprocal<ezMathAcc::BITS_12>();

  constexpr float fMaxOffset = -s_fMinEdgeOffset;
  ezSimdVec4f vMul(static_cast<float>(OFFSET_QUANTIZATION_FACTOR - 1) / (fMaxOffset - s_fMinEdgeOffset));

  if constexpr (PossiblyNearClipped)
  {
    // Flip the normalization direction when edge needs flipping
    vMul = vMul.FlipSign(IsNegative(vEdgeFlipMask));
  }

  const ezSimdVec4f vScale = vMul.CompMul(vInvLen);
  inout_vNx = inout_vNx.CompMul(vScale);
  inout_vNy = inout_vNy.CompMul(vScale);
}

EZ_ALWAYS_INLINE ezSimdVec4i RasterizerGeneric::QuantizeSlopeLookup(const ezSimdVec4f& vNx, const ezSimdVec4f& vNy)
{
  const ezSimdVec4b yNeg = vNy < ezSimdVec4f::MakeZero();

  // Remap [-1, 1] to [0, SLOPE_QUANTIZATION / 2] — used during table precomputation with unit-length normals
  constexpr float fMul = (SLOPE_QUANTIZATION_FACTOR / 2 - 1) * 0.5f;
  constexpr float fAdd = fMul + 0.5f;

  const ezSimdVec4i quantizedSlope = ezSimdVec4i::Truncate(ezSimdVec4f::MulAdd(vNx, ezSimdVec4f(fMul), ezSimdVec4f(fAdd)));

  // yNeg mask → integer 0 or -1 (all bits set)
  const ezSimdVec4i yNegInt = ezSimdVec4i::Select(yNeg, ezSimdVec4i(-1), ezSimdVec4i(0));

  return ((quantizedSlope << 1) - yNegInt) << OFFSET_QUANTIZATION_BITS;
}

EZ_ALWAYS_INLINE ezSimdVec4i RasterizerGeneric::QuantizeSlopeLookupScaled(const ezSimdVec4f& vNx, const ezSimdVec4f& vNy)
{
  // After NormalizeEdge, normals are scaled by (OFFSET_QUANTIZATION_FACTOR-1)/(maxOffset-minEdgeOffset).
  // Divide out that factor so the slope lookup matches the table computed with unit normals.
  const ezSimdVec4b yNeg = !(vNy > ezSimdVec4f::MakeZero()); // y <= 0

  constexpr float fMaxOffset = -s_fMinEdgeOffset;
  constexpr float fNormFactor = static_cast<float>(OFFSET_QUANTIZATION_FACTOR - 1) / (fMaxOffset - s_fMinEdgeOffset);
  constexpr float fMul = (SLOPE_QUANTIZATION_FACTOR / 2 - 1) * 0.5f / fNormFactor;
  constexpr float fAdd = (SLOPE_QUANTIZATION_FACTOR / 2 - 1) * 0.5f + 0.5f;

  const ezSimdVec4i quantizedSlope = ezSimdVec4i::Truncate(ezSimdVec4f::MulAdd(vNx, ezSimdVec4f(fMul), ezSimdVec4f(fAdd)));

  const ezSimdVec4i yNegInt = ezSimdVec4i::Select(yNeg, ezSimdVec4i(-1), ezSimdVec4i(0));

  return ((quantizedSlope << 1) - yNegInt) << OFFSET_QUANTIZATION_BITS;
}

EZ_ALWAYS_INLINE ezUInt32 RasterizerGeneric::QuantizeOffsetLookup(float fOffset)
{
  constexpr float fMaxOffset = -s_fMinEdgeOffset;
  constexpr float fMul = static_cast<float>(OFFSET_QUANTIZATION_FACTOR - 1) / (fMaxOffset - s_fMinEdgeOffset);
  constexpr float fAdd = 0.5f - s_fMinEdgeOffset * fMul;

  const float fLookup = fOffset * fMul + fAdd;
  return ezMath::Clamp(static_cast<ezInt32>(fLookup), 0, OFFSET_QUANTIZATION_FACTOR - 1);
}

void RasterizerGeneric::PrecomputeRasterizationTable()
{
  if (!s_PrecomputedRasterTables.IsEmpty())
    return;

  const ezUInt32 uiAngularResolution = 2000;
  const ezUInt32 uiOffsetResolution = 2000;

  s_PrecomputedRasterTables.SetCount(OFFSET_QUANTIZATION_FACTOR * SLOPE_QUANTIZATION_FACTOR);

  for (ezUInt32 i = 0; i < uiAngularResolution; ++i)
  {
    const float fAngle = -0.1f + 6.4f * float(i) / (uiAngularResolution - 1);

    float fNx = ezMath::Cos(ezAngle::MakeFromRadian(fAngle));
    float fNy = ezMath::Sin(ezAngle::MakeFromRadian(fAngle));
    const float fLen = 1.0f / (ezMath::Abs(fNx) + ezMath::Abs(fNy));

    fNx *= fLen;
    fNy *= fLen;

    const ezSimdVec4i slopeLookupVec = QuantizeSlopeLookup(ezSimdVec4f(fNx), ezSimdVec4f(fNy));
    const ezUInt32 uiSlopeLookup = static_cast<ezUInt32>(slopeLookupVec.GetComponent<0>());

    for (ezUInt32 j = 0; j < uiOffsetResolution; ++j)
    {
      const float fOffset = -0.6f + 1.2f * float(j) / (uiAngularResolution - 1);

      const ezUInt32 uiOffsetLookup = QuantizeOffsetLookup(fOffset);
      const ezUInt32 uiLookup = uiSlopeLookup | uiOffsetLookup;

      ezUInt64 block = 0;

      for (int x = 0; x < 8; ++x)
      {
        for (int y = 0; y < 8; ++y)
        {
          const float fEdgeDistance = fOffset + (x - 3.5f) / 8.0f * fNx + (y - 3.5f) / 8.0f * fNy;
          if (fEdgeDistance <= 0.0f)
          {
            const ezUInt32 uiBitIndex = 8 * x + y;
            block |= ezUInt64(1) << uiBitIndex;
          }
        }
      }

      s_PrecomputedRasterTables[uiLookup] |= static_cast<ezInt64>(block);
    }
  }
}

// ===== rasterize =====

template <bool PossiblyNearClipped>
void RasterizerGeneric::Rasterize(const OccluderGeneric& occluder)
{
  // BISECT flags — set individual flags to false to disable near-clip-specific code
  
  
  
  
  
  
  
  
  const ezUInt32* pVertexData = occluder.m_VertexData.GetData();
  const ezUInt32 uiPacketCount = occluder.m_uiPacketCount;

  const ezSimdVec4i maskY(2047 << 10);
  const ezSimdVec4i maskZ(1023);

  // Load model-view-projection matrix
  ezSimdVec4f mat0, mat1, mat2, mat3;
  mat0.Load<4>(m_ModelViewProjection + 0);
  mat1.Load<4>(m_ModelViewProjection + 4);
  mat2.Load<4>(m_ModelViewProjection + 8);
  mat3.Load<4>(m_ModelViewProjection + 12);

  const ezSimdVec4f vBoundsMin = occluder.m_vRefMin;
  const ezSimdVec4f vBoundsExtents = occluder.m_vRefMax - vBoundsMin;

  // Bake integer→bounding box transform into matrix
  mat3 = ezSimdVec4f::MulAdd(mat0, ezSimdVec4f(vBoundsMin.GetComponent<0>()),
    ezSimdVec4f::MulAdd(mat1, ezSimdVec4f(vBoundsMin.GetComponent<1>()),
      ezSimdVec4f::MulAdd(mat2, ezSimdVec4f(vBoundsMin.GetComponent<2>()), mat3)));

  mat0 = mat0.CompMul(ezSimdVec4f(vBoundsExtents.GetComponent<0>())) * ezSimdFloat(1.0f / (2047ull << 21));
  mat1 = mat1.CompMul(ezSimdVec4f(vBoundsExtents.GetComponent<1>())) * ezSimdFloat(1.0f / (2047 << 10));
  mat2 = mat2.CompMul(ezSimdVec4f(vBoundsExtents.GetComponent<2>())) * ezSimdFloat(1.0f / 1023.0f);

  // Bias X back into positive range
  mat3 = ezSimdVec4f::MulAdd(mat0, ezSimdVec4f(static_cast<float>(1024ull << 21)), mat3);

  // Skew projection
  mat1 = mat1 - mat0;
  mat2 = mat2 - mat0;

  // Transpose to get column vectors
  {
    float m0[4], m1[4], m2[4], m3[4];
    mat0.Store<4>(m0);
    mat1.Store<4>(m1);
    mat2.Store<4>(m2);
    mat3.Store<4>(m3);

    mat0 = ezSimdVec4f(m0[0], m1[0], m2[0], m3[0]);
    mat1 = ezSimdVec4f(m0[1], m1[1], m2[1], m3[1]);
    mat2 = ezSimdVec4f(m0[2], m1[2], m2[2], m3[2]);
    mat3 = ezSimdVec4f(m0[3], m1[3], m2[3], m3[3]);
  }

  // Compute depth coefficients: Z = c0 * W + c1 (linear relationship between Z and W)
  float fC0, fC1;
  {
    const float za = mat2.GetComponent<3>();
    const float zb = mat2.Dot<4>(ezSimdVec4f(static_cast<float>(1 << 21), static_cast<float>(1 << 10), 1.0f, 1.0f));
    const float wa = mat3.GetComponent<3>();
    const float wb = mat3.Dot<4>(ezSimdVec4f(static_cast<float>(1 << 21), static_cast<float>(1 << 10), 1.0f, 1.0f));

    fC0 = (za - zb) / (wa - wb);
    fC1 = za - fC0 * wa;
  }

  // Process 4 primitives at a time. Each iteration reads 16 uint32 values (4 vertices × 4 quads).
  const ezUInt32 uiGroupCount = uiPacketCount / 4;
  NEARCLIP_DEBUG("Rasterize<%s>: packetCount=%u groupCount=%u\n", PossiblyNearClipped?"true":"false", uiPacketCount, uiGroupCount);
  for (ezUInt32 packetIdx = 0; packetIdx < uiGroupCount; packetIdx += 4)
  {
    // Load 4 packets (16 uint32 values = 4 vertices × 4 primitives)
    ezSimdVec4i I0, I1, I2, I3;
    I0.Load<4>(reinterpret_cast<const ezInt32*>(pVertexData + packetIdx * 4 + 0));
    I1.Load<4>(reinterpret_cast<const ezInt32*>(pVertexData + packetIdx * 4 + 4));
    I2.Load<4>(reinterpret_cast<const ezInt32*>(pVertexData + packetIdx * 4 + 8));
    I3.Load<4>(reinterpret_cast<const ezInt32*>(pVertexData + packetIdx * 4 + 12));

    // Extract packed coordinates and convert to float
    const ezSimdVec4f Xf0 = I0.ToFloat(); // X is in bits[31:21] — full int includes Y+Z bits, but those will be compensated by skewing
    const ezSimdVec4f Xf1 = I1.ToFloat();
    const ezSimdVec4f Xf2 = I2.ToFloat();
    const ezSimdVec4f Xf3 = I3.ToFloat();

    const ezSimdVec4f Yf0 = (I0 & maskY).ToFloat();
    const ezSimdVec4f Yf1 = (I1 & maskY).ToFloat();
    const ezSimdVec4f Yf2 = (I2 & maskY).ToFloat();
    const ezSimdVec4f Yf3 = (I3 & maskY).ToFloat();

    const ezSimdVec4f Zf0 = (I0 & maskZ).ToFloat();
    const ezSimdVec4f Zf1 = (I1 & maskZ).ToFloat();
    const ezSimdVec4f Zf2 = (I2 & maskZ).ToFloat();
    const ezSimdVec4f Zf3 = (I3 & maskZ).ToFloat();

    // Transform: X_screen = Xf * mat0.x + Yf * mat0.y + Zf * mat0.z + mat0.w
    const ezSimdVec4f mat00(mat0.GetComponent<0>());
    const ezSimdVec4f mat01(mat0.GetComponent<1>());
    const ezSimdVec4f mat02(mat0.GetComponent<2>());
    const ezSimdVec4f mat03(mat0.GetComponent<3>());

    const ezSimdVec4f X0 = ezSimdVec4f::MulAdd(Xf0, mat00, ezSimdVec4f::MulAdd(Yf0, mat01, ezSimdVec4f::MulAdd(Zf0, mat02, mat03)));
    const ezSimdVec4f X1 = ezSimdVec4f::MulAdd(Xf1, mat00, ezSimdVec4f::MulAdd(Yf1, mat01, ezSimdVec4f::MulAdd(Zf1, mat02, mat03)));
    const ezSimdVec4f X2 = ezSimdVec4f::MulAdd(Xf2, mat00, ezSimdVec4f::MulAdd(Yf2, mat01, ezSimdVec4f::MulAdd(Zf2, mat02, mat03)));
    const ezSimdVec4f X3 = ezSimdVec4f::MulAdd(Xf3, mat00, ezSimdVec4f::MulAdd(Yf3, mat01, ezSimdVec4f::MulAdd(Zf3, mat02, mat03)));

    const ezSimdVec4f mat10(mat1.GetComponent<0>());
    const ezSimdVec4f mat11(mat1.GetComponent<1>());
    const ezSimdVec4f mat12(mat1.GetComponent<2>());
    const ezSimdVec4f mat13(mat1.GetComponent<3>());

    const ezSimdVec4f Y0 = ezSimdVec4f::MulAdd(Xf0, mat10, ezSimdVec4f::MulAdd(Yf0, mat11, ezSimdVec4f::MulAdd(Zf0, mat12, mat13)));
    const ezSimdVec4f Y1 = ezSimdVec4f::MulAdd(Xf1, mat10, ezSimdVec4f::MulAdd(Yf1, mat11, ezSimdVec4f::MulAdd(Zf1, mat12, mat13)));
    const ezSimdVec4f Y2 = ezSimdVec4f::MulAdd(Xf2, mat10, ezSimdVec4f::MulAdd(Yf2, mat11, ezSimdVec4f::MulAdd(Zf2, mat12, mat13)));
    const ezSimdVec4f Y3 = ezSimdVec4f::MulAdd(Xf3, mat10, ezSimdVec4f::MulAdd(Yf3, mat11, ezSimdVec4f::MulAdd(Zf3, mat12, mat13)));

    const ezSimdVec4f mat30(mat3.GetComponent<0>());
    const ezSimdVec4f mat31(mat3.GetComponent<1>());
    const ezSimdVec4f mat32(mat3.GetComponent<2>());
    const ezSimdVec4f mat33(mat3.GetComponent<3>());

    const ezSimdVec4f W0 = ezSimdVec4f::MulAdd(Xf0, mat30, ezSimdVec4f::MulAdd(Yf0, mat31, ezSimdVec4f::MulAdd(Zf0, mat32, mat33)));
    const ezSimdVec4f W1 = ezSimdVec4f::MulAdd(Xf1, mat30, ezSimdVec4f::MulAdd(Yf1, mat31, ezSimdVec4f::MulAdd(Zf1, mat32, mat33)));
    const ezSimdVec4f W2 = ezSimdVec4f::MulAdd(Xf2, mat30, ezSimdVec4f::MulAdd(Yf2, mat31, ezSimdVec4f::MulAdd(Zf2, mat32, mat33)));
    const ezSimdVec4f W3 = ezSimdVec4f::MulAdd(Xf3, mat30, ezSimdVec4f::MulAdd(Yf3, mat31, ezSimdVec4f::MulAdd(Zf3, mat32, mat33)));

    // Compute inverse W
    ezSimdVec4f invW0, invW1, invW2, invW3;
    if constexpr (PossiblyNearClipped)
    {
      // Clamp invW to [-maxInvW, +maxInvW] to prevent overflow, matching the AVX2 version.
      const ezSimdVec4f vLowerBound(-s_fMaxInvW);
      const ezSimdVec4f vUpperBound(s_fMaxInvW);
      invW0 = W0.GetReciprocal<ezMathAcc::BITS_12>().CompMax(vLowerBound).CompMin(vUpperBound);
      invW1 = W1.GetReciprocal<ezMathAcc::BITS_12>().CompMax(vLowerBound).CompMin(vUpperBound);
      invW2 = W2.GetReciprocal<ezMathAcc::BITS_12>().CompMax(vLowerBound).CompMin(vUpperBound);
      invW3 = W3.GetReciprocal<ezMathAcc::BITS_12>().CompMax(vLowerBound).CompMin(vUpperBound);
    }
    else
    {
      invW0 = W0.GetReciprocal<ezMathAcc::BITS_12>();
      invW1 = W1.GetReciprocal<ezMathAcc::BITS_12>();
      invW2 = W2.GetReciprocal<ezMathAcc::BITS_12>();
      invW3 = W3.GetReciprocal<ezMathAcc::BITS_12>();
    }

    // Perspective divide, round to sub-pixel grid, scale to block coords
    const ezSimdVec4f vEighth(0.125f);
    const ezSimdVec4f x0 = (X0.CompMul(invW0)).Round().CompMul(vEighth);
    const ezSimdVec4f x1 = (X1.CompMul(invW1)).Round().CompMul(vEighth);
    const ezSimdVec4f x2 = (X2.CompMul(invW2)).Round().CompMul(vEighth);
    const ezSimdVec4f x3 = (X3.CompMul(invW3)).Round().CompMul(vEighth);

    const ezSimdVec4f y0 = (Y0.CompMul(invW0)).Round().CompMul(vEighth);
    const ezSimdVec4f y1 = (Y1.CompMul(invW1)).Round().CompMul(vEighth);
    const ezSimdVec4f y2 = (Y2.CompMul(invW2)).Round().CompMul(vEighth);
    const ezSimdVec4f y3 = (Y3.CompMul(invW3)).Round().CompMul(vEighth);

    // Edge normals
    ezSimdVec4f edgeNormalsX0 = y1 - y0;
    ezSimdVec4f edgeNormalsX1 = y2 - y1;
    ezSimdVec4f edgeNormalsX2 = y3 - y2;
    ezSimdVec4f edgeNormalsX3 = y0 - y3;

    ezSimdVec4f edgeNormalsY0 = x0 - x1;
    ezSimdVec4f edgeNormalsY1 = x1 - x2;
    ezSimdVec4f edgeNormalsY2 = x2 - x3;
    ezSimdVec4f edgeNormalsY3 = x3 - x0;

    // Signed areas
    const ezSimdVec4f area0 = ezSimdVec4f::MulSub(edgeNormalsX0, edgeNormalsY1, edgeNormalsX1.CompMul(edgeNormalsY0));
    const ezSimdVec4f area1 = ezSimdVec4f::MulSub(edgeNormalsX1, edgeNormalsY2, edgeNormalsX2.CompMul(edgeNormalsY1));
    const ezSimdVec4f area2 = ezSimdVec4f::MulSub(edgeNormalsX2, edgeNormalsY3, edgeNormalsX3.CompMul(edgeNormalsY2));
    const ezSimdVec4f area3 = (area0 + area2) - area1;

    // W sign extraction for near-clip handling
    ezSimdVec4b wNeg0, wNeg1, wNeg2, wNeg3;
    if constexpr (PossiblyNearClipped)
    {
      wNeg0 = IsNegative(invW0);
      wNeg1 = IsNegative(invW1);
      wNeg2 = IsNegative(invW2);
      wNeg3 = IsNegative(invW3);
    }

    // Compute area signs and build config
    ezSimdVec4b areaSign0, areaSign1, areaSign2, areaSign3;
    if constexpr (PossiblyNearClipped)
    {
      // Flip area signs for near-clipped vertices
      const ezSimdVec4f wSign0 = invW0.FlipSign(ezSimdVec4b(false)); // Extract sign as ±0
      const ezSimdVec4f wSign1 = invW1.FlipSign(ezSimdVec4b(false));
      const ezSimdVec4f wSign2 = invW2.FlipSign(ezSimdVec4b(false));
      const ezSimdVec4f wSign3 = invW3.FlipSign(ezSimdVec4b(false));

      // Use bitcasts and XOR for sign flipping — the original code does:
      //   areaSign0 = (area0 XOR wSign0 XOR wSign1 XOR wSign2) <= 0
      // We need to replicate this sign-manipulation trick.
      // For the portable path, we compute the actual flipped areas:
      ezSimdVec4f flippedArea0 = area0;
      flippedArea0 = flippedArea0.FlipSign(wNeg0);
      flippedArea0 = flippedArea0.FlipSign(wNeg1);
      flippedArea0 = flippedArea0.FlipSign(wNeg2);
      areaSign0 = !(flippedArea0 > ezSimdVec4f::MakeZero()); // area <= 0

      ezSimdVec4f flippedArea1 = area1;
      flippedArea1 = flippedArea1.FlipSign(wNeg1);
      flippedArea1 = flippedArea1.FlipSign(wNeg2);
      flippedArea1 = flippedArea1.FlipSign(wNeg3);
      areaSign1 = !(flippedArea1 > ezSimdVec4f::MakeZero());

      ezSimdVec4f flippedArea2 = area2;
      flippedArea2 = flippedArea2.FlipSign(wNeg0);
      flippedArea2 = flippedArea2.FlipSign(wNeg2);
      flippedArea2 = flippedArea2.FlipSign(wNeg3);
      areaSign2 = !(flippedArea2 > ezSimdVec4f::MakeZero());

      ezSimdVec4f flippedArea3 = area3;
      flippedArea3 = flippedArea3.FlipSign(wNeg1);
      flippedArea3 = flippedArea3.FlipSign(wNeg0);
      flippedArea3 = flippedArea3.FlipSign(wNeg3);
      areaSign3 = !(flippedArea3 > ezSimdVec4f::MakeZero());
    }
    else
    {
      areaSign0 = !(area0 > ezSimdVec4f::MakeZero());
      areaSign1 = !(area1 > ezSimdVec4f::MakeZero());
      areaSign2 = !(area2 > ezSimdVec4f::MakeZero());
      areaSign3 = !(area3 > ezSimdVec4f::MakeZero());
    }

    // Build 4-bit config from area signs
    const ezUInt32 sign0Mask = areaSign0.GetMoveMask();
    const ezUInt32 sign1Mask = areaSign1.GetMoveMask();
    const ezUInt32 sign2Mask = areaSign2.GetMoveMask();
    const ezUInt32 sign3Mask = areaSign3.GetMoveMask();

    // Build per-primitive config (scalar gather from mode table)
    ezInt32 modes[4];
    for (int p = 0; p < 4; ++p)
    {
      ezUInt32 config = 0;
      if (sign0Mask & (1u << p)) config |= 1;
      if (sign1Mask & (1u << p)) config |= 2;
      if (sign2Mask & (1u << p)) config |= 4;
      if (sign3Mask & (1u << p)) config |= 8;

      if constexpr (PossiblyNearClipped)
      {
        const ezUInt32 wNeg0Mask = wNeg0.GetMoveMask();
        const ezUInt32 wNeg1Mask = wNeg1.GetMoveMask();
        const ezUInt32 wNeg2Mask = wNeg2.GetMoveMask();
        const ezUInt32 wNeg3Mask = wNeg3.GetMoveMask();
        if (wNeg0Mask & (1u << p)) config |= 16;
        if (wNeg1Mask & (1u << p)) config |= 32;
        if (wNeg2Mask & (1u << p)) config |= 64;
        if (wNeg3Mask & (1u << p)) config |= 128;
      }

      modes[p] = s_ModeTable[config];

      if constexpr (PossiblyNearClipped)
      {
        float px0[4], py0[4], px1[4], py1[4], px2[4], py2[4], px3[4], py3[4];
        x0.Store<4>(px0); y0.Store<4>(py0);
        x1.Store<4>(px1); y1.Store<4>(py1);
        x2.Store<4>(px2); y2.Store<4>(py2);
        x3.Store<4>(px3); y3.Store<4>(py3);

        float pw0[4], pw1[4], pw2[4], pw3[4];
        W0.Store<4>(pw0); W1.Store<4>(pw1); W2.Store<4>(pw2); W3.Store<4>(pw3);

        float fa0[4], fa1[4], fa2[4], fa3[4];
        area0.Store<4>(fa0); area1.Store<4>(fa1); area2.Store<4>(fa2); area3.Store<4>(fa3);

        NEARCLIP_DEBUG("  prim[%d] config=%u mode=%d areas=(%.2f,%.2f,%.2f,%.2f) W=(%.3f,%.3f,%.3f,%.3f)\n",
          p, config, modes[p], fa0[p], fa1[p], fa2[p], fa3[p], pw0[p], pw1[p], pw2[p], pw3[p]);
        NEARCLIP_DEBUG("    v0=(%.1f,%.1f) v1=(%.1f,%.1f) v2=(%.1f,%.1f) v3=(%.1f,%.1f)\n",
          px0[p], py0[p], px1[p], py1[p], px2[p], py2[p], px3[p], py3[p]);

        // Also dump the raw packed vertex data for this primitive
        {
          ezInt32 i0[4], i1[4], i2[4], i3[4];
          I0.Store<4>(i0); I1.Store<4>(i1); I2.Store<4>(i2); I3.Store<4>(i3);

          // Decode packed: X = val >> 21 (signed), Y = (val >> 10) & 2047, Z = val & 1023
          auto decode = [](ezInt32 packed) {
            int x = packed >> 21; // arithmetic shift preserves sign
            int y = (packed >> 10) & 2047;
            int z = packed & 1023;
            return ezVec3((float)x, (float)y, (float)z);
          };

          ezVec3 d0 = decode(i0[p]), d1 = decode(i1[p]), d2 = decode(i2[p]), d3 = decode(i3[p]);
          NEARCLIP_DEBUG("    decoded v0=(%d,%d,%d) v1=(%d,%d,%d) v2=(%d,%d,%d) v3=(%d,%d,%d)\n",
            (int)d0.x, (int)d0.y, (int)d0.z, (int)d1.x, (int)d1.y, (int)d1.z,
            (int)d2.x, (int)d2.y, (int)d2.z, (int)d3.x, (int)d3.y, (int)d3.z);
        }
      }
    }

    ezSimdVec4i modesVec;
    modesVec.Load<4>(modes);

    const ezSimdVec4b primitiveValid = modesVec > ezSimdVec4i(0);
    ezUInt32 validMask = primitiveValid.GetMoveMask();

    if (validMask == 0)
      continue;

    // Bounding box computation
    ezSimdVec4f minFx, minFy, maxFx, maxFy;

    if constexpr (PossiblyNearClipped)
    {
      const ezSimdVec4f vInfP(10000.0f);
      const ezSimdVec4f vInfN(-10000.0f);

      // Positive-W vertices min/max
      const ezSimdVec4f minPx = ezSimdVec4f::Select(wNeg0, vInfP, x0).CompMin(ezSimdVec4f::Select(wNeg1, vInfP, x1))
                                  .CompMin(ezSimdVec4f::Select(wNeg2, vInfP, x2).CompMin(ezSimdVec4f::Select(wNeg3, vInfP, x3)));
      const ezSimdVec4f minPy = ezSimdVec4f::Select(wNeg0, vInfP, y0).CompMin(ezSimdVec4f::Select(wNeg1, vInfP, y1))
                                  .CompMin(ezSimdVec4f::Select(wNeg2, vInfP, y2).CompMin(ezSimdVec4f::Select(wNeg3, vInfP, y3)));
      const ezSimdVec4f maxPx = ezSimdVec4f::Select(wNeg0, vInfN, x0).CompMax(ezSimdVec4f::Select(wNeg1, vInfN, x1))
                                  .CompMax(ezSimdVec4f::Select(wNeg2, vInfN, x2).CompMax(ezSimdVec4f::Select(wNeg3, vInfN, x3)));
      const ezSimdVec4f maxPy = ezSimdVec4f::Select(wNeg0, vInfN, y0).CompMax(ezSimdVec4f::Select(wNeg1, vInfN, y1))
                                  .CompMax(ezSimdVec4f::Select(wNeg2, vInfN, y2).CompMax(ezSimdVec4f::Select(wNeg3, vInfN, y3)));

      // Negative-W vertices min/max
      const ezSimdVec4f minNx = ezSimdVec4f::Select(wNeg0, x0, vInfP).CompMin(ezSimdVec4f::Select(wNeg1, x1, vInfP))
                                  .CompMin(ezSimdVec4f::Select(wNeg2, x2, vInfP).CompMin(ezSimdVec4f::Select(wNeg3, x3, vInfP)));
      const ezSimdVec4f minNy = ezSimdVec4f::Select(wNeg0, y0, vInfP).CompMin(ezSimdVec4f::Select(wNeg1, y1, vInfP))
                                  .CompMin(ezSimdVec4f::Select(wNeg2, y2, vInfP).CompMin(ezSimdVec4f::Select(wNeg3, y3, vInfP)));
      const ezSimdVec4f maxNx = ezSimdVec4f::Select(wNeg0, x0, vInfN).CompMax(ezSimdVec4f::Select(wNeg1, x1, vInfN))
                                  .CompMax(ezSimdVec4f::Select(wNeg2, x2, vInfN).CompMax(ezSimdVec4f::Select(wNeg3, x3, vInfN)));
      const ezSimdVec4f maxNy = ezSimdVec4f::Select(wNeg0, y0, vInfN).CompMax(ezSimdVec4f::Select(wNeg1, y1, vInfN))
                                  .CompMax(ezSimdVec4f::Select(wNeg2, y2, vInfN).CompMax(ezSimdVec4f::Select(wNeg3, y3, vInfN)));

      // Merge positive and negative intervals
      const ezSimdVec4f incAx = ezSimdVec4f::Select(maxNx > minPx, vInfN, minPx);
      const ezSimdVec4f incAy = ezSimdVec4f::Select(maxNy > minPy, vInfN, minPy);
      const ezSimdVec4f incBx = ezSimdVec4f::Select(maxPx > minNx, vInfP, maxPx);
      const ezSimdVec4f incBy = ezSimdVec4f::Select(maxPy > minNy, vInfP, maxPy);

      minFx = incAx.CompMin(incBx);
      minFy = incAy.CompMin(incBy);
      maxFx = incAx.CompMax(incBx);
      maxFy = incAy.CompMax(incBy);
    }
    else
    {
      minFx = x0.CompMin(x1).CompMin(x2.CompMin(x3));
      minFy = y0.CompMin(y1).CompMin(y2.CompMin(y3));
      maxFx = x0.CompMax(x1).CompMax(x2.CompMax(x3));
      maxFy = y0.CompMax(y1).CompMax(y2.CompMax(y3));
    }

    // Clamp and convert to block coordinates
    const ezSimdVec4i minX = ezSimdVec4i::Truncate(minFx + ezSimdVec4f(4.9999f / 8.0f)).CompMax(ezSimdVec4i(0));
    const ezSimdVec4i minY = ezSimdVec4i::Truncate(minFy + ezSimdVec4f(4.9999f / 8.0f)).CompMax(ezSimdVec4i(0));
    const ezSimdVec4i maxX = ezSimdVec4i::Truncate(maxFx + ezSimdVec4f(11.0f / 8.0f)).CompMin(ezSimdVec4i(m_uiBlocksX));
    const ezSimdVec4i maxY = ezSimdVec4i::Truncate(maxFy + ezSimdVec4f(11.0f / 8.0f)).CompMin(ezSimdVec4i(m_uiBlocksY));

    // Check frustum overlap
    const ezSimdVec4b inFrustum = (maxX > minX) && (maxY > minY);
    validMask &= (inFrustum && primitiveValid).GetMoveMask();

    if (validMask == 0)
      continue;

    const ezSimdVec4i rangeX = maxX - minX;
    const ezSimdVec4i rangeY = maxY - minY;

    // Compute depth values
    const ezSimdVec4f vC0(fC0);
    const ezSimdVec4f vC1(fC1);
    const ezSimdVec4f z0 = ezSimdVec4f::MulAdd(invW0, vC1, vC0);
    const ezSimdVec4f z1 = ezSimdVec4f::MulAdd(invW1, vC1, vC0);
    const ezSimdVec4f z2 = ezSimdVec4f::MulAdd(invW2, vC1, vC0);
    const ezSimdVec4f z3 = ezSimdVec4f::MulAdd(invW3, vC1, vC0);

    ezSimdVec4f maxZ = z0.CompMax(z1).CompMax(z2.CompMax(z3));

    if constexpr (PossiblyNearClipped)
    {
      const ezSimdVec4b anyNeg = wNeg0 || wNeg1 || wNeg2 || wNeg3;
      maxZ = ezSimdVec4f::Select(anyNeg, ezSimdVec4f(1.0f), maxZ);
    }

    // Select triangle area for depth gradient
    const ezSimdVec4b greaterArea = area0.Abs() < area2.Abs();
    const ezSimdVec4b modeTriangle0 = modesVec == ezSimdVec4i(Triangle0);
    const ezSimdVec4b modeTriangle1 = modesVec == ezSimdVec4i(Triangle1);
    const ezSimdVec4b useArea2 = ezSimdVec4b::Select(modeTriangle0, ezSimdVec4b(false), modeTriangle1 || greaterArea);

    ezSimdVec4f invArea;
    if constexpr (PossiblyNearClipped)
    {
      invArea = ezSimdVec4f::Select(useArea2, area2, area0).GetReciprocal<ezMathAcc::FULL>();
    }
    else
    {
      invArea = ezSimdVec4f::Select(useArea2, area2, area0).GetReciprocal<ezMathAcc::BITS_12>();
    }

    // Depth gradients
    const ezSimdVec4f z12 = z1 - z2;
    const ezSimdVec4f z20 = z2 - z0;
    const ezSimdVec4f z30 = z3 - z0;

    const ezSimdVec4f edgeNormalsX4 = y0 - y2;
    const ezSimdVec4f edgeNormalsY4 = x2 - x0;

    // AVX2 formula: fnmadd(z20, enX3, z30*enX4) = z30*enX4 - z20*enX3
    const ezSimdVec4f depthPlane1 = invArea.CompMul(ezSimdVec4f::Select(useArea2,
      z30.CompMul(edgeNormalsX4) - z20.CompMul(edgeNormalsX3),
      ezSimdVec4f::MulSub(z20, edgeNormalsX1, z12.CompMul(edgeNormalsX4))));

    const ezSimdVec4f depthPlane2 = invArea.CompMul(ezSimdVec4f::Select(useArea2,
      z30.CompMul(edgeNormalsY4) - z20.CompMul(edgeNormalsY3),
      ezSimdVec4f::MulSub(z20, edgeNormalsY1, z12.CompMul(edgeNormalsY4))));

    // Adjust depth base to bounding box min
    const ezSimdVec4f dx0 = x0 - minX.ToFloat();
    const ezSimdVec4f dy0 = y0 - minY.ToFloat();
    const ezSimdVec4f depthPlane0 = z0 - dx0.CompMul(depthPlane1) - dy0.CompMul(depthPlane2);

    // Fix edges for triangle modes
    edgeNormalsX2 = ezSimdVec4f::Select(modeTriangle0, edgeNormalsX4, edgeNormalsX2);
    edgeNormalsY2 = ezSimdVec4f::Select(modeTriangle0, edgeNormalsY4, edgeNormalsY2);
    edgeNormalsX0 = ezSimdVec4f::Select(modeTriangle1, -edgeNormalsX4, edgeNormalsX0);
    edgeNormalsY0 = ezSimdVec4f::Select(modeTriangle1, -edgeNormalsY4, edgeNormalsY0);

    // Edge flip masks for near-clipping
    ezSimdVec4f edgeFlipMask0, edgeFlipMask1, edgeFlipMask2, edgeFlipMask3;
    if constexpr (PossiblyNearClipped)
    {
      // Use ±1.0 as flip indicators
      const ezSimdVec4f vOne(1.0f);
      const ezSimdVec4f w0Sign = ezSimdVec4f::Select(wNeg0, -vOne, vOne);
      const ezSimdVec4f w1Sign = ezSimdVec4f::Select(wNeg1, -vOne, vOne);
      const ezSimdVec4f w2Sign = ezSimdVec4f::Select(wNeg2, -vOne, vOne);
      const ezSimdVec4f w3Sign = ezSimdVec4f::Select(wNeg3, -vOne, vOne);

      edgeFlipMask0 = w0Sign.CompMul(ezSimdVec4f::Select(modeTriangle1, w2Sign, w1Sign));
      edgeFlipMask1 = w1Sign.CompMul(w2Sign);
      edgeFlipMask2 = w2Sign.CompMul(ezSimdVec4f::Select(modeTriangle0, w0Sign, w3Sign));
      edgeFlipMask3 = w0Sign.CompMul(w3Sign);
    }
    else
    {
      edgeFlipMask0 = ezSimdVec4f(1.0f);
      edgeFlipMask1 = ezSimdVec4f(1.0f);
      edgeFlipMask2 = ezSimdVec4f(1.0f);
      edgeFlipMask3 = ezSimdVec4f(1.0f);
    }

    // Normalize edges
    NormalizeEdge<PossiblyNearClipped>(edgeNormalsX0, edgeNormalsY0, edgeFlipMask0);
    NormalizeEdge<PossiblyNearClipped>(edgeNormalsX1, edgeNormalsY1, edgeFlipMask1);
    NormalizeEdge<PossiblyNearClipped>(edgeNormalsX2, edgeNormalsY2, edgeFlipMask2);
    NormalizeEdge<PossiblyNearClipped>(edgeNormalsX3, edgeNormalsY3, edgeFlipMask3);

    // Compute edge offsets
    constexpr float fMaxOffset = -s_fMinEdgeOffset;
    const ezSimdVec4f vAdd(0.5f - s_fMinEdgeOffset * static_cast<float>(OFFSET_QUANTIZATION_FACTOR - 1) / (fMaxOffset - s_fMinEdgeOffset));

    ezSimdVec4f edgeOffsets0 = vAdd - dx0.CompMul(edgeNormalsX0) - dy0.CompMul(edgeNormalsY0);
    ezSimdVec4f edgeOffsets1 = vAdd - (x1 - minX.ToFloat()).CompMul(edgeNormalsX1) - (y1 - minY.ToFloat()).CompMul(edgeNormalsY1);
    ezSimdVec4f edgeOffsets2 = vAdd - (x2 - minX.ToFloat()).CompMul(edgeNormalsX2) - (y2 - minY.ToFloat()).CompMul(edgeNormalsY2);
    ezSimdVec4f edgeOffsets3 = vAdd - (x3 - minX.ToFloat()).CompMul(edgeNormalsX3) - (y3 - minY.ToFloat()).CompMul(edgeNormalsY3);

    // Quantize slopes
    const ezSimdVec4i slopeLookups0 = QuantizeSlopeLookupScaled(edgeNormalsX0, edgeNormalsY0);
    const ezSimdVec4i slopeLookups1 = QuantizeSlopeLookupScaled(edgeNormalsX1, edgeNormalsY1);
    const ezSimdVec4i slopeLookups2 = QuantizeSlopeLookupScaled(edgeNormalsX2, edgeNormalsY2);
    const ezSimdVec4i slopeLookups3 = QuantizeSlopeLookupScaled(edgeNormalsX3, edgeNormalsY3);

    // Compute first block index
    const ezSimdVec4i firstBlockIdx = minY.CompMul(ezSimdVec4i(m_uiBlocksX)) + minX;

    // Store per-primitive scalars
    ezInt32 firstBlocks[4], rangesX_arr[4], rangesY_arr[4];
    firstBlockIdx.Store<4>(firstBlocks);
    rangeX.Store<4>(rangesX_arr);
    rangeY.Store<4>(rangesY_arr);

    float maxZ_arr[4];
    maxZ.Store<4>(maxZ_arr);

    // Transpose from 4-wide SoA to per-primitive AoS
    ezSimdVec4f depthPlanes[4];
    Transpose4x4(depthPlane0, depthPlane1, depthPlane2, ezSimdVec4f::MakeZero(), depthPlanes);

    ezSimdVec4f edgeNormXArr[4];
    Transpose4x4(edgeNormalsX0, edgeNormalsX1, edgeNormalsX2, edgeNormalsX3, edgeNormXArr);

    ezSimdVec4f edgeNormYArr[4];
    Transpose4x4(edgeNormalsY0, edgeNormalsY1, edgeNormalsY2, edgeNormalsY3, edgeNormYArr);

    ezSimdVec4f edgeOffsArr[4];
    Transpose4x4(edgeOffsets0, edgeOffsets1, edgeOffsets2, edgeOffsets3, edgeOffsArr);

    ezSimdVec4i slopeLookupsArr[4];
    Transpose4x4i(slopeLookups0, slopeLookups1, slopeLookups2, slopeLookups3, slopeLookupsArr);

    // Iterate valid primitives
    const ezInt64* pTable = s_PrecomputedRasterTables.GetData();
    float* pHiZBuffer = m_HiZ.GetData();
    float* pDepthBuffer = m_DepthBuffer.GetData();

    for (ezUInt32 primitiveIdx = 0; primitiveIdx < 4; ++primitiveIdx)
    {
      if (!(validMask & (1u << primitiveIdx)))
        continue;

      const float fPrimitiveMaxZ = maxZ_arr[primitiveIdx];
      const ezUInt32 uiPrimMode = static_cast<ezUInt32>(modes[primitiveIdx]);
      const ezUInt32 uiFirstBlock = static_cast<ezUInt32>(firstBlocks[primitiveIdx]);
      const ezUInt32 uiBlockRangeX = static_cast<ezUInt32>(rangesX_arr[primitiveIdx]);
      const ezUInt32 uiBlockRangeY = static_cast<ezUInt32>(rangesY_arr[primitiveIdx]);

      // Depth plane for this primitive: {z0, dz_dx, dz_dy, 0}
      float dp[4];
      depthPlanes[primitiveIdx].Store<4>(dp);
      const float fDepthBase = dp[0];
      const float fDepthDx = dp[1];
      const float fDepthDy = dp[2];

      // Edge data
      float enx[4], eny[4], eo[4];
      edgeNormXArr[primitiveIdx].Store<4>(enx);
      edgeNormYArr[primitiveIdx].Store<4>(eny);
      edgeOffsArr[primitiveIdx].Store<4>(eo);

      ezInt32 sl[4];
      slopeLookupsArr[primitiveIdx].Store<4>(sl);

      float* pPrimitiveHiZ = pHiZBuffer + uiFirstBlock;
      float* pPrimitiveOut = pDepthBuffer + 64 * uiFirstBlock;

      float lineDepthBase = fDepthBase;
      float lineEdgeOfs[4] = {eo[0], eo[1], eo[2], eo[3]};

      for (ezUInt32 blockY = 0; blockY < uiBlockRangeY; ++blockY)
      {
        float* pBlockRowHiZ = pPrimitiveHiZ + blockY * m_uiBlocksX;
        float* pBlockRowDepth = pPrimitiveOut + blockY * m_uiBlocksX * 64;

        float curDepthBase = lineDepthBase;
        float curEdgeOfs[4] = {lineEdgeOfs[0], lineEdgeOfs[1], lineEdgeOfs[2], lineEdgeOfs[3]};

        bool bAnyBlockHit = false;

        for (ezUInt32 blockX = 0; blockX < uiBlockRangeX; ++blockX)
        {
          const float fHiZ = pBlockRowHiZ[blockX];

          // Hi-Z test: if the entire block is farther than our maximum depth, skip.
          // Cleared blocks have HiZ = -1.0f which is always < fPrimitiveMaxZ, so they pass through.
          if (fHiZ >= fPrimitiveMaxZ)
          {
            curDepthBase += fDepthDx;
            for (int e = 0; e < 4; ++e)
              curEdgeOfs[e] += enx[e];
            continue;
          }

          // Generate block mask
          ezUInt64 blockMask;

          if (uiPrimMode == Convex)
          {
            bool bAnyOutside = false;
            for (int e = 0; e < 4; ++e)
            {
              if (curEdgeOfs[e] >= static_cast<float>(OFFSET_QUANTIZATION_FACTOR - 1))
              {
                bAnyOutside = true;
                break;
              }
            }

            if (bAnyOutside)
            {
              if (bAnyBlockHit)
                break; // Convex: past the last block in this row

              curDepthBase += fDepthDx;
              for (int e = 0; e < 4; ++e)
                curEdgeOfs[e] += enx[e];
              continue;
            }

            bAnyBlockHit = true;

            ezInt32 offsetClamped[4];
            for (int e = 0; e < 4; ++e)
              offsetClamped[e] = ezMath::Clamp(static_cast<ezInt32>(curEdgeOfs[e]), 0, OFFSET_QUANTIZATION_FACTOR - 1);

            const ezUInt32 uiTableSize = static_cast<ezUInt32>(s_PrecomputedRasterTables.GetCount());
            const ezUInt32 idxA = ezMath::Min(static_cast<ezUInt32>(sl[0]) | static_cast<ezUInt32>(offsetClamped[0]), uiTableSize - 1);
            const ezUInt32 idxB = ezMath::Min(static_cast<ezUInt32>(sl[1]) | static_cast<ezUInt32>(offsetClamped[1]), uiTableSize - 1);
            const ezUInt32 idxC = ezMath::Min(static_cast<ezUInt32>(sl[2]) | static_cast<ezUInt32>(offsetClamped[2]), uiTableSize - 1);
            const ezUInt32 idxD = ezMath::Min(static_cast<ezUInt32>(sl[3]) | static_cast<ezUInt32>(offsetClamped[3]), uiTableSize - 1);

            const ezUInt64 A = static_cast<ezUInt64>(pTable[idxA]);
            const ezUInt64 B = static_cast<ezUInt64>(pTable[idxB]);
            const ezUInt64 C = static_cast<ezUInt64>(pTable[idxC]);
            const ezUInt64 D = static_cast<ezUInt64>(pTable[idxD]);

            blockMask = (A & B) & (C & D);

            if (!blockMask)
            {
              curDepthBase += fDepthDx;
              for (int e = 0; e < 4; ++e)
                curEdgeOfs[e] += enx[e];
              continue;
            }
          }
          else
          {
            ezInt32 offsetClamped[4];
            for (int e = 0; e < 4; ++e)
              offsetClamped[e] = ezMath::Clamp(static_cast<ezInt32>(curEdgeOfs[e]), 0, OFFSET_QUANTIZATION_FACTOR - 1);

            const ezUInt32 uiTableSize2 = static_cast<ezUInt32>(s_PrecomputedRasterTables.GetCount());
            const ezUInt32 idxA2 = ezMath::Min(static_cast<ezUInt32>(sl[0]) | static_cast<ezUInt32>(offsetClamped[0]), uiTableSize2 - 1);
            const ezUInt32 idxB2 = ezMath::Min(static_cast<ezUInt32>(sl[1]) | static_cast<ezUInt32>(offsetClamped[1]), uiTableSize2 - 1);
            const ezUInt32 idxC2 = ezMath::Min(static_cast<ezUInt32>(sl[2]) | static_cast<ezUInt32>(offsetClamped[2]), uiTableSize2 - 1);
            const ezUInt32 idxD2 = ezMath::Min(static_cast<ezUInt32>(sl[3]) | static_cast<ezUInt32>(offsetClamped[3]), uiTableSize2 - 1);

            const ezUInt64 A = static_cast<ezUInt64>(pTable[idxA2]);
            const ezUInt64 B = static_cast<ezUInt64>(pTable[idxB2]);
            const ezUInt64 C = static_cast<ezUInt64>(pTable[idxC2]);
            const ezUInt64 D = static_cast<ezUInt64>(pTable[idxD2]);

            switch (uiPrimMode)
            {
              case Triangle0:
                blockMask = A & B & C;
                break;
              case Triangle1:
                blockMask = A & C & D;
                break;
              case ConcaveRight:
                blockMask = (A | D) & (B & C);
                break;
              case ConcaveCenter:
                blockMask = (A & B) | (C & D);
                break;
              case ConcaveLeft:
              default:
                blockMask = (A & D) & (B | C);
                break;
            }

            if (!blockMask)
            {
              curDepthBase += fDepthDx;
              for (int e = 0; e < 4; ++e)
                curEdgeOfs[e] += enx[e];
              continue;
            }
          }

          // Write depth for this block
          float* pBlockDepth = pBlockRowDepth + blockX * 64;

          const bool bClearedBlock = (fHiZ < 0.0f);

          for (int py = 0; py < 8; ++py)
          {
            for (int px = 0; px < 8; ++px)
            {
              const ezUInt32 uiBitIdx = 8 * px + py;

              if (!(blockMask & (ezUInt64(1) << uiBitIdx)))
                continue;

              const float fPixelX = (px - 3.5f) / 8.0f;
              const float fPixelY = (py - 3.5f) / 8.0f;
              const float fDepth = curDepthBase + fPixelX * fDepthDx + fPixelY * fDepthDy;

              const ezUInt32 uiPixelIdx = py * 8 + px;

              if (bClearedBlock)
              {
                pBlockDepth[uiPixelIdx] = fDepth;
              }
              else
              {
                pBlockDepth[uiPixelIdx] = ezMath::Max(pBlockDepth[uiPixelIdx], fDepth);
              }
            }
          }

          // Update Hi-Z: recompute minimum depth across all 64 pixels in the block.
          // Matches AVX2 which always computes min across the full stored block.
          // Uncovered pixels remain at 0.0f (from Clear), so partial coverage yields HiZ = 0.
          {
            float fNewMinZ = pBlockDepth[0];
            for (int i = 1; i < 64; ++i)
              fNewMinZ = ezMath::Min(fNewMinZ, pBlockDepth[i]);
            pBlockRowHiZ[blockX] = fNewMinZ;
          }

          curDepthBase += fDepthDx;
          for (int e = 0; e < 4; ++e)
            curEdgeOfs[e] += enx[e];
        }

        lineDepthBase += fDepthDy;
        for (int e = 0; e < 4; ++e)
          lineEdgeOfs[e] += eny[e];
      }
    }
  }
}

template void RasterizerGeneric::Rasterize<true>(const OccluderGeneric& occluder);
template void RasterizerGeneric::Rasterize<false>(const OccluderGeneric& occluder);

// ===== queryVisibility =====

bool RasterizerGeneric::QueryVisibility(const ezSimdVec4f& vBoundsMin, const ezSimdVec4f& vBoundsMax, bool& out_bNeedsClipping)
{
  const ezSimdVec4f vExtents = vBoundsMax - vBoundsMin;

  // Load prebaked projection matrix
  ezSimdVec4f col0, col1, col2, col3;
  col0.Load<4>(m_ModelViewProjection + 0);
  col1.Load<4>(m_ModelViewProjection + 4);
  col2.Load<4>(m_ModelViewProjection + 8);
  col3.Load<4>(m_ModelViewProjection + 12);

  // Transform edges
  const ezSimdVec4f edge0 = col0 * vExtents.GetComponent<0>();
  const ezSimdVec4f edge1 = col1 * vExtents.GetComponent<1>();
  const ezSimdVec4f edge2 = col2 * vExtents.GetComponent<2>();

  // Transform first corner
  ezSimdVec4f corners[8];
  corners[0] = ezSimdVec4f::MulAdd(col0, ezSimdVec4f(vBoundsMin.GetComponent<0>()),
    ezSimdVec4f::MulAdd(col1, ezSimdVec4f(vBoundsMin.GetComponent<1>()),
      ezSimdVec4f::MulAdd(col2, ezSimdVec4f(vBoundsMin.GetComponent<2>()), col3)));

  corners[1] = corners[0] + edge0;
  corners[2] = corners[0] + edge1;
  corners[4] = corners[0] + edge2;
  corners[3] = corners[1] + edge1;
  corners[5] = corners[4] + edge0;
  corners[6] = corners[2] + edge2;
  corners[7] = corners[6] + edge0;

  // Transpose into SoA (X, Y, Z, W for corners 0-3 and 4-7)
  ezSimdVec4f cx03, cy03, cz03, cw03;
  ezSimdVec4f cx47, cy47, cz47, cw47;
  {
    float c0[4], c1[4], c2[4], c3[4], c4[4], c5[4], c6[4], c7[4];
    corners[0].Store<4>(c0); corners[1].Store<4>(c1); corners[2].Store<4>(c2); corners[3].Store<4>(c3);
    corners[4].Store<4>(c4); corners[5].Store<4>(c5); corners[6].Store<4>(c6); corners[7].Store<4>(c7);

    cx03 = ezSimdVec4f(c0[0], c1[0], c2[0], c3[0]);
    cy03 = ezSimdVec4f(c0[1], c1[1], c2[1], c3[1]);
    cz03 = ezSimdVec4f(c0[2], c1[2], c2[2], c3[2]);
    cw03 = ezSimdVec4f(c0[3], c1[3], c2[3], c3[3]);

    cx47 = ezSimdVec4f(c4[0], c5[0], c6[0], c7[0]);
    cy47 = ezSimdVec4f(c4[1], c5[1], c6[1], c7[1]);
    cz47 = ezSimdVec4f(c4[2], c5[2], c6[2], c7[2]);
    cw47 = ezSimdVec4f(c4[3], c5[3], c6[3], c7[3]);
  }

  // Check near plane
  const ezSimdFloat fMaxExtent = vExtents.HorizontalMax<3>();
  const ezSimdFloat fNearEps = fMaxExtent * ezSimdFloat(0.001f);
  const ezSimdVec4f vNearEps(fNearEps);
  const ezSimdVec4b closeToNear03 = cw03 < vNearEps;
  const ezSimdVec4b closeToNear47 = cw47 < vNearEps;

  if (closeToNear03.AnySet<4>() || closeToNear47.AnySet<4>())
  {
    out_bNeedsClipping = true;
    return true;
  }

  out_bNeedsClipping = false;

  // Perspective divide
  const ezSimdVec4f invW03 = cw03.GetReciprocal<ezMathAcc::BITS_12>();
  cx03 = cx03.CompMul(invW03);
  cy03 = cy03.CompMul(invW03);
  cz03 = cz03.CompMul(invW03);

  const ezSimdVec4f invW47 = cw47.GetReciprocal<ezMathAcc::BITS_12>();
  cx47 = cx47.CompMul(invW47);
  cy47 = cy47.CompMul(invW47);
  cz47 = cz47.CompMul(invW47);

  // Screen-space AABB
  const ezSimdVec4f minXVec = cx03.CompMin(cx47);
  const ezSimdVec4f maxXVec = cx03.CompMax(cx47);
  const ezSimdVec4f minYVec = cy03.CompMin(cy47);
  const ezSimdVec4f maxYVec = cy03.CompMax(cy47);

  const float fMinScreenX = ezMath::Max(static_cast<float>(minXVec.HorizontalMin<4>()) - 2.0f, 0.0f);
  const float fMinScreenY = ezMath::Max(static_cast<float>(minYVec.HorizontalMin<4>()) - 2.0f, 0.0f);
  const float fMaxScreenX = ezMath::Min(static_cast<float>(maxXVec.HorizontalMax<4>()) + 2.0f, static_cast<float>(m_uiWidth - 1));
  const float fMaxScreenY = ezMath::Min(static_cast<float>(maxYVec.HorizontalMax<4>()) + 2.0f, static_cast<float>(m_uiHeight - 1));

  const ezInt32 iMinX = static_cast<ezInt32>(ezMath::Floor(fMinScreenX));
  const ezInt32 iMinY = static_cast<ezInt32>(ezMath::Floor(fMinScreenY));
  const ezInt32 iMaxX = static_cast<ezInt32>(ezMath::Floor(fMaxScreenX));
  const ezInt32 iMaxY = static_cast<ezInt32>(ezMath::Floor(fMaxScreenY));

  if (iMinX >= iMaxX || iMinY >= iMaxY)
    return false;

  // Find max depth across all corners.
  // Add a small bias to compensate for precision differences between the query's depth
  // (computed from bounding box corners with approximate reciprocal) and the rasterized depth
  // (computed from triangle depth plane interpolation). Without this bias, objects at the
  // exact occlusion boundary can be falsely culled.
  // Find max depth across all corners.
  // Add a small bias to account for precision differences between the query depth
  // (computed from bounding box corners) and the rasterized depth (from triangle interpolation).
  // The AVX2 version implicitly has this tolerance due to 16-bit depth packing.
  const float fMaxZ = static_cast<float>(cz03.CompMax(cz47).HorizontalMax<4>()) + 0.01f;

  return Query2D(static_cast<ezUInt32>(iMinX), static_cast<ezUInt32>(iMaxX), static_cast<ezUInt32>(iMinY), static_cast<ezUInt32>(iMaxY), fMaxZ);
}

// ===== query2D =====

bool RasterizerGeneric::Query2D(ezUInt32 uiMinX, ezUInt32 uiMaxX, ezUInt32 uiMinY, ezUInt32 uiMaxY, float fMaxZ) const
{
  const float* pHiZBuffer = m_HiZ.GetData();
  const float* pDepthBuffer = m_DepthBuffer.GetData();

  const ezUInt32 blockMinX = uiMinX / 8;
  const ezUInt32 blockMaxX = uiMaxX / 8;
  const ezUInt32 blockMinY = uiMinY / 8;
  const ezUInt32 blockMaxY = uiMaxY / 8;

  for (ezUInt32 blockY = blockMinY; blockY <= blockMaxY; ++blockY)
  {
    const ezUInt32 startY = ezMath::Max(static_cast<ezInt32>(uiMinY) - static_cast<ezInt32>(8 * blockY), 0);
    const ezUInt32 endY = ezMath::Min(static_cast<ezInt32>(uiMaxY) - static_cast<ezInt32>(8 * blockY), 7);

    for (ezUInt32 blockX = blockMinX; blockX <= blockMaxX; ++blockX)
    {
      const float fHiZ = pHiZBuffer[blockY * m_uiBlocksX + blockX];

      // Hi-Z rejection. Cleared blocks have HiZ = -1.0f, so fMaxZ <= -1.0f is always
      // false for valid depths — cleared blocks correctly pass through.
      if (fMaxZ <= fHiZ)
        continue;

      const ezUInt32 startX = ezMath::Max(static_cast<ezInt32>(uiMinX) - static_cast<ezInt32>(blockX * 8), 0);
      const ezUInt32 endX = ezMath::Min(static_cast<ezInt32>(uiMaxX) - static_cast<ezInt32>(blockX * 8), 7);

      const bool bInteriorBlock = (startY == 0) && (endY == 7) && (startX == 0) && (endX == 7);

      if (bInteriorBlock)
        return true; // No masking needed and Hi-Z didn't reject → visible

      const float* pBlockDepth = pDepthBuffer + 64 * (blockY * m_uiBlocksX + blockX);

      for (ezUInt32 y = startY; y <= endY; ++y)
      {
        for (ezUInt32 x = startX; x <= endX; ++x)
        {
          if (fMaxZ > pBlockDepth[y * 8 + x])
            return true;
        }
      }
    }
  }

  return false;
}

// ===== readBackDepth =====

void RasterizerGeneric::ReadBackDepth(void* pTarget) const
{
  for (ezUInt32 blockY = 0; blockY < m_uiBlocksY; ++blockY)
  {
    for (ezUInt32 blockX = 0; blockX < m_uiBlocksX; ++blockX)
    {
      const float fHiZ = m_HiZ[blockY * m_uiBlocksX + blockX];

      if (fHiZ < 0.0f)
      {
        // Cleared block — zero-fill output
        for (ezUInt32 y = 0; y < 8; ++y)
        {
          ezUInt8* pDest = static_cast<ezUInt8*>(pTarget) + 4 * (8 * blockX + m_uiWidth * (8 * blockY + y));
          ezMemoryUtils::ZeroFill(pDest, 32);
        }
        continue;
      }

      const float* pBlockDepth = m_DepthBuffer.GetData() + 64 * (blockY * m_uiBlocksX + blockX);

      for (ezUInt32 y = 0; y < 8; ++y)
      {
        ezUInt8* pDest = static_cast<ezUInt8*>(pTarget) + 4 * (8 * blockX + m_uiWidth * (8 * blockY + y));

        for (ezUInt32 x = 0; x < 8; ++x)
        {
          const float fDepth = pBlockDepth[y * 8 + x];

          if (fDepth == 0.0f)
          {
            pDest[4 * x + 0] = 0;
            pDest[4 * x + 1] = 0;
            pDest[4 * x + 2] = 0;
            pDest[4 * x + 3] = 255; // Match AVX2 convention: alpha=255 for touched blocks
            continue;
          }

          // Linearize depth: convert from [0,1] reverse-Z to linear distance
          const float fNear = 0.25f;
          const float fFar = 1000.0f;
          const float fLinDepth = (2.0f * fNear) / (fNear + fFar - (1.0f - fDepth) * (fFar - fNear));

          const ezUInt32 d = static_cast<ezUInt32>(100 * 256 * fLinDepth);
          const ezUInt8 v0 = static_cast<ezUInt8>(d / 100);
          const ezUInt8 v1 = d % 256;

          pDest[4 * x + 0] = v0;
          pDest[4 * x + 1] = v1;
          pDest[4 * x + 2] = 0;
          pDest[4 * x + 3] = 255;
        }
      }
    }
  }
}
