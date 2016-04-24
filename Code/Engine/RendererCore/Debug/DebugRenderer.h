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
    EZ_DECLARE_POD_TYPE();

    EZ_FORCE_INLINE Line() { }

    EZ_FORCE_INLINE Line(const ezVec3& start, const ezVec3& end)
      : m_start(start), m_end(end)
    {
    }

    ezVec3 m_start;
    ezVec3 m_end;    
  };

  struct Triangle
  {
    EZ_DECLARE_POD_TYPE();

    EZ_FORCE_INLINE Triangle() { }

    EZ_FORCE_INLINE Triangle(const ezVec3& p0, const ezVec3& p1, const ezVec3& p2)
      : m_p0(p0), m_p1(p1), m_p2(p2)
    {
    }

    ezVec3 m_p0;
    ezVec3 m_p1;
    ezVec3 m_p2;    
  };


  static void DrawLines(const ezWorld* pWorld, ezArrayPtr<Line> lines, const ezColor& color);
  static void DrawLines(ezUInt32 uiWorldIndex, ezArrayPtr<Line> lines, const ezColor& color);
  
  static void DrawLineBox(const ezWorld* pWorld, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform = ezTransform::Identity());
  static void DrawLineBox(ezUInt32 uiWorldIndex, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform = ezTransform::Identity());

  static void DrawLineBoxCorners(const ezWorld* pWorld, const ezBoundingBox& box, float fCornerFraction, const ezColor& color, const ezTransform& transform = ezTransform::Identity());
  static void DrawLineBoxCorners(ezUInt32 uiWorldIndex, const ezBoundingBox& box, float fCornerFraction, const ezColor& color, const ezTransform& transform = ezTransform::Identity());

  static void DrawSolidBox(const ezWorld* pWorld, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform = ezTransform::Identity());
  static void DrawSolidBox(ezUInt32 uiWorldIndex, const ezBoundingBox& box, const ezColor& color, const ezTransform& transform = ezTransform::Identity());

  static void DrawSolidTriangles(const ezWorld* pWorld, ezArrayPtr<Triangle> triangles, const ezColor& color);
  static void DrawSolidTriangles(ezUInt32 uiWorldIndex, ezArrayPtr<Triangle> triangles, const ezColor& color);

private:
  friend class ezSimpleRenderPass;

  static void Render(const ezRenderViewContext& renderViewContext);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Graphics, DebugRenderer);
};

