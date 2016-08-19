#pragma once

#include <EditorFramework/DragDrop/DragDropHandler.h>

class ezDocument;

class EZ_EDITORFRAMEWORK_DLL ezAssetDragDropHandler : public ezDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDragDropHandler, ezDragDropHandler);

public:

protected:
  bool IsAssetType(const ezDragDropInfo* pInfo) const;

  ezString GetAssetGuidString(const ezDragDropInfo* pInfo) const;

  ezUuid GetAssetGuid(const ezDragDropInfo* pInfo) const
  {
    return ezConversionUtils::ConvertStringToUuid(GetAssetGuidString(pInfo));
  }

  ezString GetAssetTypeName(const ezUuid& assetTypeGuid) const;

  bool IsSpecificAssetType(const ezDragDropInfo* pInfo, const char* szType) const;

  ezDocument* m_pDocument;


};

