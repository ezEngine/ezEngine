#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <RendererCore/RendererCoreDLL.h>

class ezWorld;
class ezViewHandle;

/// \brief Used in ezDebugRenderer to determine where debug geometry should be rendered
class EZ_RENDERERCORE_DLL ezDebugRendererContext
{
public:
  EZ_ALWAYS_INLINE ezDebugRendererContext()
    : m_Id(-1)
  {
  }

  /// \brief If this constructor is used, the geometry is rendered in all views for that scene.
  ezDebugRendererContext(const ezWorld* pWorld);

  /// \brief If this constructor is used, the geometry is only rendered in this view.
  ezDebugRendererContext(const ezViewHandle& hView);

  EZ_ALWAYS_INLINE bool operator==(const ezDebugRendererContext& other) const { return m_Id == other.m_Id; }

private:
  friend struct ezHashHelper<ezDebugRendererContext>;

  ezUInt32 m_Id;
};


template <>
struct ezHashHelper<ezDebugRendererContext>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezDebugRendererContext value) { return ezHashHelper<ezUInt32>::Hash(value.m_Id); }

  EZ_ALWAYS_INLINE static bool Equal(ezDebugRendererContext a, ezDebugRendererContext b) { return a == b; }
};
