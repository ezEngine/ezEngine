#include <System/Window/Window.h>
#include <Foundation/Math/Vec2.h>

class GameWindow : public ezWindow
{
public:
  GameWindow();
  ~GameWindow();

  ezSizeU32 GetResolution() const;
  void SwapBuffers();

private:
  void OnResizeMessage(const ezSizeU32& newWindowSize) EZ_OVERRIDE;

  void CreateContextOGL();
  void DestroyContextOGL();

  HDC m_hDC;
  HGLRC m_hRC;
};