#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtTypeScriptAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtTypeScriptAssetDocumentWindow(ezAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const { return "TypeScriptAsset"; }

private:
  ezTypeScriptAssetDocument* m_pAssetDoc = nullptr;
  QLabel* m_pLabelInfo = nullptr;
};
