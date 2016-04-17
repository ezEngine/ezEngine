#include <RendererCore/PCH.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace ezShaderHelper
{

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

ezUInt32 CalculateHash(const ezArrayPtr<ezPermutationVar>& vars)
{
  ezStringBuilder s;

  for (auto& var : vars)
  {
    s.AppendFormat("%s = %s;", var.m_sName.GetData(), var.m_sValue.GetData());
  }

  return ezHashing::MurmurHash(ezHashing::StringWrapper(s.GetData()));
}

bool IsIdentifier(ezUInt32 c)
{
  return !ezStringUtils::IsIdentifierDelimiter_C_Code(c);
}

void ParsePermutationSection(ezStringView sPermutationSection, ezHybridArray<ezHashedString, 16>& out_PermVars)
{
  ezStringBuilder sToken;
  while (sPermutationSection.IsValid())
  {
    if (IsIdentifier(sPermutationSection.GetCharacter()))
    {
      sToken.Append(sPermutationSection.GetCharacter());
    }
    else if (!sToken.IsEmpty())
    {
      out_PermVars.ExpandAndGetRef().Assign(sToken.GetData());
      sToken.Clear();
    }

    ++sPermutationSection;
  }

  if (!sToken.IsEmpty())
  {
    out_PermVars.ExpandAndGetRef().Assign(sToken.GetData());
  }
}

void ParsePermutationVarConfig(ezStringView sPermutationVarConfig, ezVariant& out_DefaultValue, ezHybridArray<ezHashedString, 16>& out_EnumValues)
{
  if (sPermutationVarConfig.StartsWith("bool"))
  {
    bool bDefaultValue = false;

    const char* szDefaultValue = sPermutationVarConfig.FindSubString("=");
    if (!ezStringUtils::IsNullOrEmpty(szDefaultValue))
    {
      ++szDefaultValue;
      ezConversionUtils::StringToBool(szDefaultValue, bDefaultValue);
    }

    out_DefaultValue = bDefaultValue;
  }
  else if (sPermutationVarConfig.StartsWith("enum"))
  {
    const char* szOpenBracket = sPermutationVarConfig.FindSubString("{");
    const char* szCloseBracket = sPermutationVarConfig.FindLastSubString("}");

    if (ezStringUtils::IsNullOrEmpty(szOpenBracket) || ezStringUtils::IsNullOrEmpty(szCloseBracket))
    {
      ezLog::Error("No brackets found for enum definition.");
    }

    ezStringBuilder sEnumValues = ezStringView(szOpenBracket + 1, szCloseBracket);

    ezHybridArray<ezStringView, 32> enumValues;
    sEnumValues.Split(false, enumValues, ",");

    ezUInt32 uiDefaultValue = 0;
    ezUInt32 uiCurrentValue = 0;
    for (ezStringView& sName : enumValues)
    {
      sName.Trim(" \r\n\t");

      const char* szValue = sName.FindSubString("=");
      if (!ezStringUtils::IsNullOrEmpty(szValue))
      {
        sName = ezStringView(sName.GetStartPosition(), szValue);
        sName.Trim(" \r\n\t");

        ++szValue;

        ezInt32 iValue = 0;
        if (ezConversionUtils::StringToInt(szValue, iValue).Succeeded() && iValue >= 0)
        {
          uiCurrentValue = iValue;
        }
        else
        {
          ezLog::Error("Invalid enum value '%s'. Only positive numbers are allowed.", szValue);
        }
      }

      if (sName.IsEqual_NoCase("default"))
      {
        uiDefaultValue = uiCurrentValue;
      }

      if (out_EnumValues.GetCount() <= uiCurrentValue)
      {
        out_EnumValues.SetCount(uiCurrentValue + 1);
      }

      if (ezStringUtils::IsNullOrEmpty(out_EnumValues[uiCurrentValue].GetData()))
      {
        ezString sFinalName = sName;
        out_EnumValues[uiCurrentValue].Assign(sFinalName.GetData());
      }
      else
      {
        ezLog::Error("A enum value with '%d' already exists: '%s'", uiCurrentValue, out_EnumValues[uiCurrentValue].GetData());
      }

      ++uiCurrentValue;
    }

    out_DefaultValue = uiDefaultValue;
  }
  else
  {
    ezLog::Error("Unknown permutation var type");
  }
}

}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_Helper);

