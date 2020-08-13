#pragma once

#include <EditorFramework/DragDrop/AssetDragDropHandler.h>

class ezDocument;
class ezDragDropInfo;

class EZ_EDITORFRAMEWORK_DLL ezComponentDragDropHandler : public ezAssetDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezComponentDragDropHandler, ezAssetDragDropHandler);

protected:
  void CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const ezVariant& value, ezUuid parent, ezInt32 iInsertChildIndex);

  void AttachComponentToObject(const char* szType, const char* szProperty, const ezVariant& value, ezUuid ObjectGuid);

  void MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition, const ezQuat& qRotation);

  void MoveDraggedObjectsToPosition(ezVec3 vPosition, bool bAllowSnap, const ezVec3& normal);

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

  ezVec3 m_vAlignAxisWithNormal = ezVec3::ZeroVector();
};
