#include <PCH.h>
#include <Foundation/SimdMath/SimdVec4b.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdVec4b)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    // Placement new of the default constructor should not have any effect on the previous data.
    float EZ_ALIGN_16(testBlock[4]) = { 1, 2, 3, 4 };
    ezSimdVec4b* pDefCtor = ::new ((void*)&testBlock[0]) ezSimdVec4b;
    EZ_TEST_BOOL(testBlock[0] == 1.0f && testBlock[1] == 2.0f &&
      testBlock[2] == 3.0f && testBlock[3] == 4.0f);

    // Make sure the class didn't accidentally change in size.
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_CHECK_AT_COMPILETIME(sizeof(ezSimdVec4b) == 16);
    EZ_CHECK_AT_COMPILETIME(EZ_ALIGNMENT_OF(ezSimdVec4b) == 16);
#endif

    ezSimdVec4b vInit1B(true);
    EZ_TEST_BOOL(vInit1B.x() == true && vInit1B.y() == true &&
      vInit1B.z() == true && vInit1B.w() == true);

    // Make sure all components have the correct value
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_TEST_BOOL(vInit1B.m_v.m128_u32[0] == 0xFFFFFFFF && vInit1B.m_v.m128_u32[1] == 0xFFFFFFFF &&
      vInit1B.m_v.m128_u32[2] == 0xFFFFFFFF && vInit1B.m_v.m128_u32[3] == 0xFFFFFFFF);
#endif

    ezSimdVec4b vInit4B(false, true, false, true);
    EZ_TEST_BOOL(vInit4B.x() == false && vInit4B.y() == true &&
      vInit4B.z() == false && vInit4B.w() == true);

    // Make sure all components have the correct value
#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    EZ_TEST_BOOL(vInit4B.m_v.m128_u32[0] == 0 && vInit4B.m_v.m128_u32[1] == 0xFFFFFFFF &&
      vInit4B.m_v.m128_u32[2] == 0 && vInit4B.m_v.m128_u32[3] == 0xFFFFFFFF);
#endif

    ezSimdVec4b vCopy(vInit4B);
    EZ_TEST_BOOL(vCopy.x() == false && vCopy.y() == true &&
      vCopy.z() == false && vCopy.w() == true);

    EZ_TEST_BOOL(vCopy.GetComponent<0>() == false && vCopy.GetComponent<1>() == true &&
      vCopy.GetComponent<2>() == false && vCopy.GetComponent<3>() == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Swizzle")
  {
    ezSimdVec4b a(true, false, true, false);

    ezSimdVec4b b = a.Get<ezSwizzle::XXXX>();
    EZ_TEST_BOOL(b.x() && b.y() && b.z() && b.w());

    b = a.Get<ezSwizzle::YYYX>();
    EZ_TEST_BOOL(!b.x() && !b.y() && !b.z() && b.w());

    b = a.Get<ezSwizzle::ZZZX>();
    EZ_TEST_BOOL(b.x() && b.y() && b.z() && b.w());

    b = a.Get<ezSwizzle::WWWX>();
    EZ_TEST_BOOL(!b.x() && !b.y() && !b.z() && b.w());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    ezSimdVec4b a(true, false, true, false);
    ezSimdVec4b b(false, true, true, false);

    ezSimdVec4b c = a && b;
    EZ_TEST_BOOL(!c.x() && !c.y() && c.z() && !c.w());

    c = a || b;
    EZ_TEST_BOOL(c.x() && c.y() && c.z() && !c.w());

    c = !a;
    EZ_TEST_BOOL(!c.x() && c.y() && !c.z() && c.w());
    EZ_TEST_BOOL(c.AnySet());
    EZ_TEST_BOOL(!c.AllSet());
    EZ_TEST_BOOL(!c.NoneSet());

    c = c || a;
    EZ_TEST_BOOL(c.AnySet());
    EZ_TEST_BOOL(c.AllSet());
    EZ_TEST_BOOL(!c.NoneSet());

    c = !c;
    EZ_TEST_BOOL(!c.AnySet());
    EZ_TEST_BOOL(!c.AllSet());
    EZ_TEST_BOOL(c.NoneSet());
  }
}
