#pragma once

#include <RendererCore/Basics.h>
#include <Foundation/Strings/String.h>

class ezTextSectionizer
{
public:
  void Clear();

  void AddSection(const char* szName);

  void Process(const char* szText);

  ezStringView GetSectionContent(ezUInt32 uiSection, ezUInt32& out_uiFirstLine) const;

private:

  struct ezTextSection
  {
    ezTextSection(const char* szName)
    {
      m_sName = szName;
      m_szSectionStart = nullptr;
      m_uiFirstLine = 0;
    }

    void Reset()
    {
      m_szSectionStart = nullptr;
      m_Content = ezStringView();
      m_uiFirstLine = 0;
    }

    ezString m_sName;
    const char* m_szSectionStart;
    ezStringView m_Content;
    ezUInt32 m_uiFirstLine;
  };

  ezStringBuilder m_sText;
  ezHybridArray<ezTextSection, 16> m_Sections;
};

enum ezShaderSections
{
  PLATFORMS,
  PERMUTATIONS,
  RENDERSTATE,
  VERTEXSHADER,
  HULLSHADER,
  DOMAINSHADER,
  GEOMETRYSHADER,
  PIXELSHADER,
  COMPUTESHADER,
};

void GetShaderSections(const char* szContent, ezTextSectionizer& out_Sections);