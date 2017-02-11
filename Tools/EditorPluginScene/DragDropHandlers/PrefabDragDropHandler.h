#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class ezPrefabComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPrefabComponentDragDropHandler, ezComponentDragDropHandler);

protected:

  float CanHandle(const ezDragDropInfo* pInfo) const;
  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;
  virtual void OnDragUpdate(const ezDragDropInfo* pInfo) override;

private:
  void CreatePrefab(const ezVec3& vPosition, const ezUuid& AssetGuid, ezUuid parent, ezInt32 iInsertChildIndex);
};

