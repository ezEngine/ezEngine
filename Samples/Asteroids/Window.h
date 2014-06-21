#include <System/Window/Window.h>
#include <Foundation/Math/Vec2.h>

class GameWindow : public ezWindow
{
public:
  GameWindow();
  ~GameWindow();

  ezSizeU32 GetResolution() const;

  void PresentFrame();

private:
  void OnResizeMessage(const ezSizeU32& newWindowSize) override;

  ezResult CreateContextOpenGL();
  ezResult DestroyGraphicsContext();
  ezResult CreateGraphicsContext();
  ezResult DestroyContextOpenGL();
  

  HDC m_hDC;
  HGLRC m_hRC;

};