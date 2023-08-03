#include <ModelImporter2/ModelImporterPCH.h>

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

namespace ezModelImporter2
{
  ezColor ConvertAssimpType(const aiColor4D& value, bool bInvert /*= false*/)
  {
    if (bInvert)
      return ezColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b, 1.0f - value.a);
    else
      return ezColor(value.r, value.g, value.b, value.a);
  }

  ezColor ConvertAssimpType(const aiColor3D& value, bool bInvert /*= false*/)
  {
    if (bInvert)
      return ezColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b);
    else
      return ezColor(value.r, value.g, value.b);
  }

  ezMat4 ConvertAssimpType(const aiMatrix4x4& value, bool bDummy /*= false*/)
  {
    EZ_ASSERT_DEBUG(!bDummy, "not implemented");

    return ezMat4::MakeFromRowMajorArray(&value.a1);
  }

  ezVec3 ConvertAssimpType(const aiVector3D& value, bool bDummy /*= false*/)
  {
    EZ_ASSERT_DEBUG(!bDummy, "not implemented");

    return ezVec3(value.x, value.y, value.z);
  }

  ezQuat ConvertAssimpType(const aiQuaternion& value, bool bDummy /*= false*/)
  {
    EZ_ASSERT_DEBUG(!bDummy, "not implemented");

    return ezQuat(value.x, value.y, value.z, value.w);
  }

  float ConvertAssimpType(float value, bool bDummy /*= false*/)
  {
    EZ_ASSERT_DEBUG(!bDummy, "not implemented");

    return value;
  }

  int ConvertAssimpType(int value, bool bDummy /*= false*/)
  {
    EZ_ASSERT_DEBUG(!bDummy, "not implemented");

    return value;
  }

} // namespace ezModelImporter2
