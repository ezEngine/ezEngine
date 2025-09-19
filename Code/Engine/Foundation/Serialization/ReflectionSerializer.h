#pragma once

#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Reflection/Reflection.h>

class ezOpenDdlReaderElement;

/// \brief High-level serialization interface for reflected objects using DDL and binary formats.
///
/// This class provides convenient functions for serializing/deserializing individual objects or their
/// properties using ezEngine's reflection system. It supports both DDL (Data Definition Language) and
/// binary formats with the same interface.
class EZ_FOUNDATION_DLL ezReflectionSerializer
{
public:
  /// \brief Writes all property values of the reflected object to stream in DDL format.
  ///
  /// Serializes the complete object state including all reflected properties. Non-existing objects
  /// (pObject == nullptr) are stored as objects of type "null". Read-only properties are not written
  /// as they cannot be restored.
  ///
  /// \param bCompactMode Controls formatting: true for machine-readable, false for human-readable
  /// \param typeMode Controls type name verbosity in output
  ///
  /// Use ReadObjectPropertiesFromDDL() to restore properties into an existing object,
  /// or ReadObjectFromDDL() to create a new object and restore its properties.
  static void WriteObjectToDDL(ezStreamWriter& inout_stream, const ezRTTI* pRtti, const void* pObject, bool bCompactMmode = true,
    ezOpenDdlWriter::TypeStringMode typeMode = ezOpenDdlWriter::TypeStringMode::Shortest); // [tested]

  /// \brief Overload of WriteObjectToDDL that takes an existing DDL writer to output to.
  static void WriteObjectToDDL(ezOpenDdlWriter& ref_ddl, const ezRTTI* pRtti, const void* pObject, ezUuid guid = ezUuid()); // [tested]

  /// \brief Same as WriteObjectToDDL but binary.
  static void WriteObjectToBinary(ezStreamWriter& inout_stream, const ezRTTI* pRtti, const void* pObject); // [tested]

  /// \brief Reads DDL data from stream and creates a new reflected object with restored properties.
  ///
  /// The object type is read from the DDL data and the object is allocated using the default allocator
  /// for that type. All properties are restored as described in the DDL data, as long as the properties
  /// can be matched to the runtime type. Properties that don't exist in the current type are ignored
  /// (useful for forward compatibility).
  ///
  /// \param ref_pRtti Outputs the RTTI type of the created object
  /// \return Pointer to the newly created object, or nullptr on failure
  static void* ReadObjectFromDDL(ezStreamReader& inout_stream, const ezRTTI*& ref_pRtti);               // [tested]

  static void* ReadObjectFromDDL(const ezOpenDdlReaderElement* pRootElement, const ezRTTI*& ref_pRtti); // [tested]

  /// \brief Same as ReadObjectFromDDL but binary.
  static void* ReadObjectFromBinary(ezStreamReader& inout_stream, const ezRTTI*& ref_pRtti); // [tested]

  /// \brief Reads DDL data and applies property values to an existing object.
  ///
  /// This function only modifies properties that exist in the DDL data and can be matched to the
  /// object's type. Properties not present in the DDL data remain unchanged. This is useful for:
  /// - Partial object updates
  /// - Configuration loading
  /// - Applying saved settings to default objects
  ///
  /// The object should ideally be of the same type that was serialized, but type mismatches are
  /// handled gracefully - compatible properties will be restored, incompatible ones ignored.
  static void ReadObjectPropertiesFromDDL(ezStreamReader& inout_stream, const ezRTTI& rtti, void* pObject); // [tested]

  /// \brief Same as ReadObjectPropertiesFromDDL but binary.
  static void ReadObjectPropertiesFromBinary(ezStreamReader& inout_stream, const ezRTTI& rtti, void* pObject); // [tested]

  /// \brief Clones pObject of type pType and returns it.
  ///
  /// In case a class derived from ezReflectedClass is passed in the correct derived type
  /// will automatically be determined so it is not necessary to put the exact type into pType,
  /// any derived class type will do.
  static void* Clone(const void* pObject, const ezRTTI* pType); // [tested]

  /// \brief Clones pObject of type pType into the already existing pClone.
  ///
  /// In case a class derived from ezReflectedClass is passed in the correct derived type
  /// will automatically be determined so it is not necessary to put the exact type into pType,
  /// any derived class type will do. However, the function will assert if pObject and pClone
  /// actually have a different type.
  static void Clone(const void* pObject, void* pClone, const ezRTTI* pType); // [tested]

  /// \brief Templated convenience function that calls Clone and automatically deduces the type.
  template <typename T>
  static T* Clone(const T* pObject)
  {
    return static_cast<T*>(Clone(pObject, ezGetStaticRTTI<T>()));
  }
};
