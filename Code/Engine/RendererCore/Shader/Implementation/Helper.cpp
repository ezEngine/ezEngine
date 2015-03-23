#include <RendererCore/PCH.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <Foundation/Utilities/ConversionUtils.h>

void ezTextSectionizer::Clear()
{
  m_Sections.Clear();
  m_sText.Clear();
}

void ezTextSectionizer::AddSection(const char* szName)
{
  m_Sections.PushBack(ezTextSection(szName));
}

void ezTextSectionizer::Process(const char* szText)
{
  for (ezUInt32 i = 0; i < m_Sections.GetCount(); ++i)
    m_Sections[i].Reset();

  m_sText = szText;


  for (ezUInt32 s = 0; s < m_Sections.GetCount(); ++s)
  {
    m_Sections[s].m_szSectionStart = m_sText.FindSubString_NoCase(m_Sections[s].m_sName.GetData());

    if (m_Sections[s].m_szSectionStart != nullptr)
      m_Sections[s].m_Content = ezStringView(m_Sections[s].m_szSectionStart + m_Sections[s].m_sName.GetElementCount());
  }

  for (ezUInt32 s = 0; s < m_Sections.GetCount(); ++s)
  {
    if (m_Sections[s].m_szSectionStart == nullptr)
      continue;

    ezUInt32 uiLine = 1;

    const char* sz = m_sText.GetData();
    while (sz < m_Sections[s].m_szSectionStart)
    {
      if (*sz == '\n')
        ++uiLine;

      ++sz;
    }

    m_Sections[s].m_uiFirstLine = uiLine;

    for (ezUInt32 s2 = 0; s2 < m_Sections.GetCount(); ++s2)
    {
      if (s == s2)
        continue;

      if (m_Sections[s2].m_szSectionStart > m_Sections[s].m_szSectionStart)
      {
        const char* szContentStart = m_Sections[s].m_Content.GetStartPosition();
        const char* szSectionEnd = ezMath::Min(m_Sections[s].m_Content.GetEndPosition(), m_Sections[s2].m_szSectionStart);

        m_Sections[s].m_Content = ezStringView(szContentStart, szSectionEnd);
      }
    }
  }

}

ezStringView ezTextSectionizer::GetSectionContent(ezUInt32 uiSection, ezUInt32& out_uiFirstLine) const
{
  out_uiFirstLine = m_Sections[uiSection].m_uiFirstLine;
  return m_Sections[uiSection].m_Content;
}

void GetShaderSections(const char* szContent, ezTextSectionizer& out_Sections)
{
  out_Sections.Clear();

  out_Sections.AddSection("[PLATFORMS]");
  out_Sections.AddSection("[PERMUTATIONS]");
  out_Sections.AddSection("[RENDERSTATE]");
  out_Sections.AddSection("[VERTEXSHADER]");
  out_Sections.AddSection("[HULLSHADER]");
  out_Sections.AddSection("[DOMAINSHADER]");
  out_Sections.AddSection("[GEOMETRYSHADER]");
  out_Sections.AddSection("[PIXELSHADER]");
  out_Sections.AddSection("[COMPUTESHADER]");

  out_Sections.Process(szContent);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_Helper);

