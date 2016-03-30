#pragma once

#include <RendererCore/Basics.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/Color.h>

class ezWorld;
struct ezRenderViewContext;

class EZ_RENDERERCORE_DLL ezDebugRenderer
{
public:
  struct Line
  {
    EZ_FORCE_INLINE Line() { }

    EZ_FORCE_INLINE Line(const ezVec3& start, const ezVec3& end)
      : m_start(start), m_end(end)
    {
    }

    union
    {
      struct  
      {
        ezVec3 m_start;
        ezVec3 m_end;
      };

      ezVec3 m_positions[2];
    };    
  };

  struct Triangle
  {
    EZ_FORCE_INLINE Triangle() { }

    EZ_FORCE_INLINE Triangle(const ezVec3& p0, const ezVec3& p1, const ezVec3& p2)
      : m_p0(p0), m_p1(p1), m_p2(p2)
    {
    }

    union
    {
      struct
      {
        ezVec3 m_p0;
        ezVec3 m_p1;
        ezVec3 m_p2;
      };

      ezVec3 m_positions[3];
    };    
  };


  static void DrawLines(const ezWorld* pWorld, ezArrayPtr<Line> lines, const ezColor& color);

  static void DrawLineBox(const ezWorld* pWorld, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform = ezTransform::Identity());

  static void DrawSolidBox(const ezWorld* pWorld, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform = ezTransform::Identity());

  static void DrawSolidTriangles(const ezWorld* pWorld, ezArrayPtr<Triangle> triangles, const ezColor& color);

  /// \todo
  //static void DrawSprite(const ezWorld* pWorld);

  static void Render(const ezRenderViewContext& renderViewContext);

private:
  static void OnEngineStartup();
  static void OnEngineShutdown();

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Graphics, DebugRenderer);
};

