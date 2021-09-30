#pragma once

#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>

class ezScene2Document;

class ezQtScene2DocumentWindow : public ezQtSceneDocumentWindowBase
{
  Q_OBJECT

public:
  ezQtScene2DocumentWindow(ezScene2Document* pDocument);
  ~ezQtScene2DocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "Scene2"; }
  virtual bool InternalCanCloseWindow();

  ezStatus SaveAllLayers();
};
