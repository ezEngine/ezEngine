#pragma once

#include <RendererCore/Declarations.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/HashedString.h>

class EZ_RENDERERCORE_DLL ezPermutationGenerator
{
public:

  void AddPermutation(const ezHashedString& sName, const ezHashedString& sValue);

  ezUInt32 GetPermutationCount() const;
  void GetPermutation(ezUInt32 uiPerm, ezHybridArray<ezPermutationVar, 16>& out_PermVars) const; 

private:

  ezMap<ezHashedString, ezHashSet<ezHashedString> > m_Permutations;
};



