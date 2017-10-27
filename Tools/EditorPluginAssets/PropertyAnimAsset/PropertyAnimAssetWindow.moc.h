#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtPropertyAnimModel;
class QTreeView;

class ezQtPropertyAnimAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtPropertyAnimAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtPropertyAnimAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "PropertyAnimAsset"; }

  QTreeView* m_pPropertyTreeView = nullptr;
  ezQtPropertyAnimModel* m_pPropertiesModel;
};
