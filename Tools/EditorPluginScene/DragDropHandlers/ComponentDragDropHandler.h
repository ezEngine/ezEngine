#pragma once

#include <EditorPluginScene/Plugin.h>
#include <EditorPluginScene/DragDropHandlers/AssetDragDropHandler.h>

class ezDocument;

class ezComponentDragDropHandler : public ezAssetDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezComponentDragDropHandler, ezAssetDragDropHandler);

public:

protected:
  void CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const char* szValue);

  void MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition);

  void MoveDraggedObjectsToPosition(ezVec3 vPosition, bool bAllowSnap);

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

  virtual float CanHandle(const ezDragDropInfo* pInfo) const override;

};

