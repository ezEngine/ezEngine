#pragma once

#include <Core/Input/InputDevice.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Math/Vec2.h>

/// \brief A Virtual Thumbstick is an input device that transforms certain types of input (mouse / touch) into input similar to a thumbstick on a controller.
///
/// A virtual thumbstick can be used to provide an 'input device' on a touch screen, that acts like a controller thumbstick and thus allows
/// easier control over a game. The virtual thumbstick takes input inside a certain screen area. It tracks the users finger movements inside this
/// area and translates those into input from a controller thumbstick, which it then feeds back into the input system. That makes it then possible
/// to be mapped to input actions again.
/// This way a game controller type of input is emulated.
class EZ_CORE_DLL ezVirtualThumbStick : public ezInputDevice
{
public:
  /// \brief Constructor.
  ezVirtualThumbStick();

  /// \brief Destructor.
  ~ezVirtualThumbStick();

  /// \brief Returns "Thumbstick"
  virtual const char* GetDeviceType() const EZ_OVERRIDE { return "Thumbstick"; }

  /// \brief Returns "VirtualThumbstick"
  virtual const char* GetDeviceName() const EZ_OVERRIDE { return "VirtualThumbstick"; }

  /// \brief This enum allows to select either some default input mapping or to select 'Custom'.
  struct Input
  {
    enum Enum
    {
      Touchpoint,     ///< The Virtual Thumbstick will be triggered by touch input events.
      MousePosition,  ///< The Virtual Thumbstick will be triggered by mouse input.
      Custom          ///< The Thumbstick triggers are specified manually.
    };
  };

  /// \brief Specifies which type of output the thumbstick shall generate.
  struct Output
  {
    enum Enum
    {
      Controller0_LeftStick,  ///< The Thumbstick acts like the left stick of controller 0.
      Controller0_RightStick, ///< The Thumbstick acts like the right stick of controller 0.
      Controller1_LeftStick,  ///< The Thumbstick acts like the left stick of controller 1.
      Controller1_RightStick, ///< The Thumbstick acts like the right stick of controller 1.
      Controller2_LeftStick,  ///< The Thumbstick acts like the left stick of controller 2.
      Controller2_RightStick, ///< The Thumbstick acts like the right stick of controller 2.
      Controller3_LeftStick,  ///< The Thumbstick acts like the left stick of controller 3.
      Controller3_RightStick, ///< The Thumbstick acts like the right stick of controller 3.
      Custom                  ///< The thumbstick output is specified manually.
    };
  };

  /// \brief Defines whether the thumbstick center position is locked or relative to where the user started touching it.
  struct CenterMode
  {
    enum Enum
    {
      InputArea,        ///< The center of the thumbstick is always at the center of the input area.
      ActivationPoint   ///< The center of the thumbstick is always where the user activates the thumbstick (first touchpoint)
    };
  };

  /// \brief Defines the area on screen where the thumbstick is located and accepts input.
  ///
  /// \param vLowerLeft
  ///   The lower left corner of the input area. Coordinates are in [0; 1] range.
  ///
  /// \param vUpperRight
  ///   The upper right corner of the input area. Coordinates are in [0; 1] range.
  ///
  /// \param fPriority
  ///   The priority of the input area. Defines which thumbstick or other input action gets priority, if they overlap.
  ///
  /// \param center
  ///   \sa CenterMode. 
  void SetInputArea(const ezVec2& vLowerLeft, const ezVec2& vUpperRight, float fThumbstickRadius, float fPriority, CenterMode::Enum center = CenterMode::ActivationPoint);

  /// \brief Returns the input area of the virtual thumbstick.
  void GetInputArea(ezVec2& out_vLowerLeft, ezVec2& out_vUpperRight);

  /// \brief Specifies from which input slots the thumbstick is activated.
  ///
  /// If \a Input is 'Custom' the remaining parameters define the filter axes and up to three input slots that trigger the thumbstick.
  /// Otherwise the remaining parameters are ignored.
  void SetTriggerInputSlot(Input::Enum Input, const ezInputManager::ezInputActionConfig* pCustomConfig = NULL);

  /// \brief Specifies which output the thumbstick generates.
  ///
  /// If \a Output is 'Custom' the remaining parameters define which input slots the thumbstick triggers for which direction.
  /// Otherwise the remaining parameters are ignored.
  void SetThumbstickOutput(Output::Enum Output, const char* szOutputLeft = NULL, const char* szOutputRight = NULL, const char* szOutputUp = NULL, const char* szOutputDown = NULL);

  /// \brief Specifies what happens when the input slots that trigger the thumbstick are active while entering or leaving the input area.
  void SetAreaFocusMode(ezInputManager::ezInputActionConfig::OnEnterArea OnEnter, ezInputManager::ezInputActionConfig::OnLeaveArea OnLeave);

  /// \brief Allows to enable or disable the entire thumbstick temporarily.
  void SetEnabled(bool bEnabled) { m_bEnabled = bEnabled; }

  /// \brief Returns whether the thumbstick is currently enabled.
  bool IsEnabled() const { return m_bEnabled; }

  /// \brief Returns whether the thumbstick is currently active (ie. triggered) and generates output.
  bool IsActive() const { return m_bIsActive; }

protected:

  void UpdateActionMapping();

  ezVec2 m_vLowerLeft;
  ezVec2 m_vUpperRight;
  float m_fRadius;

  ezInputManager::ezInputActionConfig m_ActionConfig;
  const char* m_szOutputLeft;
  const char* m_szOutputRight;
  const char* m_szOutputUp;
  const char* m_szOutputDown;

  bool m_bEnabled;
  bool m_bConfigChanged;
  bool m_bIsActive;
  ezString m_sName;
  ezVec2 m_vCenter;
  CenterMode::Enum m_CenterMode;

  static ezInt32 s_iThumbsticks;

private:

  virtual void InitializeDevice() EZ_OVERRIDE { }
  virtual void UpdateInputSlotValues() EZ_OVERRIDE;
  virtual void RegisterInputSlots() EZ_OVERRIDE;
};