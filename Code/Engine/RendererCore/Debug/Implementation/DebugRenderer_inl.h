
EZ_FORCE_INLINE ezDebugRenderer::Line::Line()
{
}

EZ_FORCE_INLINE ezDebugRenderer::Line::Line(const ezVec3& start, const ezVec3& end)
  : m_start(start), m_end(end)
{
}

//////////////////////////////////////////////////////////////////////////

EZ_FORCE_INLINE ezDebugRenderer::Triangle::Triangle()
{
}

EZ_FORCE_INLINE ezDebugRenderer::Triangle::Triangle(const ezVec3& p0, const ezVec3& p1, const ezVec3& p2)
  : m_p0(p0), m_p1(p1), m_p2(p2)
{
}
