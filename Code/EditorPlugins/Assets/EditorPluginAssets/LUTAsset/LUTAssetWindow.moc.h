#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezLUTAssetDocument;

class ezQtLUTAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtLUTAssetDocumentWindow(ezLUTAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "LUTAsset"; }
};

class ezLUTAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(ezStringView sMapping);
};
