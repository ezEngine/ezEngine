#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <QDockWidget>
#include <Projects/Inspector/ui_DataTransferWidget.h>

class ezDataWidget : public QDockWidget, public Ui_DataTransferWidget
{
public:
  Q_OBJECT

public:
  ezDataWidget(QWidget* parent = 0);

  static ezDataWidget* s_pWidget;

private slots:
  virtual void on_ButtonRefresh_clicked();
  virtual void on_ComboTransfers_currentIndexChanged(int index);
  virtual void on_ComboItems_currentIndexChanged(int index);

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  struct TransferDataObject
  {
    ezString m_sMimeType;
    ezMemoryStreamStorage m_Storage;
  };

  struct TransferData
  {
    ezMap<ezString, TransferDataObject> m_Items;
  };



  ezMap<ezString, TransferData> m_Transfers;
};


