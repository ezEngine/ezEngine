#pragma once
#include <EditorFramework/Plugin.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>

class ezGameObjectDocument;
class ezWorldSettingsMsgToEngine;

class EZ_EDITORFRAMEWORK_DLL ezQtGameObjectDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT
public:
  ezQtGameObjectDocumentWindow(ezGameObjectDocument* pDocument);
  ~ezQtGameObjectDocumentWindow();

  ezGameObjectDocument* GetGameObjectDocument() const;

protected:
  ezGlobalSettingsMsgToEngine GetGlobalSettings() const;
  ezWorldSettingsMsgToEngine GetWorldSettings() const;
  ezGridSettingsMsgToEngine GetGridSettings() const;
};


