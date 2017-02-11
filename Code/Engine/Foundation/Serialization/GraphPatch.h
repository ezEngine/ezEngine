#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>

class ezRTTI;
class ezAbstractObjectNode;
class ezAbstractObjectGraph;

/// \brief Patch base class for ezAbstractObjectGraph patches.
///
/// Create static instance of derived class to automatically patch graphs on load.
class EZ_FOUNDATION_DLL ezGraphPatch : public ezEnumerable<ezGraphPatch>
{
public:
  /// \brief Constructor. pType is the type to patch. uiTypeVersion is the version to patch to.
  ///
  /// Patches are executed in order from version uiTypeVersion-1 to uiTypeVersion. If no patch exists for previous versions
  /// the input to the patch function can potentially be of a lower version than uiTypeVersion-1.
  ezGraphPatch(const ezRTTI* pType, ezUInt32 uiTypeVersion);

  /// \brief Patches the type pType in pNode to version uiTypeVersion.
  void PatchBaseClass(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode, const ezRTTI* pType, ezUInt32 uiTypeVersion) const;
  /// \brief Patch function. Implementation needs to patch pNode to m_uiTypeVersion.
  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const = 0;
  /// \brief Returns the type to patch.
  const ezRTTI* GetType() const;
  /// \brief Returns the type version to patch to.
  ezUInt32 GetTypeVersion() const;

  EZ_DECLARE_ENUMERABLE_CLASS(ezGraphPatch);

private:
  const ezRTTI* m_pType;
  ezUInt32 m_uiTypeVersion;
};

