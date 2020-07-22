#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

namespace ezShaderHelper
{
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

  struct ezShaderSections
  {
    enum Enum
    {
      PLATFORMS,
      PERMUTATIONS,
      MATERIALPARAMETER,
      RENDERSTATE,
      SHADER,
      VERTEXSHADER,
      HULLSHADER,
      DOMAINSHADER,
      GEOMETRYSHADER,
      PIXELSHADER,
      COMPUTESHADER
    };
  };

  void GetShaderSections(const char* szContent, ezTextSectionizer& out_Sections);

  ezUInt32 CalculateHash(const ezArrayPtr<ezPermutationVar>& vars);
} // namespace ezShaderHelper
