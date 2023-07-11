#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlParser.h>

class ezOpenDdlReader;
class ezOpenDdlWriter;
class ezOpenDdlReaderElement;

namespace ezOpenDdlUtils
{
  /// \brief Converts the data that \a pElement points to to an ezColor.
  ///
  /// \a pElement may be a primitives list of 3 or 4 floats or of 3 or 4 unsigned int8 values.
  /// It may also be a group that contains such a primitives list as the only child.
  /// floats will be interpreted as linear colors, unsigned int 8 will be interpreted as ezColorGammaUB.
  /// If only 3 values are given, alpha will be filled with 1.0f.
  /// If less than 3 or more than 4 values are given, the function returns EZ_FAILURE.
  EZ_FOUNDATION_DLL ezResult ConvertToColor(const ezOpenDdlReaderElement* pElement, ezColor& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezColorGammaUB.
  ///
  /// \a pElement may be a primitives list of 3 or 4 floats or of 3 or 4 unsigned int8 values.
  /// It may also be a group that contains such a primitives list as the only child.
  /// floats will be interpreted as linear colors, unsigned int 8 will be interpreted as ezColorGammaUB.
  /// If only 3 values are given, alpha will be filled with 1.0f.
  /// If less than 3 or more than 4 values are given, the function returns EZ_FAILURE.
  EZ_FOUNDATION_DLL ezResult ConvertToColorGamma(const ezOpenDdlReaderElement* pElement, ezColorGammaUB& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezTime.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float or double.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToTime(const ezOpenDdlReaderElement* pElement, ezTime& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezVec2.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec2(const ezOpenDdlReaderElement* pElement, ezVec2& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezVec3.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec3(const ezOpenDdlReaderElement* pElement, ezVec3& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezVec4.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec4(const ezOpenDdlReaderElement* pElement, ezVec4& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezVec2I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec2I(const ezOpenDdlReaderElement* pElement, ezVec2I32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezVec3I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec3I(const ezOpenDdlReaderElement* pElement, ezVec3I32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezVec4I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec4I(const ezOpenDdlReaderElement* pElement, ezVec4I32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezVec2U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec2U(const ezOpenDdlReaderElement* pElement, ezVec2U32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezVec3U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec3U(const ezOpenDdlReaderElement* pElement, ezVec3U32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezVec4U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec4U(const ezOpenDdlReaderElement* pElement, ezVec4U32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezMat3.
  ///
  /// \a pElement maybe be a primitives list of exactly 9 floats.
  /// The elements are expected to be in column-major format. See ezMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToMat3(const ezOpenDdlReaderElement* pElement, ezMat3& out_mResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezMat4.
  ///
  /// \a pElement maybe be a primitives list of exactly 16 floats.
  /// The elements are expected to be in column-major format. See ezMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToMat4(const ezOpenDdlReaderElement* pElement, ezMat4& out_mResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezTransform.
  ///
  /// \a pElement maybe be a primitives list of exactly 12 floats.
  /// The first 9 elements are expected to be a mat3 in column-major format. See ezMatrixLayout::ColumnMajor.
  /// The last 3 elements are the position vector.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToTransform(const ezOpenDdlReaderElement* pElement, ezTransform& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezQuat.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToQuat(const ezOpenDdlReaderElement* pElement, ezQuat& out_qResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezUuid.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 unsigned_int64.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToUuid(const ezOpenDdlReaderElement* pElement, ezUuid& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezAngle.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float.
  /// The value is assumed to be in radians.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToAngle(const ezOpenDdlReaderElement* pElement, ezAngle& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezHashedString.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 string.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToHashedString(const ezOpenDdlReaderElement* pElement, ezHashedString& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an ezTempHashedString.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 uint64.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToTempHashedString(const ezOpenDdlReaderElement* pElement, ezTempHashedString& out_result); // [tested]

  /// \brief Uses the elements custom type name to infer which type the object holds and reads it into the ezVariant.
  ///
  /// Depending on the custom type name, one of the other ConvertToXY functions is called and the respective conditions to the data format apply.
  /// Supported type names are: "Color", "ColorGamma", "Time", "Vec2", "Vec3", "Vec4", "Mat3", "Mat4", "Transform", "Quat", "Uuid", "Angle", "HashedString", "TempHashedString"
  /// Type names are case sensitive.
  EZ_FOUNDATION_DLL ezResult ConvertToVariant(const ezOpenDdlReaderElement* pElement, ezVariant& out_result); // [tested]

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  /// \brief Writes an ezColor to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreColor(ezOpenDdlWriter& ref_writer, const ezColor& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezColorGammaUB to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreColorGamma(ezOpenDdlWriter& ref_writer, const ezColorGammaUB& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezTime to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreTime(ezOpenDdlWriter& ref_writer, const ezTime& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezVec2 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec2(ezOpenDdlWriter& ref_writer, const ezVec2& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezVec3 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec3(ezOpenDdlWriter& ref_writer, const ezVec3& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezVec4 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec4(ezOpenDdlWriter& ref_writer, const ezVec4& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezVec2 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec2I(ezOpenDdlWriter& ref_writer, const ezVec2I32& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezVec3 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec3I(ezOpenDdlWriter& ref_writer, const ezVec3I32& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezVec4 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec4I(ezOpenDdlWriter& ref_writer, const ezVec4I32& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezVec2 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec2U(ezOpenDdlWriter& ref_writer, const ezVec2U32& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezVec3 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec3U(ezOpenDdlWriter& ref_writer, const ezVec3U32& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezVec4 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec4U(ezOpenDdlWriter& ref_writer, const ezVec4U32& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezMat3 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreMat3(ezOpenDdlWriter& ref_writer, const ezMat3& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezMat4 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreMat4(ezOpenDdlWriter& ref_writer, const ezMat4& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezTransform to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreTransform(ezOpenDdlWriter& ref_writer, const ezTransform& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezQuat to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreQuat(ezOpenDdlWriter& ref_writer, const ezQuat& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezUuid to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreUuid(ezOpenDdlWriter& ref_writer, const ezUuid& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezAngle to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreAngle(ezOpenDdlWriter& ref_writer, const ezAngle& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezHashedString to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreHashedString(ezOpenDdlWriter& ref_writer, const ezHashedString& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezTempHashedString to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreTempHashedString(ezOpenDdlWriter& ref_writer, const ezTempHashedString& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an ezVariant to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVariant(ezOpenDdlWriter& ref_writer, const ezVariant& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single string and an optional name.
  EZ_FOUNDATION_DLL void StoreString(ezOpenDdlWriter& ref_writer, const ezStringView& value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreBool(ezOpenDdlWriter& ref_writer, bool value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreFloat(ezOpenDdlWriter& ref_writer, float value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreDouble(ezOpenDdlWriter& ref_writer, double value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreInt8(ezOpenDdlWriter& ref_writer, ezInt8 value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreInt16(ezOpenDdlWriter& ref_writer, ezInt16 value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreInt32(ezOpenDdlWriter& ref_writer, ezInt32 value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreInt64(ezOpenDdlWriter& ref_writer, ezInt64 value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreUInt8(ezOpenDdlWriter& ref_writer, ezUInt8 value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreUInt16(ezOpenDdlWriter& ref_writer, ezUInt16 value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreUInt32(ezOpenDdlWriter& ref_writer, ezUInt32 value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  EZ_FOUNDATION_DLL void StoreUInt64(ezOpenDdlWriter& ref_writer, ezUInt64 value, ezStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an invalid variant and an optional name.
  EZ_FOUNDATION_DLL void StoreInvalid(ezOpenDdlWriter& ref_writer, ezStringView sName = {}, bool bGlobalName = false);
} // namespace ezOpenDdlUtils
