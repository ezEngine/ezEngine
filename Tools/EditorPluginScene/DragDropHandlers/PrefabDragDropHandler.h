#pragma once

#include <EditorPluginScene/DragDropHandlers/ComponentDragDropHandler.h>

class ezPrefabComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPrefabComponentDragDropHandler, ezComponentDragDropHandler);

public:

  bool CanHandle(const ezDragDropInfo* pInfo) const;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;

private:
  void CreatePrefab(const ezVec3& vPosition, const ezUuid& AssetGuid);
};

