#pragma once

#include <EditorPluginScene/Plugin.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>

class ezDocument;

class ezComponentDragDropHandler : public ezDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezComponentDragDropHandler, ezDragDropHandler);

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

  void CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const char* szValue);

  void MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition);

  void MoveDraggedObjectsToPosition(ezVec3 vPosition);

  void SelectCreatedObjects();

  void BeginTemporaryCommands();

  void EndTemporaryCommands();

  void CancelTemporaryCommands();

  ezDocument* m_pDocument;
  ezHybridArray<ezUuid, 16> m_DraggedObjects;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;

  virtual void OnDragUpdate(const ezDragDropInfo* pInfo) override;

  virtual void OnDragCancel() override;

  virtual void OnDrop(const ezDragDropInfo* pInfo) override;
};

