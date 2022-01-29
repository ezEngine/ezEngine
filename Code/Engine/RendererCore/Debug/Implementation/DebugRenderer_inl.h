
EZ_ALWAYS_INLINE ezDebugRenderer::Line::Line() = default;

EZ_ALWAYS_INLINE ezDebugRenderer::Line::Line(const ezVec3& start, const ezVec3& end)
  : m_start(start)
  , m_end(end)
{
}

EZ_ALWAYS_INLINE ezDebugRenderer::Line::Line(const ezVec3& start, const ezVec3& end, const ezColor& color)
  : m_start(start)
  , m_end(end)
  , m_startColor(color)
  , m_endColor(color)
{
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE ezDebugRenderer::Triangle::Triangle() = default;

EZ_ALWAYS_INLINE ezDebugRenderer::Triangle::Triangle(const ezVec3& p0, const ezVec3& p1, const ezVec3& p2)

{
  m_position[0] = p0;
  m_position[1] = p1;
  m_position[2] = p2;
}
