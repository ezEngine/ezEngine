#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

class ezDocumentObjectBase;

/// \brief Provides access to the properties of an ezRTTI compatible data storage.
class EZ_TOOLSFOUNDATION_DLL ezIReflectedTypeAccessor
{
public:
  /// \brief Constructor for the ezIReflectedTypeAccessor.
  ///
  /// It is a valid implementation to pass an invalid handle. Note that in this case there is no way to determine
  /// what is actually stored inside. However, it can be useful to use e.g. the ezReflectedTypeDirectAccessor
  /// to set properties on the engine runtime side without having the ezPhantomRttiManager initialized.
  ezIReflectedTypeAccessor(const ezRTTI* pRtti, ezDocumentObjectBase* pOwner) : m_pRtti(pRtti), m_pOwner(pOwner) {} // [tested]

  /// \brief Returns the ezRTTI* of the wrapped instance type.
  const ezRTTI* GetType() const { return m_pRtti; } // [tested]

  /// \brief Returns the value of the property defined by its path. Return value is invalid iff the path was invalid.
  virtual const ezVariant GetValue(const ezPropertyPath& path, ezVariant index = ezVariant()) const = 0;

  /// \brief Sets a property defined by its path to the given value. Returns whether the operation was successful.
  virtual bool SetValue(const ezPropertyPath& path, const ezVariant& value, ezVariant index = ezVariant()) = 0;

  virtual ezInt32 GetCount(const ezPropertyPath& path) const = 0;
  virtual bool GetKeys(const ezPropertyPath& path, ezHybridArray<ezVariant, 16>& out_keys) const = 0;

  virtual bool InsertValue(const ezPropertyPath& path, ezVariant index, const ezVariant& value) = 0;
  virtual bool RemoveValue(const ezPropertyPath& path, ezVariant index) = 0;
  virtual bool MoveValue(const ezPropertyPath& path, ezVariant oldIndex, ezVariant newIndex) = 0;

  virtual ezVariant GetPropertyChildIndex(const ezPropertyPath& path, const ezVariant& value) const = 0;

  const ezDocumentObjectBase* GetOwner() const { return m_pOwner; }

private:
  friend class ezDocumentObjectManager;
  friend class ezDocumentObjectBase;

  const ezRTTI* m_pRtti;
  ezDocumentObjectBase* m_pOwner;
};

