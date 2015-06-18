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

private slots:

private:
  friend class ezContainerWindow;

  static ezDynamicArray<ezApplicationPanel*> s_AllApplicationPanels;

  ezContainerWindow* m_pContainerWindow;
};

