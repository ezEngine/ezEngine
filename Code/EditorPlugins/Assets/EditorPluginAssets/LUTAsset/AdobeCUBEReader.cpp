
#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/AdobeCUBEReader.h>
#include <Foundation/CodeUtils/Tokenizer.h>

// This file implements a simple reader for the Adobe CUBE LUT file format.
// The specification can be found here (at the time of this writing):
// https://wwwimages2.adobe.com/content/dam/acom/en/products/speedgrade/cc/pdfs/cube-lut-specification-1.0.pdf

namespace
{
  bool GetVec3FromLine(ezHybridArray<const ezToken*, 32> line, ezUInt32 skip, ezVec3& out)
  {
    if (line.GetCount() < (skip + 3 + 2))
    {
      return false;
    }

    if (
      (line[skip + 0]->m_iType != ezTokenType::Float && line[skip + 0]->m_iType != ezTokenType::Integer) ||
      line[skip + 1]->m_iType != ezTokenType::Whitespace ||
      (line[skip + 2]->m_iType != ezTokenType::Float && line[skip + 2]->m_iType != ezTokenType::Integer) ||
      line[skip + 3]->m_iType != ezTokenType::Whitespace ||
      (line[skip + 4]->m_iType != ezTokenType::Float && line[skip + 4]->m_iType != ezTokenType::Integer)
      )
    {
      return false;
    }

    double res = 0;
    ezString sVal = line[skip + 0]->m_DataView;

    if (ezConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    out.x = static_cast<float>(res);

    sVal = line[skip + 2]->m_DataView;
    if (ezConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    out.y = static_cast<float>(res);


    sVal = line[skip + 4]->m_DataView;
    if (ezConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    out.z = static_cast<float>(res);

    return true;
  }
}

ezAdobeCUBEReader::ezAdobeCUBEReader() = default;
ezAdobeCUBEReader::~ezAdobeCUBEReader() = default;

ezStatus ezAdobeCUBEReader::ParseFile(ezStreamReader& Stream, ezLogInterface* pLog /*= nullptr*/)
{
  ezString sContent;
  sContent.ReadAll(Stream);

  ezTokenizer tokenizer;
  tokenizer.SetTreatHashSignAsLineComment(true);

  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)sContent.GetData(), sContent.GetElementCount()), pLog ? pLog : ezLog::GetThreadLocalLogSystem());


  auto tokens = tokenizer.GetTokens();

  ezHybridArray<const ezToken*, 32> line;
  ezUInt32 firstToken = 0;

  while (tokenizer.GetNextLine(firstToken, line).Succeeded())
  {
    if(line[0]->m_iType == ezTokenType::LineComment || line[0]->m_iType == ezTokenType::Newline)
      continue;

    if (line[0]->m_DataView == "TITLE")
    {
      if (line.GetCount() < 3)
      {
        return ezStatus(ezFmt("LUT file has invalid TITLE line."));
      }

      if (line[1]->m_iType != ezTokenType::Whitespace && line[2]->m_iType != ezTokenType::String1)
      {
        return ezStatus(ezFmt("LUT file has invalid TITLE line, expected TITLE<whitespace>\"<string>\"."));
      }

      m_sTitle = line[2]->m_DataView;

      continue;
    }
    else if (line[0]->m_DataView == "DOMAIN_MIN")
    {
      if (!::GetVec3FromLine(line, 2, m_DomainMin))
      {
        return ezStatus(ezFmt("LUT file has invalid DOMAIN_MIN line."));
      }

      continue;
    }
    else if (line[0]->m_DataView == "DOMAIN_MAX")
    {
      if (!::GetVec3FromLine(line, 2, m_DomainMax))
      {
        return ezStatus(ezFmt("LUT file has invalid DOMAIN_MAX line."));
      }

      continue;
    }
    else if (line[0]->m_DataView == "LUT_1D_SIZE")
    {
      return ezStatus(ezFmt("LUT file specifies a 1D LUT which is currently not implemented."));
    }
    else if (line[0]->m_DataView == "LUT_3D_SIZE")
    {
      if (m_uiLUTSize > 0)
      {
        return ezStatus(ezFmt("LUT file has more than one LUT_3D_SIZE entry. Aborting parse."));
      }

      if (line.GetCount() < 3)
      {
        return ezStatus(ezFmt("LUT file has invalid LUT_3D_SIZE line."));
      }

      if (line[1]->m_iType != ezTokenType::Whitespace && line[2]->m_iType != ezTokenType::Integer)
      {
        return ezStatus(ezFmt("LUT file has invalid LUT_3D_SIZE line, expected LUT_3D_SIZE<whitespace><N>."));
      }

      const ezString sVal = line[2]->m_DataView;
      if (ezConversionUtils::StringToUInt(sVal, m_uiLUTSize).Failed())
      {
        return ezStatus(ezFmt("LUT file has invalid LUT_3D_SIZE line, couldn't parse LUT size as ezUInt32."));
      }

      if (m_uiLUTSize < 2 || m_uiLUTSize > 256)
      {
        return ezStatus(ezFmt("LUT file has invalid LUT_3D_SIZE size, got {0} - but must be in range 2, 256.", m_uiLUTSize));
      }

      m_LUTValues.Reserve(m_uiLUTSize * m_uiLUTSize * m_uiLUTSize);

      continue;
    }

    if (line[0]->m_iType == ezTokenType::Float || line[0]->m_iType == ezTokenType::Integer)
    {
      if (m_uiLUTSize == 0)
      {
        return ezStatus(ezFmt("LUT data before LUT size was specified."));
      }

      ezVec3 lineValues;
      if (!::GetVec3FromLine(line, 0, lineValues))
      {
        return ezStatus(ezFmt("LUT data couldn't be read."));
      }

      m_LUTValues.PushBack(lineValues);
    }
  }

  if (m_DomainMin.x > m_DomainMax.x || m_DomainMin.y > m_DomainMax.y || m_DomainMin.z > m_DomainMax.z)
  {
    return ezStatus("LUT file has invalid domain min/max values.");
  }

  if (m_LUTValues.GetCount() != (m_uiLUTSize * m_uiLUTSize * m_uiLUTSize))
  {
    return ezStatus(ezFmt("LUT data incomplete, read {0} values but expected {1} values given a LUT size of {2}.", m_LUTValues.GetCount(), (m_uiLUTSize * m_uiLUTSize * m_uiLUTSize), m_uiLUTSize));
  }

  return ezStatus(EZ_SUCCESS);
}

ezVec3 ezAdobeCUBEReader::GetDomainMin() const
{
  return m_DomainMin;
}

ezVec3 ezAdobeCUBEReader::GetDomainMax() const
{
  return m_DomainMax;
}

ezUInt32 ezAdobeCUBEReader::GetLUTSize() const
{
  return m_uiLUTSize;
}

const ezString& ezAdobeCUBEReader::GetTitle() const
{
  return m_sTitle;
}

ezVec3 ezAdobeCUBEReader::GetLUTEntry(ezUInt32 r, ezUInt32 g, ezUInt32 b) const
{
  return m_LUTValues[GetLUTIndex(r, g, b)];
}

ezUInt32 ezAdobeCUBEReader::GetLUTIndex(ezUInt32 r, ezUInt32 g, ezUInt32 b) const
{
  return b * m_uiLUTSize * m_uiLUTSize + g * m_uiLUTSize + r;
}
