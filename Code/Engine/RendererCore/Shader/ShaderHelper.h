#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

namespace ezShaderHelper
{
  class EZ_RENDERERCORE_DLL ezTextSectionizer
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
        : m_sName(szName)

      {
      }

      void Reset()
      {
        m_szSectionStart = nullptr;
        m_Content = ezStringView();
        m_uiFirstLine = 0;
      }

      ezString m_sName;
      const char* m_szSectionStart = nullptr;
      ezStringView m_Content;
      ezUInt32 m_uiFirstLine = 0;
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
      MATERIALCONFIG,
      MATERIALCONSTANTS,
      RENDERSTATE,
      SHADER,
      VERTEXSHADER,
      HULLSHADER,
      DOMAINSHADER,
      GEOMETRYSHADER,
      PIXELSHADER,
      COMPUTESHADER,
      TEMPLATE_VARS
    };
  };

  EZ_RENDERERCORE_DLL void GetShaderSections(const char* szContent, ezTextSectionizer& out_sections);

  ezUInt32 CalculateHash(const ezArrayPtr<ezPermutationVar>& vars);
} // namespace ezShaderHelper
