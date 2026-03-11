#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/SimdMath/SimdMat4d.h>

///\todo optimize

ezResult ezSimdMat4d::Invert(const ezSimdDouble& fEpsilon)
{
  ezMat4d tmp;
  GetAsArray(tmp.m_fElementsCM, ezMatrixLayout::ColumnMajor);

  if (tmp.Invert(fEpsilon).Failed())
    return EZ_FAILURE;

  *this = ezSimdMat4d::MakeFromColumnMajorArray(tmp.m_fElementsCM);

  return EZ_SUCCESS;
}
