#pragma once

#include <Foundation/Basics.h>
#include <EditorPluginAssets/Plugin.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/Preferences/Preferences.h>

class ezQtVisualScriptAssetScene;
class ezQtNodeView;
struct ezVisualScriptInstanceActivity;

class EZ_EDITORPLUGINASSETS_DLL ezQtVisualScriptAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtVisualScriptAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtVisualScriptAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "VisualScriptAsset"; }

  void PickDebugTarget();

private slots:

private:
  ezQtVisualScriptAssetScene* m_pScene;
  ezQtNodeView* m_pView;
};

class EZ_EDITORPLUGINASSETS_DLL ezVisualScriptPreferences : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptPreferences, ezPreferences);
public:
  ezVisualScriptPreferences();

  ezUuid m_DebugObject;
};

