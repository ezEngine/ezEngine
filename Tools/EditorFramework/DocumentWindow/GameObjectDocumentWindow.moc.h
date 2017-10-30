#pragma once
#include <EditorFramework/Plugin.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>

class ezGameObjectDocument;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT
public:
  ezQtGameObjectDocumentWindow(ezGameObjectDocument* pDocument);
  ~ezQtGameObjectDocumentWindow();

  ezGameObjectDocument* GetGameObjectDocument() const;
};


