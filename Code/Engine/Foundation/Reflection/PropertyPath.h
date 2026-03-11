#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>

class ezAbstractProperty;


///\brief Reflected property step that can be used to init an ezPropertyPath
struct EZ_FOUNDATION_DLL ezPropertyPathStep
{
  ezString m_sProperty;
  ezVariant m_Index;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezPropertyPathStep);

/// \brief Stores a path from an object of a given type to a property inside of it.
///
/// Once initialized to a specific path, the target property/object of the path can be read or written on
/// multiple root objects. This is useful for implementing property binding, serialization, and generic
/// property editors.
///
/// Path syntax: 'propertyName[index]/propertyName[index]/...'
/// - The '[index]' part is only required for properties that need indices (arrays and maps)
/// - Example: "Transform/Position[0]" accesses the X component of a Position vector in a Transform property
/// - An empty path is allowed, in which case operations work directly on the root object
///
/// Usage pattern:
/// 1. Create ezPropertyPath instance
/// 2. Initialize with InitializeFromPath() using root type and path string
/// 3. Use SetValue()/GetValue() for simple access or WriteProperty()/ReadProperty() for complex operations
/// 4. Reuse the same path instance for multiple objects of the same root type
class EZ_FOUNDATION_DLL ezPropertyPath
{
public:
  ezPropertyPath();
  ~ezPropertyPath();

  /// \brief Returns true if InitializeFromPath() has been successfully called and it is therefore possible to use the other functions.
  bool IsValid() const;

  /// \brief Resolves a path string into property steps and validates them against the root type.
  ///
  /// The path syntax is 'propertyName[index]/propertyName[index]/...'. The '[index]' part is only
  /// required for properties that need indices (arrays and maps). Returns failure if any property
  /// in the path doesn't exist or has incompatible types.
  ezResult InitializeFromPath(const ezRTTI& rootObjectRtti, const char* szPath);

  /// \brief Resolves a path provided as an array of ezPropertyPathStep and validates it.
  ///
  /// This overload allows programmatic construction of paths. Each step must have a valid property
  /// name and appropriate index (if required by the property type).
  ezResult InitializeFromPath(const ezRTTI* pRootObjectRtti, const ezArrayPtr<const ezPropertyPathStep> path);

  ///\brief Applies the entire path and allows writing to the target object.
  ezResult WriteToLeafObject(void* pRootObject, const ezRTTI* pType, ezDelegate<void(void* pLeaf, const ezRTTI& pType)> func) const;
  ///\brief Applies the entire path and allows reading from the target object.
  ezResult ReadFromLeafObject(void* pRootObject, const ezRTTI* pType, ezDelegate<void(void* pLeaf, const ezRTTI& pType)> func) const;

  ///\brief Applies the path up to the last step and allows a functor to write to the final property.
  ezResult WriteProperty(
    void* pRootObject, const ezRTTI& type, ezDelegate<void(void* pLeafObject, const ezRTTI& pLeafType, const ezAbstractProperty* pProp, const ezVariant& index)> func) const;
  ///\brief Applies the path up to the last step and allows a functor to read from the final property.
  ezResult ReadProperty(
    void* pRootObject, const ezRTTI& type, ezDelegate<void(void* pLeafObject, const ezRTTI& pLeafType, const ezAbstractProperty* pProp, const ezVariant& index)> func) const;

  ///\brief Convenience function that writes 'value' to the 'pRootObject' at the current path.
  void SetValue(void* pRootObject, const ezRTTI& type, const ezVariant& value) const;
  ///\brief Convenience function that writes 'value' to the 'pRootObject' at the current path.
  template <typename T>
  EZ_ALWAYS_INLINE void SetValue(T* pRootObject, const ezVariant& value) const
  {
    SetValue(pRootObject, *ezGetStaticRTTI<T>(), value);
  }

  ///\brief Convenience function that reads the value from 'pRootObject' at the current path and stores it in 'out_value'.
  void GetValue(void* pRootObject, const ezRTTI& type, ezVariant& out_value) const;
  ///\brief Convenience function that reads the value from 'pRootObject' at the current path and stores it in 'out_value'.
  template <typename T>
  EZ_ALWAYS_INLINE void GetValue(T* pRootObject, ezVariant& out_value) const
  {
    GetValue(pRootObject, *ezGetStaticRTTI<T>(), out_value);
  }

private:
  struct ResolvedStep
  {
    const ezAbstractProperty* m_pProperty = nullptr;
    ezVariant m_Index;
  };

  static ezResult ResolvePath(void* pCurrentObject, const ezRTTI* pType, const ezArrayPtr<const ResolvedStep> path, bool bWriteToObject,
    const ezDelegate<void(void* pLeaf, const ezRTTI& pType)>& func);

  bool m_bIsValid = false;
  ezHybridArray<ResolvedStep, 2> m_PathSteps;
};
