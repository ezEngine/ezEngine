// Shared MOC SIMD test definitions.
// This file is included AFTER the _mmw_* functions are defined (either SSE or ez).
// The including file must define:
//   MOC_TEST_PREFIX - string prefix for test names (e.g., "SSE" or "EZ")
//   _mmw_* functions/macros
//   simd_f32() / simd_i32() accessor functions
//   simd_cast<>() template functions
//   __mw / __mwi typedefs

// Helper to extract lane values for comparison
static void GetFloatLanes(const __mw& v, float out[4])
{
  const float* p = simd_f32(const_cast<__mw&>(v));
  out[0] = p[0]; out[1] = p[1]; out[2] = p[2]; out[3] = p[3];
}

static void GetIntLanes(const __mwi& v, int out[4])
{
  const int* p = simd_i32(const_cast<__mwi&>(v));
  out[0] = p[0]; out[1] = p[1]; out[2] = p[2]; out[3] = p[3];
}

// Helper macro to force expansion of MOC_TEST_PREFIX before token pasting
#define MOC_SIMD_TEST_PASTE2(Group, Name) EZ_CREATE_SIMPLE_RENDERER_TEST(Group, Name)
#define MOC_SIMD_TEST_PASTE(Name) MOC_SIMD_TEST_PASTE2(MOCSIMDTest, Name)

MOC_SIMD_TEST_PASTE(MOC_TEST_PREFIX)
{
  // ===== FLOAT ARITHMETIC =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "set1_ps / setzero_ps")
  {
    float r[4];
    GetFloatLanes(_mmw_set1_ps(42.0f), r);
    EZ_TEST_FLOAT(r[0], 42.0f, 0);
    EZ_TEST_FLOAT(r[1], 42.0f, 0);
    EZ_TEST_FLOAT(r[2], 42.0f, 0);
    EZ_TEST_FLOAT(r[3], 42.0f, 0);

    GetFloatLanes(_mmw_setzero_ps(), r);
    EZ_TEST_FLOAT(r[0], 0.0f, 0);
    EZ_TEST_FLOAT(r[3], 0.0f, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "add_ps / sub_ps / mul_ps / div_ps")
  {
    __mw a = _mmw_set1_ps(10.0f);
    __mw b = _mmw_set1_ps(3.0f);
    float r[4];

    GetFloatLanes(_mmw_add_ps(a, b), r);
    EZ_TEST_FLOAT(r[0], 13.0f, 0.0001f);

    GetFloatLanes(_mmw_sub_ps(a, b), r);
    EZ_TEST_FLOAT(r[0], 7.0f, 0.0001f);

    GetFloatLanes(_mmw_mul_ps(a, b), r);
    EZ_TEST_FLOAT(r[0], 30.0f, 0.0001f);

    GetFloatLanes(_mmw_div_ps(a, b), r);
    EZ_TEST_FLOAT(r[0], 10.0f / 3.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "min_ps / max_ps")
  {
    __mw a = _mmw_set1_ps(5.0f);
    __mw b = _mmw_set1_ps(8.0f);
    float r[4];

    GetFloatLanes(_mmw_min_ps(a, b), r);
    EZ_TEST_FLOAT(r[0], 5.0f, 0);

    GetFloatLanes(_mmw_max_ps(a, b), r);
    EZ_TEST_FLOAT(r[0], 8.0f, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "abs_ps / neg_ps")
  {
    __mw a = _mmw_set1_ps(-7.5f);
    float r[4];

    GetFloatLanes(_mmw_abs_ps(a), r);
    EZ_TEST_FLOAT(r[0], 7.5f, 0);

    GetFloatLanes(_mmw_neg_ps(_mmw_set1_ps(3.0f)), r);
    EZ_TEST_FLOAT(r[0], -3.0f, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "fmadd_ps / fmsub_ps")
  {
    __mw a = _mmw_set1_ps(2.0f);
    __mw b = _mmw_set1_ps(3.0f);
    __mw c = _mmw_set1_ps(4.0f);
    float r[4];

    // fmadd: a*b + c = 2*3+4 = 10
    GetFloatLanes(_mmw_fmadd_ps(a, b, c), r);
    EZ_TEST_FLOAT(r[0], 10.0f, 0.0001f);

    // fmsub: a*b - c = 2*3-4 = 2
    GetFloatLanes(_mmw_fmsub_ps(a, b, c), r);
    EZ_TEST_FLOAT(r[0], 2.0f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "floor_ps / ceil_ps")
  {
    float r[4];
    GetFloatLanes(_mmw_floor_ps(_mmw_set1_ps(2.7f)), r);
    EZ_TEST_FLOAT(r[0], 2.0f, 0);

    GetFloatLanes(_mmw_floor_ps(_mmw_set1_ps(-2.3f)), r);
    EZ_TEST_FLOAT(r[0], -3.0f, 0);

    GetFloatLanes(_mmw_ceil_ps(_mmw_set1_ps(2.3f)), r);
    EZ_TEST_FLOAT(r[0], 3.0f, 0);

    GetFloatLanes(_mmw_ceil_ps(_mmw_set1_ps(-2.7f)), r);
    EZ_TEST_FLOAT(r[0], -2.0f, 0);
  }

  // ===== FLOAT COMPARISONS =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "cmpge_ps / cmpgt_ps / cmpeq_ps")
  {
    __mw a = _mmw_set1_ps(5.0f);
    __mw b = _mmw_set1_ps(5.0f);
    __mw c = _mmw_set1_ps(3.0f);
    int r[4];

    // a >= b (equal) -> all 1s
    GetIntLanes(simd_cast<__mwi>(_mmw_cmpge_ps(a, b)), r);
    EZ_TEST_INT(r[0], ~0);

    // c >= a (less) -> all 0s
    GetIntLanes(simd_cast<__mwi>(_mmw_cmpge_ps(c, a)), r);
    EZ_TEST_INT(r[0], 0);

    // a > c -> all 1s
    GetIntLanes(simd_cast<__mwi>(_mmw_cmpgt_ps(a, c)), r);
    EZ_TEST_INT(r[0], ~0);

    // a > b (equal) -> all 0s
    GetIntLanes(simd_cast<__mwi>(_mmw_cmpgt_ps(a, b)), r);
    EZ_TEST_INT(r[0], 0);

    // a == b -> all 1s
    GetIntLanes(simd_cast<__mwi>(_mmw_cmpeq_ps(a, b)), r);
    EZ_TEST_INT(r[0], ~0);

    // a == c -> all 0s
    GetIntLanes(simd_cast<__mwi>(_mmw_cmpeq_ps(a, c)), r);
    EZ_TEST_INT(r[0], 0);
  }

  // ===== BLEND / MOVEMASK =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "blendv_ps")
  {
    __mw a = _mmw_set1_ps(1.0f);
    __mw b = _mmw_set1_ps(2.0f);

    // Mask with sign bit set -> select b
    __mw maskAllSet = simd_cast<__mw>(_mmw_set1_epi32((int)0x80000000));
    float r[4];
    GetFloatLanes(_mmw_blendv_ps(a, b, maskAllSet), r);
    EZ_TEST_FLOAT(r[0], 2.0f, 0);

    // Mask with sign bit clear -> select a
    __mw maskClear = _mmw_setzero_ps();
    GetFloatLanes(_mmw_blendv_ps(a, b, maskClear), r);
    EZ_TEST_FLOAT(r[0], 1.0f, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "blendv_epi32")
  {
    __mwi a = _mmw_set1_epi32(100);
    __mwi b = _mmw_set1_epi32(200);

    // Mask negative -> select b
    __mwi maskNeg = _mmw_set1_epi32(-1);
    int r[4];
    GetIntLanes(_mmw_blendv_epi32(a, b, maskNeg), r);
    EZ_TEST_INT(r[0], 200);

    // Mask zero -> select a
    __mwi maskZero = _mmw_setzero_epi32();
    GetIntLanes(_mmw_blendv_epi32(a, b, maskZero), r);
    EZ_TEST_INT(r[0], 100);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "movemask_ps")
  {
    // All positive -> 0
    EZ_TEST_INT(_mmw_movemask_ps(_mmw_set1_ps(1.0f)), 0);

    // All negative -> 0xF
    EZ_TEST_INT(_mmw_movemask_ps(_mmw_set1_ps(-1.0f)), 0xF);

    // Negative zero has sign bit set
    EZ_TEST_INT(_mmw_movemask_ps(_mmw_set1_ps(-0.0f)), 0xF);
  }

  // ===== FLOAT BITWISE =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "and_ps / or_ps / xor_ps / andnot_ps")
  {
    __mw allBits = simd_cast<__mw>(_mmw_set1_epi32(~0));
    __mw noBits = _mmw_setzero_ps();
    int r[4];

    // and: all & 0 = 0
    GetIntLanes(simd_cast<__mwi>(_mmw_and_ps(allBits, noBits)), r);
    EZ_TEST_INT(r[0], 0);

    // or: all | 0 = all
    GetIntLanes(simd_cast<__mwi>(_mmw_or_ps(allBits, noBits)), r);
    EZ_TEST_INT(r[0], ~0);

    // xor: all ^ all = 0
    GetIntLanes(simd_cast<__mwi>(_mmw_xor_ps(allBits, allBits)), r);
    EZ_TEST_INT(r[0], 0);

    // andnot(a, b) = ~a & b
    GetIntLanes(simd_cast<__mwi>(_mmw_andnot_ps(noBits, allBits)), r);
    EZ_TEST_INT(r[0], ~0);
    GetIntLanes(simd_cast<__mwi>(_mmw_andnot_ps(allBits, allBits)), r);
    EZ_TEST_INT(r[0], 0);
  }

  // ===== INTEGER ARITHMETIC =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "set1_epi32 / add / sub / mullo / neg / abs")
  {
    __mwi a = _mmw_set1_epi32(10);
    __mwi b = _mmw_set1_epi32(3);
    int r[4];

    GetIntLanes(_mmw_add_epi32(a, b), r);
    EZ_TEST_INT(r[0], 13);

    GetIntLanes(_mmw_sub_epi32(a, b), r);
    EZ_TEST_INT(r[0], 7);

    GetIntLanes(_mmw_mullo_epi32(a, b), r);
    EZ_TEST_INT(r[0], 30);

    GetIntLanes(_mmw_neg_epi32(a), r);
    EZ_TEST_INT(r[0], -10);

    GetIntLanes(_mmw_abs_epi32(_mmw_set1_epi32(-42)), r);
    EZ_TEST_INT(r[0], 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "min_epi32 / max_epi32")
  {
    __mwi a = _mmw_set1_epi32(-5);
    __mwi b = _mmw_set1_epi32(10);
    int r[4];

    GetIntLanes(_mmw_min_epi32(a, b), r);
    EZ_TEST_INT(r[0], -5);

    GetIntLanes(_mmw_max_epi32(a, b), r);
    EZ_TEST_INT(r[0], 10);
  }

  // ===== INTEGER SHIFTS =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "slli / srai / srli")
  {
    __mwi v = _mmw_set1_epi32(0x0000FF00);
    int r[4];

    // Left shift by 4
    GetIntLanes(_mmw_slli_epi32(v, 4), r);
    EZ_TEST_INT(r[0], (int)0x000FF000);

    // Arithmetic right shift (sign-extending)
    __mwi neg = _mmw_set1_epi32((int)0x80000000);
    GetIntLanes(_mmw_srai_epi32(neg, 4), r);
    EZ_TEST_INT(r[0], (int)0xF8000000);

    // Logical right shift (zero-filling)
    GetIntLanes(_mmw_srli_epi32(neg, 4), r);
    EZ_TEST_INT(r[0], (int)0x08000000);
  }

  // ===== INTEGER COMPARISONS =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "cmpeq_epi32 / cmpgt_epi32")
  {
    __mwi a = _mmw_set1_epi32(5);
    __mwi b = _mmw_set1_epi32(5);
    __mwi c = _mmw_set1_epi32(3);
    int r[4];

    GetIntLanes(_mmw_cmpeq_epi32(a, b), r);
    EZ_TEST_INT(r[0], ~0);

    GetIntLanes(_mmw_cmpeq_epi32(a, c), r);
    EZ_TEST_INT(r[0], 0);

    GetIntLanes(_mmw_cmpgt_epi32(a, c), r);
    EZ_TEST_INT(r[0], ~0);

    GetIntLanes(_mmw_cmpgt_epi32(c, a), r);
    EZ_TEST_INT(r[0], 0);
  }

  // ===== INTEGER BITWISE =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "and / or / xor / not / andnot epi32")
  {
    __mwi a = _mmw_set1_epi32(0x0F0F0F0F);
    __mwi b = _mmw_set1_epi32((int)0xF0F0F0F0u);
    int r[4];

    GetIntLanes(_mmw_and_epi32(a, b), r);
    EZ_TEST_INT(r[0], 0);

    GetIntLanes(_mmw_or_epi32(a, b), r);
    EZ_TEST_INT(r[0], ~0);

    GetIntLanes(_mmw_xor_epi32(a, a), r);
    EZ_TEST_INT(r[0], 0);

    GetIntLanes(_mmw_not_epi32(_mmw_setzero_epi32()), r);
    EZ_TEST_INT(r[0], ~0);

    // andnot(a, b) = ~a & b
    GetIntLanes(_mmw_andnot_epi32(a, _mmw_set1_epi32(~0)), r);
    EZ_TEST_INT(r[0], (int)0xF0F0F0F0u);
  }

  // ===== CONVERSION =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "cvtps_epi32 (round) / cvttps_epi32 (truncate)")
  {
    int r[4];

    // Truncate: 2.7 -> 2
    GetIntLanes(_mmw_cvttps_epi32(_mmw_set1_ps(2.7f)), r);
    EZ_TEST_INT(r[0], 2);

    // Truncate: -2.7 -> -2
    GetIntLanes(_mmw_cvttps_epi32(_mmw_set1_ps(-2.7f)), r);
    EZ_TEST_INT(r[0], -2);

    // Round: 2.5 -> 2 (banker's rounding) or 3 (round half up) - both acceptable
    GetIntLanes(_mmw_cvtps_epi32(_mmw_set1_ps(2.3f)), r);
    EZ_TEST_INT(r[0], 2);

    GetIntLanes(_mmw_cvtps_epi32(_mmw_set1_ps(2.7f)), r);
    EZ_TEST_INT(r[0], 3);

    // cvtepi32_ps: int -> float
    float rf[4];
    GetFloatLanes(_mmw_cvtepi32_ps(_mmw_set1_epi32(42)), rf);
    EZ_TEST_FLOAT(rf[0], 42.0f, 0);
  }

  // ===== SPECIAL OPS =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "testz_epi32")
  {
    // Zero & anything = zero -> testz returns 1
    EZ_TEST_INT(_mmw_testz_epi32(_mmw_setzero_epi32(), _mmw_set1_epi32(~0)), 1);

    // Non-zero overlap -> testz returns 0
    EZ_TEST_INT(_mmw_testz_epi32(_mmw_set1_epi32(1), _mmw_set1_epi32(1)), 0);

    // No overlapping bits -> testz returns 1
    EZ_TEST_INT(_mmw_testz_epi32(_mmw_set1_epi32(0x0F), _mmw_set1_epi32((int)0xF0)), 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "subs_epu16")
  {
    int r[4];

    // Normal subtract: 100 - 30 = 70
    GetIntLanes(_mmw_subs_epu16(_mmw_set1_epi32(100), _mmw_set1_epi32(30)), r);
    EZ_TEST_INT(r[0], 70);

    // Saturate to zero: 10 - 30 = 0 (not -20)
    GetIntLanes(_mmw_subs_epu16(_mmw_set1_epi32(10), _mmw_set1_epi32(30)), r);
    EZ_TEST_INT(r[0], 0);

    // Zero - anything = 0
    GetIntLanes(_mmw_subs_epu16(_mmw_setzero_epi32(), _mmw_set1_epi32(5)), r);
    EZ_TEST_INT(r[0], 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "sllv_ones")
  {
    int r[4];

    // Shift 0 -> ~0 (all bits set)
    GetIntLanes(_mmw_sllv_ones(_mmw_set1_epi32(0)), r);
    EZ_TEST_INT(r[0], ~0);

    // Shift 1 -> ~0 << 1 = 0xFFFFFFFE
    GetIntLanes(_mmw_sllv_ones(_mmw_set1_epi32(1)), r);
    EZ_TEST_INT(r[0], (int)0xFFFFFFFEu);

    // Shift 16 -> ~0 << 16 = 0xFFFF0000
    GetIntLanes(_mmw_sllv_ones(_mmw_set1_epi32(16)), r);
    EZ_TEST_INT(r[0], (int)0xFFFF0000u);

    // Shift 31 -> ~0 << 31 = 0x80000000
    GetIntLanes(_mmw_sllv_ones(_mmw_set1_epi32(31)), r);
    EZ_TEST_INT(r[0], (int)0x80000000u);

    // Shift 32 -> 0
    GetIntLanes(_mmw_sllv_ones(_mmw_set1_epi32(32)), r);
    EZ_TEST_INT(r[0], 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "transpose_epi8")
  {
    // _mmw_transpose_epi8 performs a 4x4 byte matrix transpose across the 128-bit register.
    // Input bytes:  [a0 a1 a2 a3] [b0 b1 b2 b3] [c0 c1 c2 c3] [d0 d1 d2 d3]
    // Output bytes: [a0 b0 c0 d0] [a1 b1 c1 d1] [a2 b2 c2 d2] [a3 b3 c3 d3]
    int r[4];

    // Test with uniform: all lanes = 0x04030201
    // Each lane has bytes [01, 02, 03, 04]
    // After transpose: lane0 = [01,01,01,01], lane1 = [02,02,02,02], etc.
    __mwi uniform = _mmw_set1_epi32(0x04030201);
    GetIntLanes(_mmw_transpose_epi8(uniform), r);
    EZ_TEST_INT(r[0], 0x01010101);
    EZ_TEST_INT(r[1], 0x02020202);
    EZ_TEST_INT(r[2], 0x03030303);
    EZ_TEST_INT(r[3], 0x04040404);

    // All ones -> all ones
    GetIntLanes(_mmw_transpose_epi8(_mmw_set1_epi32(~0)), r);
    EZ_TEST_INT(r[0], ~0);
    EZ_TEST_INT(r[3], ~0);

    // All zeros -> all zeros
    GetIntLanes(_mmw_transpose_epi8(_mmw_setzero_epi32()), r);
    EZ_TEST_INT(r[0], 0);

    // Test with distinct values per lane
    // Lane 0: [0x11, 0x22, 0x33, 0x44] = 0x44332211
    // Lane 1: [0x55, 0x66, 0x77, 0x88] = 0x88776655
    // Lane 2: [0xAA, 0xBB, 0xCC, 0xDD] = 0xDDCCBBAA
    // Lane 3: [0xEE, 0xFF, 0x01, 0x02] = 0x0201FFEE
    __mwi distinct = _mmw_setzero_epi32();
    simd_i32(distinct)[0] = 0x44332211;
    simd_i32(distinct)[1] = (int)0x88776655u;
    simd_i32(distinct)[2] = (int)0xDDCCBBAAu;
    simd_i32(distinct)[3] = 0x0201FFEE;

    // After transpose:
    // Lane 0: [0x11, 0x55, 0xAA, 0xEE] = 0xEEAA5511
    // Lane 1: [0x22, 0x66, 0xBB, 0xFF] = 0xFFBB6622
    // Lane 2: [0x33, 0x77, 0xCC, 0x01] = 0x01CC7733
    // Lane 3: [0x44, 0x88, 0xDD, 0x02] = 0x02DD8844
    GetIntLanes(_mmw_transpose_epi8(distinct), r);
    EZ_TEST_INT(r[0], (int)0xEEAA5511u);
    EZ_TEST_INT(r[1], (int)0xFFBB6622u);
    EZ_TEST_INT(r[2], 0x01CC7733);
    EZ_TEST_INT(r[3], 0x02DD8844);
  }

  // ===== SIMD CAST =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "simd_cast round-trip")
  {
    // Float -> Int -> Float bit cast should preserve bits
    __mw original = _mmw_set1_ps(1.0f);
    __mw roundTrip = simd_cast<__mw>(simd_cast<__mwi>(original));
    float ro[4], rr[4];
    GetFloatLanes(original, ro);
    GetFloatLanes(roundTrip, rr);
    EZ_TEST_FLOAT(ro[0], rr[0], 0);

    // Int bits of 1.0f should be 0x3F800000
    int ri[4];
    GetIntLanes(simd_cast<__mwi>(_mmw_set1_ps(1.0f)), ri);
    EZ_TEST_INT(ri[0], 0x3F800000);
  }

  // ===== PER-LANE ACCESS =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "simd_f32 / simd_i32 per-lane read/write")
  {
    __mw fv = _mmw_setzero_ps();
    simd_f32(fv)[0] = 1.0f;
    simd_f32(fv)[1] = 2.0f;
    simd_f32(fv)[2] = 3.0f;
    simd_f32(fv)[3] = 4.0f;

    EZ_TEST_FLOAT(simd_f32(fv)[0], 1.0f, 0);
    EZ_TEST_FLOAT(simd_f32(fv)[1], 2.0f, 0);
    EZ_TEST_FLOAT(simd_f32(fv)[2], 3.0f, 0);
    EZ_TEST_FLOAT(simd_f32(fv)[3], 4.0f, 0);

    __mwi iv = _mmw_setzero_epi32();
    simd_i32(iv)[0] = 10;
    simd_i32(iv)[1] = 20;
    simd_i32(iv)[2] = 30;
    simd_i32(iv)[3] = 40;

    EZ_TEST_INT(simd_i32(iv)[0], 10);
    EZ_TEST_INT(simd_i32(iv)[1], 20);
    EZ_TEST_INT(simd_i32(iv)[2], 30);
    EZ_TEST_INT(simd_i32(iv)[3], 40);
  }

  // ===== MIXED-LANE OPERATIONS =====

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "srai sign extraction pattern")
  {
    // This pattern is critical: srai(x, 31) extracts sign bit to all bits
    int r[4];

    // Positive -> srai 31 -> 0
    GetIntLanes(_mmw_srai_epi32(_mmw_set1_epi32(1), 31), r);
    EZ_TEST_INT(r[0], 0);

    // Negative -> srai 31 -> -1 (all bits set)
    GetIntLanes(_mmw_srai_epi32(_mmw_set1_epi32(-1), 31), r);
    EZ_TEST_INT(r[0], -1);

    // 0x80000000 -> srai 31 -> -1
    GetIntLanes(_mmw_srai_epi32(_mmw_set1_epi32((int)0x80000000u), 31), r);
    EZ_TEST_INT(r[0], -1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "blendv_ps with comparison mask")
  {
    // This tests the actual usage pattern in MOC: cmpge produces a mask, blendv uses it
    __mw a = _mmw_set1_ps(1.0f);
    __mw b = _mmw_set1_ps(2.0f);

    // 5.0 >= 3.0 is true -> mask has sign bit set (all 1s) -> select b
    __mw mask = _mmw_cmpge_ps(_mmw_set1_ps(5.0f), _mmw_set1_ps(3.0f));
    float r[4];
    GetFloatLanes(_mmw_blendv_ps(a, b, mask), r);
    EZ_TEST_FLOAT(r[0], 2.0f, 0);

    // 3.0 >= 5.0 is false -> mask is 0 -> select a
    mask = _mmw_cmpge_ps(_mmw_set1_ps(3.0f), _mmw_set1_ps(5.0f));
    GetFloatLanes(_mmw_blendv_ps(a, b, mask), r);
    EZ_TEST_FLOAT(r[0], 1.0f, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "movemask with comparison pattern")
  {
    // cmpgt produces masks, movemask extracts sign bits
    __mw mask = _mmw_cmpgt_ps(_mmw_set1_ps(5.0f), _mmw_set1_ps(3.0f));
    EZ_TEST_INT(_mmw_movemask_ps(mask), 0xF);

    mask = _mmw_cmpgt_ps(_mmw_set1_ps(3.0f), _mmw_set1_ps(5.0f));
    EZ_TEST_INT(_mmw_movemask_ps(mask), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "depth update pattern: srai + cmpeq + andnot")
  {
    // Simulate the MOC depth update pattern:
    // sign = srai(cast(sub(z1, z0)), 31)   - extract sign of depth difference
    // deadLane = cmpeq(mask, 0) | sign
    // result = andnot(deadLane, mask)

    __mwi mask = _mmw_set1_epi32(0x0F0F0F0F);

    // Positive depth difference (z1 > z0) -> sign = 0 -> deadLane only from mask==0
    __mw depthDiff = _mmw_set1_ps(1.0f); // positive
    __mwi sign = _mmw_srai_epi32(simd_cast<__mwi>(depthDiff), 31);
    int r[4];
    GetIntLanes(sign, r);
    EZ_TEST_INT(r[0], 0); // positive means no sign

    // Negative depth difference -> sign = -1
    depthDiff = _mmw_set1_ps(-1.0f);
    sign = _mmw_srai_epi32(simd_cast<__mwi>(depthDiff), 31);
    GetIntLanes(sign, r);
    EZ_TEST_INT(r[0], -1); // negative means all-1s sign

    // Full pattern
    __mwi deadLane = _mmw_or_epi32(_mmw_cmpeq_epi32(mask, _mmw_setzero_epi32()), sign);
    __mwi result = _mmw_andnot_epi32(deadLane, mask);
    GetIntLanes(result, r);
    EZ_TEST_INT(r[0], 0); // dead because sign was -1
  }
}
