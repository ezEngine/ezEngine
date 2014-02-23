#pragma once

#include <Foundation/Basics.h>
#include <QDockWidget>
#include <Tools/Inspector/ui_InputWidget.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Core/Input/InputManager.h>

class ezInputWidget : public QDockWidget, public Ui_InputWidget
{
public:
  Q_OBJECT

public:
  ezInputWidget(QWidget* parent = 0);

  static ezInputWidget* s_pWidget;

private slots:
  virtual void on_ButtonClearSlots_clicked();
  virtual void on_ButtonClearActions_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  void ClearSlots();
  void ClearActions();

  void UpdateSlotTable(bool bRecreate);
  void UpdateActionTable(bool bRecreate);

  struct SlotData
  {
    ezInt32 m_iTableRow;
    ezUInt16 m_uiSlotFlags;
    ezKeyState::Enum m_KeyState;
    float m_fValue;
    float m_fDeadZone;

    SlotData()
    {
      m_iTableRow = -1;
      m_uiSlotFlags = 0;
      m_KeyState = ezKeyState::Up;
      m_fValue = 0;
      m_fDeadZone = 0;
    }
  };

  ezMap<ezString, SlotData> m_InputSlots;

  struct ActionData
  {
    ezInt32 m_iTableRow;
    ezKeyState::Enum m_KeyState;
    float m_fValue;
    bool m_bUseTimeScaling;

    ezString m_sTrigger[ezInputActionConfig::MaxInputSlotAlternatives];
    float m_fTriggerScaling[ezInputActionConfig::MaxInputSlotAlternatives];

    ActionData()
    {
      m_iTableRow = -1;
      m_KeyState = ezKeyState::Up;
      m_fValue = 0;
      m_bUseTimeScaling = false;
    }
  };

  ezMap<ezString, ActionData> m_InputActions;
};


