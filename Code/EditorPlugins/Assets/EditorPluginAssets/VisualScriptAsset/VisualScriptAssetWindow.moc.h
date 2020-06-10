#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtVisualScriptAssetScene;
class ezQtNodeView;
struct ezVisualScriptInstanceActivity;
class ezVisualScriptAssetDocument;

class EZ_EDITORPLUGINASSETS_DLL ezQtVisualScriptAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtVisualScriptAssetDocumentWindow(ezDocument* pDocument, const ezDocumentObject* pOpenContext);
  ~ezQtVisualScriptAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "VisualScriptAsset"; }

  ezVisualScriptAssetDocument* GetVisualScriptDocument();

  void PickDebugTarget();

private Q_SLOTS:

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

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
