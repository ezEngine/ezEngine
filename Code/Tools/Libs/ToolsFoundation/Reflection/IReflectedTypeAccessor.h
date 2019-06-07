#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

class ezDocumentObject;
struct ezStatus;

/// \brief Provides access to the properties of an ezRTTI compatible data storage.
class EZ_TOOLSFOUNDATION_DLL ezIReflectedTypeAccessor
{
public:
  /// \brief Constructor for the ezIReflectedTypeAccessor.
  ///
  /// It is a valid implementation to pass an invalid handle. Note that in this case there is no way to determine
  /// what is actually stored inside. However, it can be useful to use e.g. the ezReflectedTypeDirectAccessor
  /// to set properties on the engine runtime side without having the ezPhantomRttiManager initialized.
  ezIReflectedTypeAccessor(const ezRTTI* pRtti, ezDocumentObject* pOwner) : m_pRtti(pRtti), m_pOwner(pOwner) {} // [tested]

  /// \brief Returns the ezRTTI* of the wrapped instance type.
  const ezRTTI* GetType() const { return m_pRtti; } // [tested]

  /// \brief Returns the value of the property defined by its path. Return value is invalid iff the path was invalid.
  virtual const ezVariant GetValue(const char* szProperty, ezVariant index = ezVariant(), ezStatus* res = nullptr) const = 0;

  /// \brief Sets a property defined by its path to the given value. Returns whether the operation was successful.
  virtual bool SetValue(const char* szProperty, const ezVariant& value, ezVariant index = ezVariant()) = 0;

  virtual ezInt32 GetCount(const char* szProperty) const = 0;
  virtual bool GetKeys(const char* szProperty, ezHybridArray<ezVariant, 16>& out_keys) const = 0;

  virtual bool InsertValue(const char* szProperty, ezVariant index, const ezVariant& value) = 0;
  virtual bool RemoveValue(const char* szProperty, ezVariant index) = 0;
  virtual bool MoveValue(const char* szProperty, ezVariant oldIndex, ezVariant newIndex) = 0;

  virtual ezVariant GetPropertyChildIndex(const char* szProperty, const ezVariant& value) const = 0;

  const ezDocumentObject* GetOwner() const { return m_pOwner; }

  bool GetValues(const char* szProperty, ezHybridArray<ezVariant, 16>& out_values) const;


private:
  friend class ezDocumentObjectManager;
  friend class ezDocumentObject;

  const ezRTTI* m_pRtti;
  ezDocumentObject* m_pOwner;
};

