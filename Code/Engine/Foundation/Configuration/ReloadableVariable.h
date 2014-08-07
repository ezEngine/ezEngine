#pragma once

/// \file

#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

/// \brief Allows to wrap a global/static variable such that its state will be preserved during plugin reloading.
///
/// Using this macro, global and static variables inside plugins can be made 'reloadable', when plugins are unloaded and loaded again.
/// That means their last value is stored when a plugin is unloaded, and it is restored when that same plugin is loaded again,
/// before anyone can access the variable. It happens fully automatically. All that the variable type needs to support, is
/// serialization to a binary stream.
/// \n
/// To make a variable reloadable, just use the macro directly after the variable definition, like so:\n
/// float g_fValue = 0.0f.\n
/// EZ_MAKE_RELOADABLE(float, g_fValue);\n
/// \n
/// \note The state of the reloadable variables is only made persistent over the lifetime of the application and thus only restored
/// when a plugin is reloaded. It is not stored on disk and can thus not be restored when the application is restarted.
/// \n
/// \note This mechanism should be used rarely. It is meant to help in the few cases that one has global variables that need to
/// stay unchanged while code is being reloaded and it cannot be recomputed during plugin initialization. In such cases writing state
/// to disk might be contra-productive, as that might result in state being restored from a file, even though a plugin has actually been
/// loaded for the first time (the save file might still exist due to a crash).
/// Still, try to avoid global variables in general, and only use reloadable variables, if there is no better solution.
#define EZ_MAKE_RELOADABLE(VarType, Variable) \
  static ezReloadableVariableWrapper<VarType> EZ_CONCAT(g_ReloadableVar, EZ_SOURCE_LINE) (Variable, EZ_STRINGIZE(EZ_CONCAT(VarType, Variable)));


/// \brief [internal] A helper class to implement persistent global variables during plugin reloading.
class EZ_FOUNDATION_DLL ezReloadableVariableBase : public ezEnumerable<ezReloadableVariableBase>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezReloadableVariableBase);

public:
  virtual ~ezReloadableVariableBase() { }

  /// \brief Automatically called by ezPlugin whenever a plugin is unloaded, to preserve the current state of all reloadable global variables.
  static void StoreVariables();

protected:
  /// \brief Protected constructor, because this class should never be instantiated directly.
  ezReloadableVariableBase() { }

  /// \brief Called by 'ezReloadableVariableWrapper' to retrieve the stored state of a reloadable variable (if available).
  static void RetrieveVariable(const char* szVarName, ezReloadableVariableBase* pVariable);

  const char* m_szVariableName;

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezReloadableVariableBase);

  /// \brief Overridden by 'ezReloadableVariableWrapper' to implement type specific serialization.
  virtual void SaveState(ezStreamWriterBase& Stream) = 0;
  /// \brief Overridden by 'ezReloadableVariableWrapper' to implement type specific serialization.
  virtual void LoadState(ezStreamReaderBase& Stream) = 0;

  /// \brief This map stores the last state of all known reloadable variables, to allow reading it back again on demand.
  static ezMap<ezString, ezMemoryStreamStorage> s_StoredVariables;
};

/// \brief [internal] Helper class, derived from ezReloadableVariableBase, to implement type specific reloading of global variables.
template<typename Type>
class ezReloadableVariableWrapper : public ezReloadableVariableBase
{
public:
  ezReloadableVariableWrapper(Type& Variable, const char* szVarName);
   
private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezReloadableVariableWrapper);

  virtual void SaveState(ezStreamWriterBase& Stream) override;
  virtual void LoadState(ezStreamReaderBase& Stream) override;

  Type& m_Variable;
};


#include <Foundation/Configuration/Implementation/ReloadableVariable_inl.h>

