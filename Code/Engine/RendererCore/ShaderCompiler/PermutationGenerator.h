#pragma once

#include <RendererCore/Declarations.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/HashedString.h>

/// \brief A helper class to iterate over all possible permutations.
///
/// Just add all permutation variables and their possible values.
/// Then the number of possible permutations and each permutation
/// can be queried.
class EZ_RENDERERCORE_DLL ezPermutationGenerator
{
public:

  /// \brief Resets everything.
  void Clear();

  /// \brief Adds the name and one of the possible values of a permutation variable.
  void AddPermutation(const ezHashedString& sName, const ezHashedString& sValue);

  /// \brief Returns how many permutations are possible.
  ezUInt32 GetPermutationCount() const;

  /// \brief Returns the n-th permutation.
  void GetPermutation(ezUInt32 uiPerm, ezHybridArray<ezPermutationVar, 16>& out_PermVars) const; 

private:

  ezMap<ezHashedString, ezHashSet<ezHashedString> > m_Permutations;
};



