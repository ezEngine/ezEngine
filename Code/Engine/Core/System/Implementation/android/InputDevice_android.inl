#include <Core/System/Implementation/android/InputDevice_android.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#include <android/log.h>
#include <android_native_app_glue.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStandardInputDevice, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// Comment in to get verbose output on android input
// #  define DEBUG_ANDROID_INPUT

#  ifdef DEBUG_ANDROID_INPUT
#    define DEBUG_LOG(...) ezLog::Debug(__VA_ARGS__)
#  else
#    define DEBUG_LOG(...)
#  endif

ezStandardInputDevice::ezStandardInputDevice(ezUInt32 uiWindowNumber)
{
  ezAndroidUtils::s_InputEvent.AddEventHandler(ezMakeDelegate(&ezStandardInputDevice::AndroidInputEventHandler, this));
}

ezStandardInputDevice::~ezStandardInputDevice()
{
  ezAndroidUtils::s_InputEvent.RemoveEventHandler(ezMakeDelegate(&ezStandardInputDevice::AndroidInputEventHandler, this));
}

void ezStandardInputDevice::SetShowMouseCursor(bool bShow) {}

bool ezStandardInputDevice::GetShowMouseCursor() const
{
  return false;
}

void ezStandardInputDevice::SetClipMouseCursor(ezMouseCursorClipMode::Enum mode) {}

ezMouseCursorClipMode::Enum ezStandardInputDevice::GetClipMouseCursor() const
{
  return ezMouseCursorClipMode::Default;
}

void ezStandardInputDevice::InitializeDevice()
{
}

void ezStandardInputDevice::RegisterInputSlots()
{
  RegisterInputSlot(ezInputSlot_TouchPoint0, "Touchpoint 0", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint0_PositionX, "Touchpoint 0 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint0_PositionY, "Touchpoint 0 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint1, "Touchpoint 1", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint1_PositionX, "Touchpoint 1 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint1_PositionY, "Touchpoint 1 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint2, "Touchpoint 2", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint2_PositionX, "Touchpoint 2 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint2_PositionY, "Touchpoint 2 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint3, "Touchpoint 3", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint3_PositionX, "Touchpoint 3 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint3_PositionY, "Touchpoint 3 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint4, "Touchpoint 4", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint4_PositionX, "Touchpoint 4 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint4_PositionY, "Touchpoint 4 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint5, "Touchpoint 5", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint5_PositionX, "Touchpoint 5 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint5_PositionY, "Touchpoint 5 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint6, "Touchpoint 6", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint6_PositionX, "Touchpoint 6 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint6_PositionY, "Touchpoint 6 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint7, "Touchpoint 7", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint7_PositionX, "Touchpoint 7 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint7_PositionY, "Touchpoint 7 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint8, "Touchpoint 8", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint8_PositionX, "Touchpoint 8 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint8_PositionY, "Touchpoint 8 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint9, "Touchpoint 9", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint9_PositionX, "Touchpoint 9 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint9_PositionY, "Touchpoint 9 Position Y", ezInputSlotFlags::IsTouchPosition);
}

void ezStandardInputDevice::ResetInputSlotValues()
{
  for (int id = 0; id < 10; ++id)
  {
    // We can't reset the position inside AndroidHandleInput as we want the position to be valid when lifting a finger. Thus, we clear the position here after the update has been performed.
    if (m_InputSlotValues[ezInputManager::GetInputSlotTouchPoint(id)] == 0)
    {
      m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionX(id)] = 0;
      m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionY(id)] = 0;
    }
  }
}

void ezStandardInputDevice::AndroidInputEventHandler(ezAndroidInputEvent& event)
{
  event.m_iReturn = AndroidHandleInput(event.m_pEvent);
  SUPER::UpdateInputSlotValues();
}

ezInt32 ezStandardInputDevice::AndroidHandleInput(AInputEvent* pEvent)
{
  // #TODO_ANDROID Only touchscreen input is implemented right now.
  const ezInt32 iEventType = AInputEvent_getType(pEvent);
  const ezInt32 iEventSource = AInputEvent_getSource(pEvent);
  const ezUInt32 uiAction = (ezUInt32)AMotionEvent_getAction(pEvent);
  const ezInt32 iKeyCode = AKeyEvent_getKeyCode(pEvent);
  const ezInt32 iButtonState = AMotionEvent_getButtonState(pEvent);
  (void)iKeyCode;
  (void)iButtonState;
  DEBUG_LOG("Android INPUT: iEventType: {}, iEventSource: {}, uiAction: {}, iKeyCode: {}, iButtonState: {}", iEventType,
    iEventSource, uiAction, iKeyCode, iButtonState);

  ezHybridArray<ezScreenInfo, 2> screens;
  if (ezScreen::EnumerateScreens(screens).Failed() || screens.IsEmpty())
    return 0;

  // I.e. fingers have touched the touchscreen.
  if (iEventType == AINPUT_EVENT_TYPE_MOTION && (iEventSource & AINPUT_SOURCE_TOUCHSCREEN) != 0)
  {
    // Update pointer positions
    const ezUInt64 uiPointerCount = AMotionEvent_getPointerCount(pEvent);
    for (ezUInt32 uiPointerIndex = 0; uiPointerIndex < uiPointerCount; uiPointerIndex++)
    {
      const float fPixelX = AMotionEvent_getX(pEvent, uiPointerIndex);
      const float fPixelY = AMotionEvent_getY(pEvent, uiPointerIndex);
      const ezInt32 id = AMotionEvent_getPointerId(pEvent, uiPointerIndex);
      if (id < 10)
      {
        m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionX(id)] = static_cast<float>(fPixelX / static_cast<float>(screens[0].m_iResolutionX));
        m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionY(id)] = static_cast<float>(fPixelY / static_cast<float>(screens[0].m_iResolutionY));
        DEBUG_LOG("Finger MOVE: {} = {} x {}", id, m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionX(id)], m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionY(id)]);
      }
    }

    // Update pointer state
    const ezUInt32 uiActionEvent = uiAction & AMOTION_EVENT_ACTION_MASK;
    const ezUInt32 uiActionPointerIndex = (uiAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    const ezInt32 id = AMotionEvent_getPointerId(pEvent, uiActionPointerIndex);
    // We only support up to 10 touch points at the same time.
    if (id >= 10)
      return 0;

    {
      // Not sure if the action finger is always present in the upper loop of uiPointerCount, so we update it here for good measure.
      const float fPixelX = AMotionEvent_getX(pEvent, uiActionPointerIndex);
      const float fPixelY = AMotionEvent_getY(pEvent, uiActionPointerIndex);
      m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionX(id)] = static_cast<float>(fPixelX / static_cast<float>(screens[0].m_iResolutionX));
      m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionY(id)] = static_cast<float>(fPixelY / static_cast<float>(screens[0].m_iResolutionY));
      DEBUG_LOG("Finger MOVE: {} = {} x {}", id, m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionX(id)], m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionY(id)]);
    }

    switch (uiActionEvent)
    {
      case AMOTION_EVENT_ACTION_DOWN:
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
        m_InputSlotValues[ezInputManager::GetInputSlotTouchPoint(id)] = 1;
        DEBUG_LOG("Finger DOWN: {}", id);
        return 1;
      case AMOTION_EVENT_ACTION_MOVE:
        // Finger moved (we always update that at the top).
        return 1;
      case AMOTION_EVENT_ACTION_UP:
      case AMOTION_EVENT_ACTION_POINTER_UP:
      case AMOTION_EVENT_ACTION_CANCEL:
      case AMOTION_EVENT_ACTION_OUTSIDE:
        m_InputSlotValues[ezInputManager::GetInputSlotTouchPoint(id)] = 0;
        DEBUG_LOG("Finger UP: {}", id);
        return 1;
      case AMOTION_EVENT_ACTION_SCROLL:
      case AMOTION_EVENT_ACTION_HOVER_ENTER:
      case AMOTION_EVENT_ACTION_HOVER_MOVE:
      case AMOTION_EVENT_ACTION_HOVER_EXIT:
        return 0;
      default:
        DEBUG_LOG("Unknown AMOTION_EVENT_ACTION: {}", uiActionEvent);
        return 0;
    }
  }
  return 0;
}
