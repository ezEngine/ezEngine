#include <System/Window/Window.h>
#include <Foundation/Math/Vec2.h>

class GameWindow : public ezWindow
{
public:
  GameWindow();
  ~GameWindow();

private:
  void OnResizeMessage(const ezSizeU32& newWindowSize) override;

};