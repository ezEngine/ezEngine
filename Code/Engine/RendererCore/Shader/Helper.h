#pragma once

#include <RendererCore/Basics.h>
#include <Foundation/Strings/String.h>

class ezTextSectionizer
{
public:
  void Clear();

  void AddSection(const char* szName);

  void Process(const char* szText);

  ezStringIterator GetSectionContent(ezUInt32 uiSection) const;

private:

  struct ezTextSection
  {
    ezTextSection(const char* szName)
    {
      m_sName = szName;
      m_szSectionStart = nullptr;
    }

    void Reset()
    {
      m_szSectionStart = nullptr;
      m_Content = ezStringIterator();
    }

    ezString m_sName;
    const char* m_szSectionStart;
    ezStringIterator m_Content;
  };

  ezStringBuilder m_sText;
  ezHybridArray<ezTextSection, 16> m_Sections;
};

enum ezShaderSections
{
  PLATFORMS,
  PERMUTATIONS,
  VERTEXSHADER,
  HULLSHADER,
  DOMAINSHADER,
  GEOMETRYSHADER,
  PIXELSHADER,
  COMPUTESHADER,
};

void GetShaderSections(const char* szContent, ezTextSectionizer& out_Sections);