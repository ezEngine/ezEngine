#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

/// \brief Provides access to the properties of an ezReflectedType compatible data storage.
class EZ_TOOLSFOUNDATION_DLL ezIReflectedTypeAccessor
{
public:
  /// \brief Constructor for the ezIReflectedTypeAccessor.
  ///
  /// It is a valid implementation to pass an invalid handle. Note that in this case there is no way to determine
  /// what is actually stored inside. However, it can be useful to use e.g. the ezReflectedTypeDirectAccessor
  /// to set properties on the engine runtime side without having the ezReflectedTypeManager initialized.
  ezIReflectedTypeAccessor(ezReflectedTypeHandle hReflectedType) : m_hReflectedType(hReflectedType) {} // [tested]

  /// \brief Returns the ezReflectedTypeHandle of the wrapped instance type.
  ezReflectedTypeHandle GetReflectedTypeHandle() const { return m_hReflectedType; } // [tested]

  /// \brief Returns the value of the property defined by its path. Return value is invalid iff the path was invalid.
  virtual const ezVariant GetValue(const ezPropertyPath& path) const = 0;

  virtual const ezVariant GetValue(const char* szPath) const;

  /// \brief Sets a property defined by its path to the given value. Returns whether the operation was successful.
  virtual bool SetValue(const ezPropertyPath& path, const ezVariant& value) = 0;

  virtual bool SetValue(const char* szPath, const ezVariant& value);

private:
  ezReflectedTypeHandle m_hReflectedType;
};

