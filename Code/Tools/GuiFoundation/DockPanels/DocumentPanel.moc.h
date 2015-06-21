#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <QDockWidget>

class EZ_GUIFOUNDATION_DLL ezDocumentPanel : public QDockWidget
{
public:
  Q_OBJECT

public:
  ezDocumentPanel(QWidget* parent);
  ~ezDocumentPanel();

  // prevents closing of the dockwidget, even with Alt+F4
  virtual void closeEvent(QCloseEvent* e) override;

  static const ezDynamicArray<ezDocumentPanel*>& GetAllDocumentPanels() { return s_AllDocumentPanels; }

private:
  static ezDynamicArray<ezDocumentPanel*> s_AllDocumentPanels;
};
