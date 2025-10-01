#include <Core/Platform/NoImpl/InputDevice_NoImpl.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputDeviceMouseKeyboard_NoImpl, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezInputDeviceMouseKeyboard_NoImpl::ezInputDeviceMouseKeyboard_NoImpl(ezUInt32 uiWindowNumber) {}
ezInputDeviceMouseKeyboard_NoImpl::~ezInputDeviceMouseKeyboard_NoImpl() = default;

void ezInputDeviceMouseKeyboard_NoImpl::SetShowMouseCursor(bool bShow) {}

bool ezInputDeviceMouseKeyboard_NoImpl::GetShowMouseCursor() const
{
  return false;
}

void ezInputDeviceMouseKeyboard_NoImpl::SetClipMouseCursor(ezMouseCursorClipMode::Enum mode) {}

ezMouseCursorClipMode::Enum ezInputDeviceMouseKeyboard_NoImpl::GetClipMouseCursor() const
{
  return ezMouseCursorClipMode::Default;
}

void ezInputDeviceMouseKeyboard_NoImpl::InitializeDevice() {}

void ezInputDeviceMouseKeyboard_NoImpl::RegisterInputSlots() {}
