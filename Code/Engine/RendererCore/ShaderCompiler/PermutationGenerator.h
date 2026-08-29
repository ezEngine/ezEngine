#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

/// A helper class to iterate over all possible permutations.
///
/// Just add all permutation variables and their possible values.
/// Then the number of possible permutations and each permutation
/// can be queried.
class EZ_RENDERERCORE_DLL ezPermutationGenerator
{
public:
  /// Resets everything.
  void Clear();

  /// Removes all permutations for the given variable
  void RemovePermutations(const ezHashedString& sPermVarName);

  /// Adds the name and one of the possible values of a permutation variable.
  void AddPermutation(const ezHashedString& sName, const ezHashedString& sValue);

  /// Returns how many permutations are possible.
  ezUInt32 GetPermutationCount() const;

  /// Returns the n-th permutation.
  void GetPermutation(ezUInt32 uiPerm, ezDynamicArray<ezPermutationVar>& out_permVars) const;

private:
  ezMap<ezHashedString, ezHashSet<ezHashedString>> m_Permutations;
};
