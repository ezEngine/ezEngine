#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class ezSoundEventComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSoundEventComponentDragDropHandler, ezComponentDragDropHandler);

public:

  float CanHandle(const ezDragDropInfo* pInfo) const;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;


};

