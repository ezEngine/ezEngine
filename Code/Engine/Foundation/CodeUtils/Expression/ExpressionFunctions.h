#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>

namespace ezExpression
{
  using Output = ezArrayPtr<ezSimdVec4f>;
  using Inputs = ezArrayPtr<ezArrayPtr<const ezSimdVec4f>>; // Inputs are in SOA form, means inner array contains all values for one input parameter, one for each instance.
  using GlobalData = ezHashTable<ezHashedString, ezVariant>;
} // namespace ezExpression

/// \brief defines an external function that can be called in expressions.
///  These functions need to be state-less and thread-safe.
using ezExpressionFunction = ezDelegate<void(ezExpression::Inputs, ezExpression::Output, const ezExpression::GlobalData&)>;

/// \brief defines an optional validation function used to validate required global data for an expression function
using ezExpressionValidateGlobalData = ezDelegate<ezResult(const ezExpression::GlobalData&)>;

struct EZ_FOUNDATION_DLL ezDefaultExpressionFunctions
{
  static void Random(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData);
  static void PerlinNoise(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData);
};
