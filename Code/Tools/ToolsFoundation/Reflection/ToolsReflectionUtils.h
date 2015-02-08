#pragma once

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class ezIReflectedTypeAccessor;

/// \brief Helper functions for handling reflection related operations.
class EZ_TOOLSFOUNDATION_DLL ezToolsReflectionUtils
{
public:
  /// \brief Returns a global default initialization value for the given variant type.
  static ezVariant GetDefaultVariantFromType(ezVariant::Type::Enum type); // [tested]

  /// \brief Creates a ReflectedTypeDescriptor from an ezRTTI instance that can be serialized and registered at the ezReflectedTypeManager.
  static void GetReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc); // [tested]

  static void GetPropertyPathFromString(const char* szPath, ezPropertyPath& out_Path, ezHybridArray<ezString, 6>& out_Storage);

  static ezString GetStringFromPropertyPath(const ezPropertyPath& Path);

  static ezPropertyPath CreatePropertyPath(const char* pData1, const char* pData2 = nullptr, const char* pData3 = nullptr, const char* pData4 = nullptr, const char* pData5 = nullptr, const char* pData6 = nullptr);

  /// \brief Returns a member property by a ezPropertyPath. Also changes the ezRTTI and void* to match the properties owner object.
  static ezAbstractMemberProperty* GetMemberPropertyByPath(const ezRTTI*& inout_pRtti, void*& inout_pData, const ezPropertyPath& path);

  /// \brief Writes all property values of the reflected type accessor to \a stream in (extended) JSON format.
  ///
  /// Using ezReflectionUtils::ReadObjectPropertiesFromJSON() you can read those properties back into an existing object.
  /// Using ezReflectionUtils::ReadObjectFromJSON() an object of the same type is allocated an its properties are restored from the JSON data.
  ///
  /// The whitespace mode should be set according to whether the JSON data is used for interchange with other code only,
  /// or might also be read by humans.
  ///
  /// Read-only properties are not written out, as they cannot be restored anyway.
  static void WriteObjectToJSON(ezStreamWriterBase& stream, const ezIReflectedTypeAccessor& accessor, ezJSONWriter::WhitespaceMode::Enum WhitespaceMode = ezJSONWriter::WhitespaceMode::NewlinesOnly);

  static void ReadObjectPropertiesFromJSON(ezStreamReaderBase& stream, ezIReflectedTypeAccessor& accessor);

  /// \brief Converts an enum or bitfield value into its string representation.
  /// 
  /// The type of pEnumerationRtti will be automatically detected. The syntax of out_sOutput equals MSVC debugger output.
  static bool EnumerationToString(const ezReflectedType* pEnumerationRtti, ezInt64 iValue, ezStringBuilder& out_sOutput); // [tested]

  /// \brief Converts an enum or bitfield in its string representation to its value.
  /// 
  /// The type of pEnumerationRtti will be automatically detected. The syntax of szValue must equal the MSVC debugger output.
  static bool StringToEnumeration(const ezReflectedType* pEnumerationRtti, const char* szValue, ezInt64& out_iValue); // [tested]

};
