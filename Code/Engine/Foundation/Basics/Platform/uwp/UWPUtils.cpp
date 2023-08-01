#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#  include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#  include <Foundation/Types/Uuid.h>
#  include <Windows.Foundation.numerics.h>

ezMat4 ezUwpUtils::ConvertMat4(const ABI::Windows::Foundation::Numerics::Matrix4x4& in)
{
  return ezMat4(in.M11, in.M21, in.M31, in.M41, in.M12, in.M22, in.M32, in.M42, in.M13, in.M23, in.M33, in.M43, in.M14, in.M24, in.M34, in.M44);
}

ezVec3 ezUwpUtils::ConvertVec3(const ABI::Windows::Foundation::Numerics::Vector3& in)
{
  return ezVec3(in.X, in.Y, in.Z);
}

void ezUwpUtils::ConvertVec3(const ezVec3& in, ABI::Windows::Foundation::Numerics::Vector3& out)
{
  out.X = in.x;
  out.Y = in.y;
  out.Z = in.z;
}

ezQuat ezUwpUtils::ConvertQuat(const ABI::Windows::Foundation::Numerics::Quaternion& in)
{
  return ezQuat(in.X, in.Y, in.Z, in.W);
}

void ezUwpUtils::ConvertQuat(const ezQuat& in, ABI::Windows::Foundation::Numerics::Quaternion& out)
{
  out.X = in.x;
  out.Y = in.y;
  out.Z = in.z;
  out.W = in.w;
}

ezUuid ezUwpUtils::ConvertGuid(const GUID& in)
{
  return *reinterpret_cast<const ezUuid*>(&in);
}

void ezUwpUtils::ConvertGuid(const ezUuid& in, GUID& out)
{
  ezMemoryUtils::Copy(reinterpret_cast<ezUInt32*>(&out), reinterpret_cast<const ezUInt32*>(&in), 4);
}


#endif



EZ_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_uwp_UWPUtils);
