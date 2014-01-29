#include <PCH.h>
#include <SampleApp_RTS/General/Application.h>
#include <gl/GL.h>
#include <gl/glu.h>
#include <Foundation/Math/Color.h>

void SetColor(ezUInt8 c, float fColorScale)
{
  glColor3f((c * fColorScale) / 255.0f, (c * fColorScale) / 255.0f, (c * fColorScale) / 255.0f);
}

void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true, float fColorScale = 1.0f)
{
  glBegin(GL_QUADS);
    if (bColor)
      SetColor(200, fColorScale);

    glVertex3f(v.x      , v.y      , v.z + s.z);
    glVertex3f(v.x + s.x, v.y      , v.z + s.z);
    glVertex3f(v.x + s.x, v.y + s.y, v.z + s.z);
    glVertex3f(v.x      , v.y + s.y, v.z + s.z);

    if (bColor)
      SetColor(180, fColorScale);

    glVertex3f(v.x + s.x, v.y      , v.z      );
    glVertex3f(v.x      , v.y      , v.z      );
    glVertex3f(v.x      , v.y + s.y, v.z      );
    glVertex3f(v.x + s.x, v.y + s.y, v.z      );

    if (bColor)
      SetColor(200, fColorScale);

    glVertex3f(v.x      , v.y      , v.z      );
    glVertex3f(v.x      , v.y      , v.z + s.z);
    glVertex3f(v.x      , v.y + s.y, v.z + s.z);
    glVertex3f(v.x      , v.y + s.y, v.z      );

    if (bColor)
      SetColor(180, fColorScale);

    glVertex3f(v.x + s.x, v.y      , v.z + s.z);
    glVertex3f(v.x + s.x, v.y      , v.z      );
    glVertex3f(v.x + s.x, v.y + s.y, v.z      );
    glVertex3f(v.x + s.x, v.y + s.y, v.z + s.z);

    if (bColor)
      SetColor(220, fColorScale);

    glVertex3f(v.x      , v.y + s.y, v.z + s.z);
    glVertex3f(v.x + s.x, v.y + s.y, v.z + s.z);
    glVertex3f(v.x + s.x, v.y + s.y, v.z      );
    glVertex3f(v.x      , v.y + s.y, v.z      );

    if (bColor)
      SetColor(220, fColorScale);

    glVertex3f(v.x      , v.y      , v.z      );
    glVertex3f(v.x + s.x, v.y      , v.z      );
    glVertex3f(v.x + s.x, v.y      , v.z + s.z);
    glVertex3f(v.x      , v.y      , v.z + s.z);

  glEnd();
}