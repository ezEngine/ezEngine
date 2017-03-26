namespace ezModelImporter
{
  inline void Mesh::Triangle::operator = (const Mesh::Triangle& rhs)
  {
    m_Vertices[0] = rhs.m_Vertices[0];
    m_Vertices[1] = rhs.m_Vertices[1];
    m_Vertices[2] = rhs.m_Vertices[2];
  }
}
