#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

#if EZ_ENABLED(EZ_SUPPORTS_GLFW)

extern "C"
{
  typedef struct GLFWwindow GLFWwindow;
}

class EZ_CORE_DLL ezStandardInputDevice : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStandardInputDevice, ezInputDeviceMouseKeyboard);

public:
  ezStandardInputDevice(ezUInt32 uiWindowNumber, GLFWwindow* windowHandle);
  ~ezStandardInputDevice();

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;
  virtual void SetClipMouseCursor(ezMouseCursorClipMode::Enum mode) override;
  virtual ezMouseCursorClipMode::Enum GetClipMouseCursor() const override;

  // GLFW callback for key pressed, released, repeated events
  void OnKey(int key, int scancode, int action, int mods);

  // GLFW callback for text input (each UTF32 code point individually)
  void OnCharacter(unsigned int codepoint);

  // GLFW callback on mouse move
  void OnCursorPosition(double xpos, double ypos);

  // GLFW callback on mouse button actions
  void OnMouseButton(int button, int action, int mods);

  // GLFW callback for mouse scroll
  void OnScroll(double xoffset, double yoffset);

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;

private:
  ezUInt32 m_uiWindowNumber = 0;
  GLFWwindow* m_pWindow = nullptr;
  ezVec2d m_LastPos = ezVec2d(ezMath::MaxValue<double>());
};

#endif
