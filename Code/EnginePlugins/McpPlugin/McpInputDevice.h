#pragma once

#include <McpPlugin/McpPluginDLL.h>

#include <Core/Input/InputDevice.h>

/// \brief A synthetic input device that writes whatever an agent tells it to.
///
/// A real ezInputDevice rather than ezInputManager::InjectInputSlotValue(), for three reasons that all
/// fall out of how the manager works:
///
///  - GatherDeviceInputSlotValues() merges every device per slot with ezMath::Max, so this coexists with
///    the actual keyboard and mouse instead of fighting them. A human at the machine keeps control.
///  - Key down/up transitions and dead zones are computed by the manager from the merged value, so
///    ezKeyState::Pressed and Released come out correct without this class knowing anything about them.
///  - Holding a key for several frames is just leaving the value in m_InputSlotValues, and letting go is
///    erasing it. There is no per-slot bookkeeping to get wrong.
///
/// It registers no slots of its own. It writes the names that the real devices already registered -
/// 'keyboard_w', 'mouse_button_0' - which is what makes the Max merge meaningful in the first place.
/// A slot nobody registered can still be written, it just never reaches anything that reads input.
///
/// The header of ezInputDevice warns that a device may need integration into window message handling.
/// That applies to hardware; there is no hardware here.
class ezMcpInputDevice : public ezInputDevice
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpInputDevice, ezInputDevice);

public:
  ezMcpInputDevice();
  ~ezMcpInputDevice();

  /// \brief The one instance, or nullptr while the plugin's input tool does not exist.
  static ezMcpInputDevice* GetInstance() { return s_pInstance; }

  /// \brief Sets a slot's value, held until it is changed again or cleared.
  ///
  /// \param uiFrames How many frames to hold it for, counted down once per input update. 0 means
  ///        indefinitely - which is what a caller who wants to press a key now and release it later
  ///        wants, and also the way to leave a movement axis held down.
  void SetSlotValue(ezStringView sSlot, float fValue, ezUInt32 uiFrames);

  /// \brief Stops writing a slot entirely. Anything else writing it is then the only source again.
  void ClearSlot(ezStringView sSlot);

  /// \brief Stops writing every slot.
  void ClearAllSlots();

  /// \brief Injects typed text in one call, as if the OS had delivered every character within the same frame.
  ///
  /// Unlike SetSlotValue(), this is not held: it is consumed the next time something reads
  /// ezInputManager::RetrieveLastCharacters() - same as a real keystroke - and gone afterwards. Queuing
  /// again before that happens appends, it does not replace.
  void QueueText(ezStringView sText);

  /// \brief What this device is currently writing, and for how much longer.
  struct HeldSlot
  {
    ezString m_sSlot;
    float m_fValue = 0.0f;

    /// 0 means 'until changed'.
    ezUInt32 m_uiFramesRemaining = 0;
  };

  void GetHeldSlots(ezDynamicArray<HeldSlot>& out_slots) const;

private:
  virtual void InitializeDevice() override {}
  virtual void RegisterInputSlots() override {}
  virtual void UpdateInputSlotValues() override;

  /// Slots the caller asked to be held for a limited number of frames, and the count that is left.
  /// Slots held indefinitely are not in here at all - they just sit in m_InputSlotValues.
  ezMap<ezString, ezUInt32> m_RemainingFrames;

  static ezMcpInputDevice* s_pInstance;
};
