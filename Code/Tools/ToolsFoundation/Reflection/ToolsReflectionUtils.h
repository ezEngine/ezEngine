#pragma once

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class ezIReflectedTypeAccessor;
class ezDocumentObject;

/// \brief Helper functions for handling reflection related operations.
class EZ_TOOLSFOUNDATION_DLL ezToolsReflectionUtils
{
public:
  /// \brief Returns a global default initialization value for the given variant type.
  static ezVariant GetDefaultVariantFromType(ezVariant::Type::Enum type); // [tested]

  static ezVariant GetDefaultValue(const ezAbstractProperty* pProperty);

  /// \brief Creates a ReflectedTypeDescriptor from an ezRTTI instance that can be serialized and registered at the ezPhantomRttiManager.
  static void GetReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc); // [tested]

  /// \brief Writes all property values of the reflected type accessor to \a stream in (extended) JSON format.
  ///
  /// Using ezReflectionUtils::ReadObjectPropertiesFromJSON() you can read those properties back into an existing object.
  /// Using ezReflectionUtils::ReadObjectFromJSON() an object of the same type is allocated an its properties are restored from the JSON data.
  ///
  /// The whitespace mode should be set according to whether the JSON data is used for interchange with other code only,
  /// or might also be read by humans.
  ///
  /// Read-only properties are not written out, as they cannot be restored anyway.
  static void WriteObjectToJSON(bool bSerializeOwnerPtrs, ezStreamWriter& stream, const ezDocumentObject* pObject, ezJSONWriter::WhitespaceMode WhitespaceMode = ezJSONWriter::WhitespaceMode::None);

  static void GatherObjectTypes(const ezDocumentObject* pObject, ezSet<const ezRTTI*>& inout_types, bool bOnlyPhantomTypes = true);

  static bool DependencySortTypeDescriptorArray(ezDynamicArray<ezReflectedTypeDescriptor*>& descriptors);

};
