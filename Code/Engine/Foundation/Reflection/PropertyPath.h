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

///\brief Stores a path from an object of a given type to a property inside of it.
/// Once initialized to a specific path, the target property/object of the path can be read or written on
/// multiple root objects.
/// An empty path is allowed in which case WriteToLeafObject/ReadFromLeafObject will return pRootObject directly.
///
/// TODO: read/write methods and ResolvePath should return a failure state.
class EZ_FOUNDATION_DLL ezPropertyPath
{
public:
  ezPropertyPath();
  ~ezPropertyPath();

  /// \brief Returns true if InitializeFromPath() has been successfully called and it is therefore possible to use the other functions.
  bool IsValid() const;

  ///\brief Resolves a path in the syntax 'propertyName[index]/propertyName[index]/...' into steps.
  /// The '[index]' part is only added for properties that require indices (arrays and maps).
  ezResult InitializeFromPath(const ezRTTI& rootObjectRtti, const char* szPath);
  ///\brief Resolves a path provided as an array of ezPropertyPathStep.
  ezResult InitializeFromPath(const ezRTTI& rootObjectRtti, const ezArrayPtr<const ezPropertyPathStep> path);

  ///\brief Applies the entire path and allows writing to the target object.
  void WriteToLeafObject(void* pRootObject, const ezRTTI& pType, ezDelegate<void(void* pLeaf, const ezRTTI& pType)> func) const;
  ///\brief Applies the entire path and allows reading from the target object.
  void ReadFromLeafObject(void* pRootObject, const ezRTTI& pType, ezDelegate<void(void* pLeaf, const ezRTTI& pType)> func) const;

  ///\brief Applies the path up to the last step and allows a functor to write to the final property.
  void WriteProperty(void* pRootObject, const ezRTTI& pType,
                     ezDelegate<void(void* pLeaf, ezAbstractProperty* pProp, const ezVariant& index)> func) const;
  ///\brief Applies the path up to the last step and allows a functor to read from the final property.
  void ReadProperty(void* pRootObject, const ezRTTI& pType,
                    ezDelegate<void(void* pLeaf, const ezAbstractProperty* pProp, const ezVariant& index)> func) const;

  ///\brief Convenience function that writes 'value' to the 'pRootObject' at the current path.
  void SetValue(void* pRootObject, const ezRTTI& pType, const ezVariant& value) const;
  ///\brief Convenience function that writes 'value' to the 'pRootObject' at the current path.
  template <typename T>
  EZ_ALWAYS_INLINE void SetValue(T* pRootObject, const ezVariant& value) const
  {
    SetValue(pRootObject, *ezGetStaticRTTI<T>(), value);
  }

  ///\brief Convenience function that reads the value from 'pRootObject' at the current path and stores it in 'out_value'.
  void GetValue(void* pRootObject, const ezRTTI& pType, ezVariant& out_value) const;
  ///\brief Convenience function that reads the value from 'pRootObject' at the current path and stores it in 'out_value'.
  template <typename T>
  EZ_ALWAYS_INLINE void GetValue(T* pRootObject, ezVariant& out_value) const
  {
    GetValue(pRootObject, *ezGetStaticRTTI<T>(), out_value);
  }

private:
  struct ResolvedStep
  {
    ezAbstractProperty* m_pProperty = nullptr;
    ezVariant m_Index;
  };

  static void ResolvePath(void* pCurrentObject, const ezRTTI* pType, const ezArrayPtr<const ResolvedStep> path, bool bWriteToObject,
                          const ezDelegate<void(void* pLeaf, const ezRTTI& pType)>& func);

  bool m_bIsValid = false;
  ezHybridArray<ResolvedStep, 2> m_PathSteps;
};
