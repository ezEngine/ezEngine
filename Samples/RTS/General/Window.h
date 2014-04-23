#pragma once

#include <System/Window/Window.h>
#include <Foundation/Math/Vec2.h>

class GameWindow : public ezWindow
{
public:
  GameWindow();
  ~GameWindow();

  ezSizeU32 GetResolution() const;
  bool IsMinimized() const { return m_bMinimized; }

private:
  void OnResizeMessage(const ezSizeU32& newWindowSize) override;

  bool m_bMinimized;
};