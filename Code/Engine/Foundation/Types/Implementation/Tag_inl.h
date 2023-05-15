#pragma once

ezTag::ezTag()
  : m_uiBitIndex(0xFFFFFFFEu),
    m_uiBlockIndex(0xFFFFFFFEu)
{
}

bool ezTag::operator==(const ezTag& rhs) const
{
  return m_sTagString == rhs.m_sTagString;
}

bool ezTag::operator!=(const ezTag& rhs) const
{
  return m_sTagString != rhs.m_sTagString;
}

bool ezTag::operator<(const ezTag& rhs) const
{
  return m_sTagString < rhs.m_sTagString;
}

const ezString& ezTag::GetTagString() const
{
  return m_sTagString.GetString();
}

bool ezTag::IsValid() const
{
  return m_uiBlockIndex != 0xFFFFFFFEu;
}
