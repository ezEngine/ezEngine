#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class ezMiniAudioSoundComponentDragDropHandler : public ezComponentDragDropHandler
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMiniAudioSoundComponentDragDropHandler, ezComponentDragDropHandler);

public:
  float CanHandle(const ezDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const ezDragDropInfo* pInfo) override;
};
