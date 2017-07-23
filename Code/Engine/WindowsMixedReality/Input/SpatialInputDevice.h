#pragma once

#include <WindowsMixedReality/Basics.h>
#include <Core/Input/InputDevice.h>
#include <windows.ui.input.spatial.h>
#include <wrl/client.h>

class EZ_WINDOWSMIXEDREALITY_DLL ezInputDeviceSpatialInteraction : public ezInputDevice
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDeviceSpatialInteraction, ezInputDevice);

public:
  ezInputDeviceSpatialInteraction();
  ~ezInputDeviceSpatialInteraction();

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;
  virtual void UpdateInputSlotValues() override;

private:
  HRESULT OnSourcePressed(ABI::Windows::UI::Input::Spatial::ISpatialInteractionManager* pManager, ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceEventArgs* args);

  ComPtr<ABI::Windows::UI::Input::Spatial::ISpatialInteractionManagerStatics> m_pSpatialInteractionManagerStatics;
  ComPtr<ABI::Windows::UI::Input::Spatial::ISpatialInteractionManager> m_pSpatialInteractionManager;
  EventRegistrationToken m_OnSourcePressedToken;

};

