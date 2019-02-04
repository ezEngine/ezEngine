#include <PCH.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/SimdMath/SimdMat4f.h>

///\todo optimize

ezResult ezSimdMat4f::Invert(const ezSimdFloat& fEpsilon)
{
  ezMat4 tmp;
  GetAsArray(tmp.m_fElementsCM, ezMatrixLayout::ColumnMajor);

  if (tmp.Invert(fEpsilon).Failed())
    return EZ_FAILURE;

  SetFromArray(tmp.m_fElementsCM, ezMatrixLayout::ColumnMajor);

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(Foundation, Foundation_SimdMath_Implementation_SimdMat4f);

