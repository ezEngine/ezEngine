#pragma once

#include <Core/System/Window.h>

/// Window implementation that provides sizing and message-loop behavior without creating a native window.
class EZ_CORE_DLL ezHeadlessWindow final : public ezWindowPlatformShared
{
public:
  virtual ~ezHeadlessWindow();

  virtual ezSizeU32 GetClientAreaSize() const override { return m_WindowSize; }
  virtual ezResult InitializeWindow() override;
  virtual void DestroyWindow() override;
  virtual ezResult Resize(const ezSizeU32& newWindowSize) override;
  virtual void ProcessWindowMessages() override;
  virtual ezWindowHandle GetNativeWindowHandle() const override;

private:
  ezSizeU32 m_WindowSize;
};
