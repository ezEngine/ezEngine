#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ads/DockWidget.h>

class ezDocument;

class EZ_GUIFOUNDATION_DLL ezQtDocumentPanel : public ads::CDockWidget
{
public:
  Q_OBJECT

public:
  ezQtDocumentPanel(ads::CDockManager* pDockManager, QWidget* pParent, ezDocument* pDocument);
  ~ezQtDocumentPanel();

  virtual bool event(QEvent* pEvent) override;

private:
  ezDocument* m_pDocument = nullptr;
};
