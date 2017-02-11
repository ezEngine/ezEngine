#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <QDockWidget>

class EZ_GUIFOUNDATION_DLL ezQtDocumentPanel : public QDockWidget
{
public:
  Q_OBJECT

public:
  ezQtDocumentPanel(QWidget* parent);
  ~ezQtDocumentPanel();

  // prevents closing of the dockwidget, even with Alt+F4
  virtual void closeEvent(QCloseEvent* e) override;

  static const ezDynamicArray<ezQtDocumentPanel*>& GetAllDocumentPanels() { return s_AllDocumentPanels; }

private:
  static ezDynamicArray<ezQtDocumentPanel*> s_AllDocumentPanels;
};
