#include <PCH.h>
#include <RTS/General/Application.h>
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

    glTexCoord2f(0, 0);
    glVertex3f(v.x      , v.y      , v.z + s.z);
    glTexCoord2f(1, 0);
    glVertex3f(v.x + s.x, v.y      , v.z + s.z);
    glTexCoord2f(1, 1);
    glVertex3f(v.x + s.x, v.y + s.y, v.z + s.z);
    glTexCoord2f(0, 1);
    glVertex3f(v.x      , v.y + s.y, v.z + s.z);

    if (bColor)
      SetColor(180, fColorScale);

    glTexCoord2f(1, 0);
    glVertex3f(v.x + s.x, v.y      , v.z      );
    glTexCoord2f(0, 0);
    glVertex3f(v.x      , v.y      , v.z      );
    glTexCoord2f(0, 1);
    glVertex3f(v.x      , v.y + s.y, v.z      );
    glTexCoord2f(1, 1);
    glVertex3f(v.x + s.x, v.y + s.y, v.z      );

    if (bColor)
      SetColor(200, fColorScale);

    glTexCoord2f(0, 0);
    glVertex3f(v.x      , v.y      , v.z      );
    glTexCoord2f(0, 1);
    glVertex3f(v.x      , v.y      , v.z + s.z);
    glTexCoord2f(1, 1);
    glVertex3f(v.x      , v.y + s.y, v.z + s.z);
    glTexCoord2f(1, 0);
    glVertex3f(v.x      , v.y + s.y, v.z      );

    if (bColor)
      SetColor(180, fColorScale);

    glTexCoord2f(0, 1);
    glVertex3f(v.x + s.x, v.y      , v.z + s.z);
    glTexCoord2f(0, 0);
    glVertex3f(v.x + s.x, v.y      , v.z      );
    glTexCoord2f(1, 0);
    glVertex3f(v.x + s.x, v.y + s.y, v.z      );
    glTexCoord2f(1, 1);
    glVertex3f(v.x + s.x, v.y + s.y, v.z + s.z);

    if (bColor)
      SetColor(220, fColorScale);

    glTexCoord2f(0, 1);
    glVertex3f(v.x      , v.y + s.y, v.z + s.z);
    glTexCoord2f(1, 1);
    glVertex3f(v.x + s.x, v.y + s.y, v.z + s.z);
    glTexCoord2f(1, 0);
    glVertex3f(v.x + s.x, v.y + s.y, v.z      );
    glTexCoord2f(0, 0);
    glVertex3f(v.x      , v.y + s.y, v.z      );

    if (bColor)
      SetColor(220, fColorScale);

    glTexCoord2f(0, 0);
    glVertex3f(v.x      , v.y      , v.z      );
    glTexCoord2f(1, 0);
    glVertex3f(v.x + s.x, v.y      , v.z      );
    glTexCoord2f(1, 1);
    glVertex3f(v.x + s.x, v.y      , v.z + s.z);
    glTexCoord2f(0, 1);
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

void GameRenderer::RenderFormattedText(float fTextSize, TextAlignment Align, ezColor Color, ezInt32 x, ezInt32 y, const char* szText, ...)
{
  va_list args;
  va_start(args, szText);
  ezStringBuilder sText;
  sText.Format(szText, args);
  va_end(args);

  RenderText(fTextSize, Align, Color, x, y, sText.GetData());
}

void GameRenderer::RenderText(float fTextSize, TextAlignment Align, ezColor Color, ezInt32 x, ezInt32 y, const char* szText)
{
  if (m_uiFontTextureID == 0)
    return;

  ezStringBuilder sText = szText;

  glDepthMask(false);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

  const ezInt32 iCharsPerRow = 16;
  const ezInt32 iCharsPerColumn = 8;

  const float fCharWidth = fTextSize / 2.0f;
  float fMoveX = sText.GetCharacterCount() * fCharWidth;

  switch (Align)
  {
  case ALIGN_CENTER:
    fMoveX = -fMoveX / 2;
    break;
  case ALIGN_RIGHT:
    fMoveX = -fMoveX;
    break;
  case ALIGN_LEFT:
  default:
    fMoveX = 0;
    break;
  }

  ezVec3 vTranslation(0.0f);
  vTranslation += ezVec3((float) x + fMoveX, (float) y, 0);

  ezStringView it = sText.GetIteratorFront();

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_uiFontTextureID);

  glColor4f(Color.r, Color.g, Color.b, Color.a);
  glBegin(GL_QUADS);

  while (it.IsValid())
  {
    ezUInt32 uiChar = it.GetCharacter();

    if (!ezUnicodeUtils::IsASCII(uiChar))
      uiChar = 0;

    {
      const int px = uiChar % iCharsPerRow;
      const int py = uiChar / iCharsPerRow;

      const float f8r = (1.0f / iCharsPerRow);
      const float f8c = (1.0f / iCharsPerColumn);
      const float cx = f8r * px;
      const float cy = f8c * py;
      const float cx2 = cx + f8r;
      const float cy2 = cy + f8c;

      const ezVec3 v = vTranslation;

      glTexCoord2f(cx, cy2);
      glVertex2f(v.x, v.y + fTextSize);

      glTexCoord2f(cx2, cy2);
      glVertex2f(v.x + fTextSize, v.y + fTextSize);

      glTexCoord2f(cx2, cy);
      glVertex2f(v.x + fTextSize, v.y);

      glTexCoord2f(cx, cy);
      glVertex2f(v.x, v.y);
    }

    vTranslation += ezVec3(fCharWidth, 0, 0);
    ++it;
  }

  glEnd();

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glDepthMask(true);
}