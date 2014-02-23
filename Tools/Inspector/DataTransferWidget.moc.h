#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_DataTransferWidget.h>

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
  virtual void on_ButtonSave_clicked();
  virtual void on_ButtonOpen_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  struct TransferDataObject
  {
    ezString m_sMimeType;
    ezString m_sExtension;
    ezMemoryStreamStorage m_Storage;
    ezString m_sFileName;
  };

  struct TransferData
  {
    ezMap<ezString, TransferDataObject> m_Items;
  };

  bool SaveToFile(TransferDataObject& item, const char* szFile);

  TransferDataObject* GetCurrentItem();
  TransferData* GetCurrentTransfer();

  ezMap<ezString, TransferData> m_Transfers;
};


