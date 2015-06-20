#pragma once

#include <GuiFoundation/Basics.h>
#include <QDockWidget>
#include <Foundation/Containers/DynamicArray.h>

class ezContainerWindow;

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

class EZ_GUIFOUNDATION_DLL ezApplicationPanel : public QDockWidget
{
public:
  Q_OBJECT

public:
  ezApplicationPanel();
  ~ezApplicationPanel();

  void EnsureVisible();

  static const ezDynamicArray<ezApplicationPanel*>& GetAllApplicationPanels() { return s_AllApplicationPanels; }

  // prevents closing of the dockwidget, even with Alt+F4
  virtual void closeEvent(QCloseEvent* e) override;

private slots:

private:
  friend class ezContainerWindow;

  static ezDynamicArray<ezApplicationPanel*> s_AllApplicationPanels;

  ezContainerWindow* m_pContainerWindow;
};

