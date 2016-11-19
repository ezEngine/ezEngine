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
  EZ_FOUNDATION_DLL ezResult ConvertToColor(const ezOpenDdlReaderElement* pElement, ezColor& out_result);

  /// \brief Converts the data that \a pElement points to to an ezColorGammaUB.
  ///
  /// \a pElement may be a primitives list of 3 or 4 floats or of 3 or 4 unsigned int8 values.
  /// It may also be a group that contains such a primitives list as the only child.
  /// floats will be interpreted as linear colors, unsigned int 8 will be interpreted as ezColorGammaUB.
  /// If only 3 values are given, alpha will be filled with 1.0f.
  /// If less than 3 or more than 4 values are given, the function returns EZ_FAILURE.
  EZ_FOUNDATION_DLL ezResult ConvertToColorGamma(const ezOpenDdlReaderElement* pElement, ezColorGammaUB& out_result);

  /// \brief Converts the data that \a pElement points to to an ezTime.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float or double.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToTime(const ezOpenDdlReaderElement* pElement, ezTime& out_result);

  /// \brief Converts the data that \a pElement points to to an ezVec2.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec2(const ezOpenDdlReaderElement* pElement, ezVec2& out_result);

  /// \brief Converts the data that \a pElement points to to an ezVec3.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec3(const ezOpenDdlReaderElement* pElement, ezVec3& out_result);

  /// \brief Converts the data that \a pElement points to to an ezVec4.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToVec4(const ezOpenDdlReaderElement* pElement, ezVec4& out_result);

  /// \brief Converts the data that \a pElement points to to an ezMat3.
  ///
  /// \a pElement maybe be a primitives list of exactly 9 floats.
  /// The elements are expected to be in column-major format. See ezMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToMat3(const ezOpenDdlReaderElement* pElement, ezMat3& out_result);

  /// \brief Converts the data that \a pElement points to to an ezMat4.
  ///
  /// \a pElement maybe be a primitives list of exactly 16 floats.
  /// The elements are expected to be in column-major format. See ezMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToMat4(const ezOpenDdlReaderElement* pElement, ezMat4& out_result);

  /// \brief Converts the data that \a pElement points to to an ezTransform.
  ///
  /// \a pElement maybe be a primitives list of exactly 12 floats.
  /// The first 9 elements are expected to be a mat3 in column-major format. See ezMatrixLayout::ColumnMajor.
  /// The last 3 elements are the position vector.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToTransform(const ezOpenDdlReaderElement* pElement, ezTransform& out_result);

  /// \brief Converts the data that \a pElement points to to an ezQuat.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToQuat(const ezOpenDdlReaderElement* pElement, ezQuat& out_result);

  /// \brief Converts the data that \a pElement points to to an ezUuid.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 unsigned_int64.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToUuid(const ezOpenDdlReaderElement* pElement, ezUuid& out_result);

  /// \brief Converts the data that \a pElement points to to an ezAngle.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float.
  /// The value is assumed to be in degree.
  /// It may also be a group that contains such a primitives list as the only child.
  EZ_FOUNDATION_DLL ezResult ConvertToAngle(const ezOpenDdlReaderElement* pElement, ezAngle& out_result);

  /// \brief Uses the elements custom type name to infer which type the object holds and reads it into the ezVariant.
  ///
  /// Depending on the custom type name, one of the other ConvertToXY functions is called and the respective conditions to the data format apply.
  /// Supported type names are: "Color", "ColorGamma", "Time", "Vec2", "Vec3", "Vec4", "Mat3", "Mat4", "Transform", "Quat", "Uuid", "Angle"
  /// Type names are case sensitive.
  EZ_FOUNDATION_DLL ezResult ConvertToVariant(const ezOpenDdlReaderElement* pElement, ezVariant& out_result);

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  /// \brief Writes an ezColor to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreColor(ezOpenDdlWriter& writer, const ezColor& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezColorGammaUB to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreColorGamma(ezOpenDdlWriter& writer, const ezColorGammaUB& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezTime to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreTime(ezOpenDdlWriter& writer, const ezTime& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezVec2 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec2(ezOpenDdlWriter& writer, const ezVec2& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezVec3 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec3(ezOpenDdlWriter& writer, const ezVec3& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezVec4 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVec4(ezOpenDdlWriter& writer, const ezVec4& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezMat3 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreMat3(ezOpenDdlWriter& writer, const ezMat3& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezMat4 to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreMat4(ezOpenDdlWriter& writer, const ezMat4& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezTransform to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreTransform(ezOpenDdlWriter& writer, const ezTransform& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezQuat to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreQuat(ezOpenDdlWriter& writer, const ezQuat& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezUuid to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreUuid(ezOpenDdlWriter& writer, const ezUuid& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezAngle to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreAngle(ezOpenDdlWriter& writer, const ezAngle& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes an ezVariant to DDL such that the type can be reconstructed.
  EZ_FOUNDATION_DLL void StoreVariant(ezOpenDdlWriter& writer, const ezVariant& value, const char* szName = nullptr, bool bGlobalName = false);

  /// \brief Writes a primitives list with a single string and an optional name.
  EZ_FOUNDATION_DLL void StoreString(ezOpenDdlWriter& writer, const ezStringView& value, const char* szName = nullptr, bool bGlobalName = false);

}

