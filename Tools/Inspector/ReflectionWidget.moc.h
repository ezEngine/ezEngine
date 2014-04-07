#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_ReflectionWidget.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Reflection/Reflection.h>

class ezReflectionWidget : public QDockWidget, public Ui_ReflectionWidget
{
public:
  Q_OBJECT

public:
  ezReflectionWidget(QWidget* parent = 0);

  static ezReflectionWidget* s_pWidget;

private slots:

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  struct PropertyData
  {
    ezString m_sType;
    ezString m_sPropertyName;
    ezInt8 m_iCategory;
  };

  struct TypeData
  {
    TypeData()
    {
      m_pTreeItem = nullptr;
    }

    QTreeWidgetItem* m_pTreeItem;

    ezUInt32 m_uiSize;
    ezString m_sParentType;
    ezString m_sPlugin;

    ezHybridArray<PropertyData, 16> m_Properties;
  };

  bool UpdateTree();

  ezMap<ezString, TypeData> m_Types;
};


