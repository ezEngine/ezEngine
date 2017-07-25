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
  struct SourceInfo
  {
    /// \brief Describes as which InputSlot this source is going to be exposed
    enum class SlotMapping
    {
      Unknown,
      Ignored,
      Hand0,
      Hand1,
      //Controller0,
      //Controller1,
      // Voice
    };

    SlotMapping m_SlotMapping = SlotMapping::Unknown;
    bool m_bCurrentlySeen = false;
  };

  struct SourceDetails
  {
    ezUInt32 m_uiSourceID = 0xFFFFFFFF;
    ABI::Windows::UI::Input::Spatial::SpatialInteractionSourceKind m_Kind;
    bool m_bIsPressed = false;
    ezVec3 m_vPosition;
  };

  void GetSourceDetails(ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceEventArgs* args, SourceDetails& out_Details);
  void UpdateSourceInfo(const SourceDetails& details, bool isTracked, SourceInfo*& out_pInfo);
  void SetTrackingStatus(ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceEventArgs* args, bool bTracked);

  HRESULT OnSourceDetected(ABI::Windows::UI::Input::Spatial::ISpatialInteractionManager* pManager, ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceEventArgs* args);

  HRESULT OnSourceLost(ABI::Windows::UI::Input::Spatial::ISpatialInteractionManager* pManager, ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceEventArgs* args);

  HRESULT OnSourcePressed(ABI::Windows::UI::Input::Spatial::ISpatialInteractionManager* pManager, ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceEventArgs* args);

  HRESULT OnSourceReleased(ABI::Windows::UI::Input::Spatial::ISpatialInteractionManager* pManager, ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceEventArgs* args);

  EventRegistrationToken m_OnSourceDetectedToken;
  EventRegistrationToken m_OnSourceLostToken;
  EventRegistrationToken m_OnSourcePressedToken;
  EventRegistrationToken m_OnSourceReleasedToken;
  ComPtr<ABI::Windows::UI::Input::Spatial::ISpatialInteractionManagerStatics> m_pSpatialInteractionManagerStatics;
  ComPtr<ABI::Windows::UI::Input::Spatial::ISpatialInteractionManager> m_pSpatialInteractionManager;

  ezMap<ezUInt32, SourceInfo> m_InputSources;
};

