#pragma once

#include <ProceduralPlacementPlugin/Basics.h>
#include <Foundation/Types/Delegate.h>

namespace ezExpression
{
  typedef ezArrayPtr<ezSimdVec4f> Output;
  typedef ezArrayPtr<ezArrayPtr<const ezSimdVec4f>> Inputs; // Inputs are in SOA form, means inner array contains all values for one input parameter, one for each instance.
  typedef ezArrayPtr<const ezUInt8> UserData;
}

/// \brief defines an external function that can be called in expressions.
///  These functions need to be state-less and thread-safe.
typedef ezDelegate<void(ezExpression::Inputs, ezExpression::Output, ezExpression::UserData)> ezExpressionFunction;

class EZ_PROCEDURALPLACEMENTPLUGIN_DLL ezExpressionFunctionRegistry
{
public:
  static bool RegisterFunction(const char* szName, ezExpressionFunction func);

  static const char* GetName(ezUInt32 uiNameHash);
  static ezExpressionFunction GetFunction(ezUInt32 uiNameHash);
  static ezExpressionFunction GetFunction(const char* szName);
};

#define EZ_REGISTER_EXPRESSION_FUNCTION(name, func) \
  static bool EZ_CONCAT(s_bExpRegisterDummy, EZ_SOURCE_LINE) = ezExpressionFunctionRegistry::RegisterFunction(name, func)
