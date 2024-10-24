
#if EZ_ENABLED(EZ_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/Window_GLFW.h>

#else

class EZ_CORE_DLL ezWindowWin : public ezWindowPlatformShared
{
public:
  ~ezWindowWin();

  virtual ezResult InitializeWindow() override;
  virtual void DestroyWindow() override;
  virtual ezResult Resize(const ezSizeU32& newWindowSize) override;
  virtual void ProcessWindowMessages() override;
  virtual void OnResize(const ezSizeU32& newWindowSize) override;
  virtual ezWindowHandle GetNativeWindowHandle() const override;

  /// \brief Called on any window message.
  ///
  /// You can use this function for example to dispatch the message to another system.
  ///
  /// \remarks
  ///   Will be called <i>after</i> the On[...] callbacks!
  ///
  /// \see OnResizeMessage
  virtual void OnWindowMessage(ezMinWindows::HWND hWnd, ezMinWindows::UINT msg, ezMinWindows::WPARAM wparam, ezMinWindows::LPARAM lparam)
  {
    EZ_IGNORE_UNUSED(hWnd);
    EZ_IGNORE_UNUSED(msg);
    EZ_IGNORE_UNUSED(wparam);
    EZ_IGNORE_UNUSED(lparam);
  }
};

// can't use a 'using' here, because that can't be forward declared
class EZ_CORE_DLL ezWindow : public ezWindowWin
{
};

#endif
