#pragma once

#include <Foundation/SimdMath/SimdMath.h>

class ezSimdVec4b
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdVec4b();
  ezSimdVec4b(bool b);
  ezSimdVec4b(bool x, bool y, bool z, bool w);
  ezSimdVec4b(ezInternal::QuadBool b);

  ezSimdVec4b operator&&(const ezSimdVec4b& rhs) const;
  ezSimdVec4b operator||(const ezSimdVec4b& rhs) const;
  ezSimdVec4b operator!() const;

  bool AllSet();
  bool AnySet();
  bool NoneSet();

public:
  ezInternal::QuadBool m_v;
};

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  #include <Foundation/SimdMath/Implementation/SSE/SSEVec4b_inl.h>
#else
  #include <Foundation/SimdMath/Implementation/FPU/FPUVec4b_inl.h>
#endif

