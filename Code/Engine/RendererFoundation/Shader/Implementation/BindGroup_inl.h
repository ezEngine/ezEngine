
ezGALBindGroupItem::ezGALBindGroupItem() { m_Dummy[0] = {}; }
ezGALBindGroupItem::ezGALBindGroupItem(const ezGALBindGroupItem& rhs)
{
  m_Flags = rhs.m_Flags;
  m_Dummy[0] = rhs.m_Dummy[0];
  m_Dummy[1] = rhs.m_Dummy[1];
  m_Dummy[2] = rhs.m_Dummy[2];
  m_Dummy[3] = rhs.m_Dummy[3];
}

void ezGALBindGroupItem::operator=(const ezGALBindGroupItem& rhs)
{
  m_Flags = rhs.m_Flags;
  m_Dummy[0] = rhs.m_Dummy[0];
  m_Dummy[1] = rhs.m_Dummy[1];
  m_Dummy[2] = rhs.m_Dummy[2];
  m_Dummy[3] = rhs.m_Dummy[3];
}
