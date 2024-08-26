#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QDockWidget>

class ezDocument;

class EZ_GUIFOUNDATION_DLL ezQtDocumentPanel : public QDockWidget
{
public:
  Q_OBJECT

public:
  ezQtDocumentPanel(QWidget* pParent, ezDocument* pDocument);
  ~ezQtDocumentPanel();

  // prevents closing of the dockwidget, even with Alt+F4
  virtual void closeEvent(QCloseEvent* e) override;
  virtual bool event(QEvent* pEvent) override;

  static const ezDynamicArray<ezQtDocumentPanel*>& GetAllDocumentPanels() { return s_AllDocumentPanels; }

private:
  ezDocument* m_pDocument = nullptr;

  static ezDynamicArray<ezQtDocumentPanel*> s_AllDocumentPanels;
};
