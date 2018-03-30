#pragma once

#include <RendererCore/Basics.h>
#include <Foundation/Algorithm/Hashing.h>

class ezWorld;
class ezView;
class ezViewHandle;

class EZ_RENDERERCORE_DLL ezDebugRendererContext
{
public:
  EZ_ALWAYS_INLINE ezDebugRendererContext() : m_Id(-1) { }

  ezDebugRendererContext(const ezWorld* pWorld);
  ezDebugRendererContext(const ezViewHandle& hView);

  EZ_ALWAYS_INLINE bool operator==(const ezDebugRendererContext& other) const
  {
    return m_Id == other.m_Id;
  }

private:
  friend struct ezHashHelper<ezDebugRendererContext>;

  ezUInt32 m_Id;
};


template <>
struct ezHashHelper<ezDebugRendererContext>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezDebugRendererContext value)
  {
    return ezHashHelper<ezUInt32>::Hash(value.m_Id);
  }

  EZ_ALWAYS_INLINE static bool Equal(ezDebugRendererContext a, ezDebugRendererContext b)
  {
    return a == b;
  }
};

