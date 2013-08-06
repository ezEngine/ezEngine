#pragma once

#include <Core/Input/InputDevice.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Math/Vec2.h>

class EZ_CORE_DLL ezVirtualThumbStick : public ezInputDevice
{
public:
  ezVirtualThumbStick();
  ~ezVirtualThumbStick();

  virtual const char* GetDeviceType() const EZ_OVERRIDE { return "Thumbstick"; }

  virtual const char* GetDeviceName() const EZ_OVERRIDE { return "VirtualThumbstick"; }

  void SetInputArea(const ezVec2& vLowerLeft, const ezVec2& vUpperRight);
  void GetInputArea(ezVec2& out_vLowerLeft, ezVec2& out_vUpperRight);

  struct InputTrigger
  {
    enum Enum
    {
      Touchpoint,
      Custom
    };
  };

  struct OutputTrigger
  {
    enum Enum
    {
      Controller0_LeftStick,
      Controller0_RightStick,
      Custom
    };
  };

  void SetTriggerInputSlot(InputTrigger::Enum Input, const char* szFilterAxisX = NULL, const char* szFilterAxisY = NULL, const char* szTrigger1 = NULL, const char* szTrigger2 = NULL, const char* szTrigger3 = NULL);

  void SetThumbstickOutput(OutputTrigger::Enum Output, const char* szOutputLeft = NULL, const char* szOutputRight = NULL, const char* szOutputUp = NULL, const char* szOutputDown = NULL);

  void SetAreaFocusMode(ezInputManager::ezInputActionConfig::OnEnterArea OnEnter, ezInputManager::ezInputActionConfig::OnLeaveArea OnLeave);

  void SetEnabled(bool bEnabled) { m_bEnabled = bEnabled; }

  bool IsEnabled() const { return m_bEnabled; }

private:

  void UpdateActionMapping();

  virtual void InitializeDevice() EZ_OVERRIDE { }

  virtual void UpdateInputSlotValues() EZ_OVERRIDE;

  virtual void RegisterInputSlots() EZ_OVERRIDE;

  ezVec2 m_vLowerLeft;
  ezVec2 m_vUpperRight;

  ezInputManager::ezInputActionConfig::OnEnterArea m_OnEnter;
  ezInputManager::ezInputActionConfig::OnLeaveArea m_OnLeave;

  const char* m_szFilterAxisX;
  const char* m_szFilterAxisY;
  const char* m_szTrigger1;
  const char* m_szTrigger2;
  const char* m_szTrigger3;

  const char* m_szOutputLeft;
  const char* m_szOutputRight;
  const char* m_szOutputUp;
  const char* m_szOutputDown;

  static ezInt32 s_iThumbsticks;

  bool m_bEnabled;
  ezString m_sName;
};