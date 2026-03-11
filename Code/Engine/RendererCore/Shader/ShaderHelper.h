#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

namespace ezShaderHelper
{
  /// Parses shader source code into named sections.
  ///
  /// Shader files are divided into sections like [PLATFORMS], [PERMUTATIONS], [VERTEXSHADER], etc.
  /// This class extracts these sections for further processing during shader compilation.
  class EZ_RENDERERCORE_DLL ezTextSectionizer
  {
  public:
    void Clear();

    /// Registers a section name to look for during parsing.
    void AddSection(const char* szName);

    /// Parses the text and extracts all registered sections.
    void Process(const char* szText);

    /// Returns the content of a specific section and its starting line number.
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

  /// Defines the standard section names in shader files.
  struct ezShaderSections
  {
    enum Enum
    {
      PLATFORMS,         ///< Platform requirements
      PERMUTATIONS,      ///< Shader permutation variables
      MATERIALPARAMETER, ///< Material parameters exposed to materials
      MATERIALCONFIG,    ///< Material configuration
      MATERIALCONSTANTS, ///< Material constant definitions
      RENDERSTATE,       ///< Render state configuration
      SHADER,            ///< Shared shader code
      VERTEXSHADER,      ///< Vertex shader entry point
      HULLSHADER,        ///< Hull shader entry point (tessellation)
      DOMAINSHADER,      ///< Domain shader entry point (tessellation)
      GEOMETRYSHADER,    ///< Geometry shader entry point
      PIXELSHADER,       ///< Pixel shader entry point
      COMPUTESHADER,     ///< Compute shader entry point
      TEMPLATE_VARS      ///< Template variables for shader generation
    };
  };

  /// Extracts all standard sections from shader source code.
  EZ_RENDERERCORE_DLL void GetShaderSections(const char* szContent, ezTextSectionizer& out_sections);

  /// Calculates a hash from permutation variables for shader variant identification.
  ezUInt32 CalculateHash(const ezArrayPtr<ezPermutationVar>& vars);
} // namespace ezShaderHelper
