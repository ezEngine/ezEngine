#include <PCH.h>
#include <WindowsMixedReality/Input/SpatialInputDevice.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>
#include <wrl/event.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputDeviceSpatialInteraction, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE

ezInputDeviceSpatialInteraction g_SpatialInputDevice;

ezInputDeviceSpatialInteraction::ezInputDeviceSpatialInteraction()
{
}

ezInputDeviceSpatialInteraction::~ezInputDeviceSpatialInteraction()
{
}

void ezInputDeviceSpatialInteraction::InitializeDevice()
{
  ezUwpUtils::RetrieveStatics(RuntimeClass_Windows_UI_Input_Spatial_SpatialInteractionManager, m_pSpatialInteractionManagerStatics);

  if (SUCCEEDED(m_pSpatialInteractionManagerStatics->GetForCurrentView(&m_pSpatialInteractionManager)))
  {
    using DefOnSourcePressed = __FITypedEventHandler_2_Windows__CUI__CInput__CSpatial__CSpatialInteractionManager_Windows__CUI__CInput__CSpatial__CSpatialInteractionSourceEventArgs;

    m_pSpatialInteractionManager->add_SourcePressed(Microsoft::WRL::Callback<DefOnSourcePressed>(this, &ezInputDeviceSpatialInteraction::OnSourcePressed).Get(), &m_OnSourcePressedToken);
  }
}

void ezInputDeviceSpatialInteraction::RegisterInputSlots()
{
  RegisterInputSlot(ezInputSlot_Spatial_Hand0_Tap, "Air Tap", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_Spatial_Hand0_PositionPosX, "Hand Position Pos X", ezInputSlotFlags::IsTrackedValue);
  RegisterInputSlot(ezInputSlot_Spatial_Hand0_PositionPosY, "Hand Position Pos Y", ezInputSlotFlags::IsTrackedValue);
  RegisterInputSlot(ezInputSlot_Spatial_Hand0_PositionPosZ, "Hand Position Pos Z", ezInputSlotFlags::IsTrackedValue);
  RegisterInputSlot(ezInputSlot_Spatial_Hand0_PositionNegX, "Hand Position Neg X", ezInputSlotFlags::IsTrackedValue);
  RegisterInputSlot(ezInputSlot_Spatial_Hand0_PositionNegY, "Hand Position Neg Y", ezInputSlotFlags::IsTrackedValue);
  RegisterInputSlot(ezInputSlot_Spatial_Hand0_PositionNegZ, "Hand Position Neg Z", ezInputSlotFlags::IsTrackedValue);
}

void ezInputDeviceSpatialInteraction::ResetInputSlotValues()
{
  m_InputSlotValues[ezInputSlot_Spatial_Hand0_Tap] = 0.0f;
}

void ezInputDeviceSpatialInteraction::UpdateInputSlotValues()
{

}

HRESULT ezInputDeviceSpatialInteraction::OnSourcePressed(ABI::Windows::UI::Input::Spatial::ISpatialInteractionManager* manager, ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceEventArgs* args)
{
  ezLog::Debug("Source Pressed");

  ComPtr<ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceState> state;
  args->get_State(&state);

  ComPtr<ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceProperties> props;
  state->get_Properties(&props);

  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();

  ComPtr<ABI::Windows::Perception::Spatial::ISpatialCoordinateSystem> coordinateSystem;
  pHoloSpace->GetDefaultReferenceFrame()->GetInternalCoordinateSystem(coordinateSystem);

  ComPtr<ABI::Windows::UI::Input::Spatial::ISpatialInteractionSourceLocation> location;
  if (FAILED(props->TryGetLocation(coordinateSystem.Get(), &location)))
    return S_OK;

  if (location == nullptr)
    return S_OK;

  ABI::Windows::Foundation::__FIReference_1_Windows__CFoundation__CNumerics__CVector3_t* pos;
  ABI::Windows::Foundation::__FIReference_1_Windows__CFoundation__CNumerics__CVector3_t* vel;
  location->get_Position(&pos);
  location->get_Velocity(&vel);

  const ezVec3 vPosition = ezUwpUtils::ConvertVec3(pos);
  const ezVec3 vVelocity = ezUwpUtils::ConvertVec3(vel);

  ezLog::Info("Hand Position: {0} | {1} | {2}", vPosition.x, vPosition.y, vPosition.z);
  ezLog::Info("Hand Velocity: {0} | {1} | {2}", vVelocity.x, vVelocity.y, vVelocity.z);

  /// \todo Coordinate system swaps Y and Z, not sure whether to do a conversion here
  m_InputSlotValues[ezInputSlot_Spatial_Hand0_PositionPosX] = ezMath::Max(0.0f, vPosition.x);
  m_InputSlotValues[ezInputSlot_Spatial_Hand0_PositionPosY] = ezMath::Max(0.0f, vPosition.y);
  m_InputSlotValues[ezInputSlot_Spatial_Hand0_PositionPosZ] = ezMath::Max(0.0f, vPosition.z);
  m_InputSlotValues[ezInputSlot_Spatial_Hand0_PositionNegX] = ezMath::Max(0.0f, -vPosition.x);
  m_InputSlotValues[ezInputSlot_Spatial_Hand0_PositionNegY] = ezMath::Max(0.0f, -vPosition.y);
  m_InputSlotValues[ezInputSlot_Spatial_Hand0_PositionNegZ] = ezMath::Max(0.0f, -vPosition.z);
  m_InputSlotValues[ezInputSlot_Spatial_Hand0_Tap] = 1.0f;

  return S_OK;
}


