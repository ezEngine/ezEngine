#include <ModelImporterPCH.h>

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

namespace ezModelImporter2
{
  ezColor ConvertAssimpType(const aiColor4D& value, bool invert /*= false*/)
  {
    if (invert)
      return ezColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b, 1.0f - value.a);
    else
      return ezColor(value.r, value.g, value.b, value.a);
  }

  ezColor ConvertAssimpType(const aiColor3D& value, bool invert /*= false*/)
  {
    if (invert)
      return ezColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b);
    else
      return ezColor(value.r, value.g, value.b);
  }

  ezMat4 ConvertAssimpType(const aiMatrix4x4& value, bool dummy /*= false*/)
  {
    EZ_ASSERT_DEBUG(!dummy, "not implemented");

    ezMat4 mTransformation;
    mTransformation.SetFromArray(&value.a1, ezMatrixLayout::RowMajor);
    return mTransformation;
  }

  ezVec3 ConvertAssimpType(const aiVector3D& value, bool dummy /*= false*/)
  {
    EZ_ASSERT_DEBUG(!dummy, "not implemented");

    return ezVec3(value.x, value.y, value.z);
  }

  ezQuat ConvertAssimpType(const aiQuaternion& value, bool dummy /*= false*/)
  {
    EZ_ASSERT_DEBUG(!dummy, "not implemented");

    return ezQuat(value.x, value.y, value.z, value.w);
  }

  float ConvertAssimpType(float value, bool dummy /*= false*/)
  {
    EZ_ASSERT_DEBUG(!dummy, "not implemented");

    return value;
  }

  int ConvertAssimpType(int value, bool dummy /*= false*/)
  {
    EZ_ASSERT_DEBUG(!dummy, "not implemented");

    return value;
  }

} // namespace ezModelImporter2
