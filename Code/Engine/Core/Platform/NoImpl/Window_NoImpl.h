#pragma once

class EZ_CORE_DLL ezWindowNoImpl : public ezWindowPlatformShared
{
public:
  ~ezWindowNoImpl();

  virtual ezResult InitializeWindow() override;
  virtual void DestroyWindow() override;
  virtual ezResult Resize(const ezSizeU32& newWindowSize) override;
  virtual void ProcessWindowMessages() override;
  virtual void OnResize(const ezSizeU32& newWindowSize) override;
  virtual ezWindowHandle GetNativeWindowHandle() const override;
};


// can't use a 'using' here, because that can't be forward declared
class EZ_CORE_DLL ezWindow : public ezWindowNoImpl
{
};
