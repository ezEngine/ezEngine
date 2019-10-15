#pragma once

#include <Core/CoreDLL.h>
#include <Core/Scripting/DuktapeHelper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

class EZ_CORE_DLL ezDuktapeFunction final : public ezDuktapeHelper
{
public:
  ezDuktapeFunction(duk_context* pExistingContext, ezInt32 iExpectedStackChange);
  ~ezDuktapeFunction();

  /// \name Retrieving function parameters
  ///@{

  /// Returns how many Parameters were passed to the called C-Function.
  ezUInt32 GetNumVarArgFunctionParameters() const;

  ezInt16 GetFunctionMagicValue() const;

  ///@}

  /// \name Returning values from C function
  ///@{

  ezInt32 ReturnVoid();
  ezInt32 ReturnNull();
  ezInt32 ReturnUndefined();
  ezInt32 ReturnBool(bool value);
  ezInt32 ReturnInt(ezInt32 value);
  ezInt32 ReturnUInt(ezUInt32 value);
  ezInt32 ReturnFloat(float value);
  ezInt32 ReturnNumber(double value);
  ezInt32 ReturnString(const char* value);
  ezInt32 ReturnCustom();

  ///@}

private:
  bool m_bDidReturnValue = false;
};

#endif
