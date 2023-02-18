
EZ_ALWAYS_INLINE ezDebugRenderer::Line::Line() = default;

EZ_ALWAYS_INLINE ezDebugRenderer::Line::Line(const ezVec3& vStart, const ezVec3& vEnd)
  : m_start(vStart)
  , m_end(vEnd)
{
}

EZ_ALWAYS_INLINE ezDebugRenderer::Line::Line(const ezVec3& vStart, const ezVec3& vEnd, const ezColor& color)
  : m_start(vStart)
  , m_end(vEnd)
  , m_startColor(color)
  , m_endColor(color)
{
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE ezDebugRenderer::Triangle::Triangle() = default;

EZ_ALWAYS_INLINE ezDebugRenderer::Triangle::Triangle(const ezVec3& v0, const ezVec3& v1, const ezVec3& v2)

{
  m_position[0] = v0;
  m_position[1] = v1;
  m_position[2] = v2;
}
