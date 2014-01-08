#include <SampleApp_RTS/General/Application.h>
#include <gl/GL.h>
#include <gl/glu.h>
#include <Foundation/Math/Color.h>

void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true)
{
  glBegin(GL_QUADS);
    if (bColor)
      glColor3ub(200, 200, 200);

    glVertex3f(v.x      , v.y      , v.z + s.z);
    glVertex3f(v.x + s.x, v.y      , v.z + s.z);
    glVertex3f(v.x + s.x, v.y + s.y, v.z + s.z);
    glVertex3f(v.x      , v.y + s.y, v.z + s.z);

    if (bColor)
      glColor3ub(180, 180, 180);

    glVertex3f(v.x + s.x, v.y      , v.z      );
    glVertex3f(v.x      , v.y      , v.z      );
    glVertex3f(v.x      , v.y + s.y, v.z      );
    glVertex3f(v.x + s.x, v.y + s.y, v.z      );

    if (bColor)
      glColor3ub(200, 200, 200);

    glVertex3f(v.x      , v.y      , v.z      );
    glVertex3f(v.x      , v.y      , v.z + s.z);
    glVertex3f(v.x      , v.y + s.y, v.z + s.z);
    glVertex3f(v.x      , v.y + s.y, v.z      );

    if (bColor)
      glColor3ub(180, 180, 180);

    glVertex3f(v.x + s.x, v.y      , v.z + s.z);
    glVertex3f(v.x + s.x, v.y      , v.z      );
    glVertex3f(v.x + s.x, v.y + s.y, v.z      );
    glVertex3f(v.x + s.x, v.y + s.y, v.z + s.z);

    if (bColor)
      glColor3ub(220, 220, 220);

    glVertex3f(v.x      , v.y + s.y, v.z + s.z);
    glVertex3f(v.x + s.x, v.y + s.y, v.z + s.z);
    glVertex3f(v.x + s.x, v.y + s.y, v.z      );
    glVertex3f(v.x      , v.y + s.y, v.z      );

    if (bColor)
      glColor3ub(220, 220, 220);

    glVertex3f(v.x      , v.y      , v.z      );
    glVertex3f(v.x + s.x, v.y      , v.z      );
    glVertex3f(v.x + s.x, v.y      , v.z + s.z);
    glVertex3f(v.x      , v.y      , v.z + s.z);

  glEnd();
}