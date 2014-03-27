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

void RenderCircleXZ(const ezVec3& v, float fRadius, const ezMat4& mRot)
{
  int div = ezMath::Clamp<int>((int) ezMath::Trunc(fRadius * 20.0f), 8, 180);

  float step = 360.0f / div;

  float f1 = 0.0f;
  float f2 = step;

  ezVec3 v1;
  ezVec3 v2;

  glBegin(GL_LINES);

  for (int i=0; i<div; i++)
  {
    v1.Set(ezMath::Cos(ezAngle::Degree(f1)) * fRadius, 0.0f, ezMath::Sin(ezAngle::Degree(f1)) * fRadius);
    v2.Set(ezMath::Cos(ezAngle::Degree(f2)) * fRadius, 0.0f, ezMath::Sin(ezAngle::Degree(f2)) * fRadius);

    v1 = mRot * (v1 + v);
    v2 = mRot * (v2 + v);

    glVertex3f(v1.x, v1.y, v1.z); 
    glVertex3f(v2.x, v2.y, v2.z); 

    f1 = f2;
    f2 += step;
  }		

  glEnd();
}