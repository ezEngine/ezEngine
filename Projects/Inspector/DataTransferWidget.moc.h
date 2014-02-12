#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/DynamicArray.h>
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
  virtual void on_DataTable_itemSelectionChanged();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  struct TransferDataObject
  {
    ezString m_sMimeType;
    ezString m_sText;
    ezUInt32 m_uiWidth;
    ezUInt32 m_uiHeight;
    ezDynamicArray<ezUInt8> m_Image;
  };

  struct TransferData
  {
    TransferData()
    {
      m_iRow = -1;
      m_pItem = NULL;
    }

    ezInt32 m_iRow;
    QTableWidgetItem* m_pItem;

    ezMap<ezString, TransferDataObject> m_Objects;
  };



  ezMap<ezString, TransferData> m_Data;
};


