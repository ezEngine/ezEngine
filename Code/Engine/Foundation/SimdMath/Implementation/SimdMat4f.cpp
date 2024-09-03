#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/SimdMath/SimdMat4f.h>

///\todo optimize

ezResult ezSimdMat4f::Invert(const ezSimdFloat& fEpsilon)
{
  ezMat4 tmp;
  GetAsArray(tmp.m_fElementsCM, ezMatrixLayout::ColumnMajor);

  if (tmp.Invert(fEpsilon).Failed())
    return EZ_FAILURE;

  *this = ezSimdMat4f::MakeFromColumnMajorArray(tmp.m_fElementsCM);

  return EZ_SUCCESS;
}
