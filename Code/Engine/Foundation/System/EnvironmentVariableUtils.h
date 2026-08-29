#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

/// This is a helper class to interact with environment variables.
class EZ_FOUNDATION_DLL ezEnvironmentVariableUtils
{
public:
  /// Returns the current value of the request environment variable. If it isn't set szDefault will be returned.
  static ezString GetValueString(ezStringView sName, ezStringView sDefault = nullptr);

  /// Sets the environment variable for the current execution environment (i.e. this process and child processes created after this call).
  static ezResult SetValueString(ezStringView sName, ezStringView sValue);

  /// Returns the current value of the request environment variable. If it isn't set iDefault will be returned.
  static ezInt32 GetValueInt(ezStringView sName, ezInt32 iDefault = -1);

  /// Sets the environment variable for the current execution environment.
  static ezResult SetValueInt(ezStringView sName, ezInt32 iValue);

  /// Returns true if the environment variable with the given name is set, false otherwise.
  static bool IsVariableSet(ezStringView sName);

  /// Removes an environment variable from the current execution context (i.e. this process and child processes created after this call).
  static ezResult UnsetVariable(ezStringView sName);

private:
  /// [internal]
  static ezString GetValueStringImpl(ezStringView sName, ezStringView sDefault);

  /// [internal]
  static ezResult SetValueStringImpl(ezStringView sName, ezStringView sValue);

  /// [internal]
  static bool IsVariableSetImpl(ezStringView sName);

  /// [internal]
  static ezResult UnsetVariableImpl(ezStringView sName);
};
