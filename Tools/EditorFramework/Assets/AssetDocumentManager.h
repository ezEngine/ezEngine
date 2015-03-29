#pragma once

#include <EditorFramework/Plugin.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Basics/Status.h>

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentManager : public ezDocumentManagerBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentManager);

public:
  ezAssetDocumentManager() {};
  ~ezAssetDocumentManager() {};

  virtual ezString GetResourceTypeExtension() const = 0;

  static bool IsResourceUpToDate(ezUInt64 uiHash, const char* szResourceFile);
  ezString GenerateResourceFileName(const char* szDocumentPath, const char* szPlatform) const;
  ezString GenerateRelativeResourceFileName(const char* szDocumentPath) const;
};
