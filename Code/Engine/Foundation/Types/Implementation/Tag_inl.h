
#pragma once

ezTag::ezTag()
  : m_uiBlockIndex(0xFFFFFFFEu)
{
}

bool ezTag::operator == (const ezTag& rhs) const
{
  return m_TagString == rhs.m_TagString;
}

bool ezTag::operator != (const ezTag& rhs) const
{
  return m_TagString != rhs.m_TagString;
}

bool ezTag::operator < (const ezTag& rhs) const
{
  return m_TagString < rhs.m_TagString;
}

const ezString& ezTag::GetTagString() const
{
  return m_TagString.GetString();
}

ezUInt32 ezTag::GetTagHash() const
{
  return m_TagString.GetHash();
}

bool ezTag::IsValid() const
{
  return m_uiBlockIndex != 0xFFFFFFFEu;
}
