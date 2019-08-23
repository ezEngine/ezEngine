#pragma once

#include <Foundation/Types/Delegate.h>
#include <ProcGenPlugin/Declarations.h>

namespace ezExpression
{
  typedef ezArrayPtr<ezSimdVec4f> Output;
  typedef ezArrayPtr<ezArrayPtr<const ezSimdVec4f>> Inputs; // Inputs are in SOA form, means inner array contains all values for one input parameter, one for each instance.
  typedef ezHashTable<ezHashedString, ezVariant> GlobalData;
} // namespace ezExpression

/// \brief defines an external function that can be called in expressions.
///  These functions need to be state-less and thread-safe.
typedef ezDelegate<void(ezExpression::Inputs, ezExpression::Output, const ezExpression::GlobalData&)> ezExpressionFunction;

/// \brief defines an optional validation function used to validate required global data for an expression function
typedef ezDelegate<ezResult(const ezExpression::GlobalData&)> ezExpressionValidateGlobalData;

struct EZ_PROCGENPLUGIN_DLL ezDefaultExpressionFunctions
{
  static void Random(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData);
  static void PerlinNoise(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData);
};
