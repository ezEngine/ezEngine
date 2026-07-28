#include <Core/Platform/NoImpl/InputDevice_NoImpl.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputDeviceMouseKeyboard_NoImpl, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezInputDeviceMouseKeyboard_NoImpl::ezInputDeviceMouseKeyboard_NoImpl(ezUInt32 uiWindowNumber) {}
ezInputDeviceMouseKeyboard_NoImpl::~ezInputDeviceMouseKeyboard_NoImpl() = default;

void ezInputDeviceMouseKeyboard_NoImpl::ApplyShowMouseCursor(bool bShow, bool bCustomCursorActive)
{
  EZ_IGNORE_UNUSED(bShow);
  EZ_IGNORE_UNUSED(bCustomCursorActive);
}

void ezInputDeviceMouseKeyboard_NoImpl::ApplyClipMouseCursor(ezMouseCursorClipMode::Enum mode)
{
  EZ_IGNORE_UNUSED(mode);
}

void ezInputDeviceMouseKeyboard_NoImpl::InitializeDevice() {}

void ezInputDeviceMouseKeyboard_NoImpl::RegisterInputSlots() {}
