#pragma once

#include <Core/Input/InputDevice.h>

struct ezAndroidInputEvent;
struct AInputEvent;

/// \brief Android standard input device.
class EZ_CORE_DLL ezInputDevice_Android : public ezInputDevice
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDevice_Android, ezInputDevice);

public:
  ezInputDevice_Android();
  ~ezInputDevice_Android();

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;
  virtual void UpdateInputSlotValues() override;

private:
  void AndroidInputEventHandler(ezAndroidInputEvent& event);
  void AndroidAppCommandEventHandler(ezInt32 iCmd);
  bool AndroidHandleInput(AInputEvent* pEvent);

private:
  ezInt32 m_iResolutionX = 0;
  ezInt32 m_iResolutionY = 0;
};
