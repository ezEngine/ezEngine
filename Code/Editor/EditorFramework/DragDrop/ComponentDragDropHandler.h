#pragma once

#include <EditorFramework/DragDrop/AssetDragDropHandler.h>

class ezDocument;
class ezDragDropInfo;

class EZ_EDITORFRAMEWORK_DLL ezComponentDragDropHandler : public ezAssetDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezComponentDragDropHandler, ezAssetDragDropHandler);

public:

protected:
  void CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const char* szValue, ezUuid parent, ezInt32 iInsertChildIndex);

  void AttachComponentToObject(const char* szType, const char* szProperty, const char* szValue, ezUuid ObjectGuid);

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

