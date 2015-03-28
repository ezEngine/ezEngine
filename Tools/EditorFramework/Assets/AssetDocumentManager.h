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
};
