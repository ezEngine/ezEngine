#include <Texture/TexturePCH.h>

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Texture/Image/Formats/CubeLutFileFormat.h>
#include <Texture/Image/Image.h>

// This file implements a reader for the Adobe CUBE LUT file format.
// The specification can be found here (at the time of this writing):
// https://wwwimages2.adobe.com/content/dam/acom/en/products/speedgrade/cc/pdfs/cube-lut-specification-1.0.pdf

EZ_STATICLINK_FORCE static ezImageFileFormatRegistrator<ezCubeLutFileFormat> g_cubeLutFormat;

namespace
{
  bool IsNumberToken(const ezToken* pToken)
  {
    return pToken->m_iType == ezTokenType::Float || pToken->m_iType == ezTokenType::Integer;
  }

  /// Whether the line starts with a number. The tokenizer doesn't treat a sign or a leading dot as part of a number
  /// ("-0.5" is a NonIdentifier token followed by a Float token), so those are checked separately.
  bool IsNumberLine(ezArrayPtr<const ezToken*> line)
  {
    if (IsNumberToken(line[0]))
      return true;

    return line.GetCount() > 1 && (line[0]->m_DataView == "-" || line[0]->m_DataView == "+" || line[0]->m_DataView == ".") && IsNumberToken(line[1]);
  }

  /// Parses exactly three whitespace separated numbers, starting at token uiSkip.
  ///
  /// The token texts are joined and split at whitespace again, because the tokenizer splits signed numbers
  /// into several tokens (see IsNumberLine()). Trailing comments are ignored.
  bool GetVec3FromLine(ezArrayPtr<const ezToken*> line, ezUInt32 uiSkip, ezVec3& ref_vOut)
  {
    ezStringBuilder sNumbers;
    for (ezUInt32 i = uiSkip; i < line.GetCount(); ++i)
    {
      const ezToken* pToken = line[i];

      if (pToken->m_iType == ezTokenType::LineComment || pToken->m_iType == ezTokenType::Newline)
        break;

      if (pToken->m_iType == ezTokenType::Whitespace)
        sNumbers.Append(" ");
      else
        sNumbers.Append(pToken->m_DataView);
    }

    ezHybridArray<ezStringView, 4> parts;
    sNumbers.Split(false, parts, " ");

    if (parts.GetCount() != 3)
      return false;

    for (ezUInt32 i = 0; i < 3; ++i)
    {
      double fRes = 0;
      // reject trailing garbage, StringToFloat() succeeds as soon as it finds a number at the start
      const char* szParseEnd = nullptr;
      if (ezConversionUtils::StringToFloat(parts[i], fRes, &szParseEnd).Failed() || szParseEnd != parts[i].GetEndPointer())
        return false;

      ref_vOut.GetData()[i] = static_cast<float>(fRes);
    }

    return true;
  }

  /// Parses the CUBE file. If pValues is null, only the header lines are parsed and data lines are counted but not converted.
  ezResult ParseCubeFile(ezStreamReader& inout_stream, ezUInt32& out_uiLUTSize, ezDynamicArray<ezVec3>* pValues)
  {
    out_uiLUTSize = 0;

    ezString sContent;
    sContent.ReadAll(inout_stream);

    ezTokenizer tokenizer;
    tokenizer.SetTreatHashSignAsLineComment(true);
    tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)sContent.GetData(), sContent.GetElementCount()), ezLog::GetThreadLocalLogSystem());

    ezVec3 vDomainMin = ezVec3::MakeZero();
    ezVec3 vDomainMax = ezVec3(1.0f);
    ezUInt32 uiNumValues = 0;

    ezTempHybridArray<const ezToken*, 32> line;
    ezUInt32 uiFirstToken = 0;

    while (tokenizer.GetNextLine(uiFirstToken, line).Succeeded())
    {
      if (line[0]->m_iType == ezTokenType::Whitespace)
      {
        line.RemoveAtAndCopy(0);

        if (line.IsEmpty())
          continue;
      }

      if (line[0]->m_iType == ezTokenType::LineComment || line[0]->m_iType == ezTokenType::Newline)
        continue;

      if (line[0]->m_DataView == "TITLE")
      {
        // the title is not needed
        continue;
      }
      else if (line[0]->m_DataView == "DOMAIN_MIN")
      {
        if (!GetVec3FromLine(line, 2, vDomainMin))
        {
          ezLog::Error("CUBE file has invalid DOMAIN_MIN line.");
          return EZ_FAILURE;
        }

        continue;
      }
      else if (line[0]->m_DataView == "DOMAIN_MAX")
      {
        if (!GetVec3FromLine(line, 2, vDomainMax))
        {
          ezLog::Error("CUBE file has invalid DOMAIN_MAX line.");
          return EZ_FAILURE;
        }

        continue;
      }
      else if (line[0]->m_DataView == "LUT_1D_SIZE")
      {
        ezLog::Error("CUBE file specifies a 1D LUT, which is not supported.");
        return EZ_FAILURE;
      }
      else if (line[0]->m_DataView == "LUT_3D_SIZE")
      {
        if (out_uiLUTSize > 0)
        {
          ezLog::Error("CUBE file has more than one LUT_3D_SIZE entry.");
          return EZ_FAILURE;
        }

        if (line.GetCount() < 3 || line[1]->m_iType != ezTokenType::Whitespace || line[2]->m_iType != ezTokenType::Integer)
        {
          ezLog::Error("CUBE file has invalid LUT_3D_SIZE line, expected LUT_3D_SIZE<whitespace><N>.");
          return EZ_FAILURE;
        }

        if (ezConversionUtils::StringToUInt(line[2]->m_DataView, out_uiLUTSize).Failed())
        {
          ezLog::Error("CUBE file has invalid LUT_3D_SIZE line, couldn't parse LUT size.");
          return EZ_FAILURE;
        }

        if (out_uiLUTSize < 2 || out_uiLUTSize > 256)
        {
          ezLog::Error("CUBE file has invalid LUT_3D_SIZE {}, must be in range [2; 256].", out_uiLUTSize);
          return EZ_FAILURE;
        }

        if (pValues)
        {
          pValues->Reserve(out_uiLUTSize * out_uiLUTSize * out_uiLUTSize);
        }

        continue;
      }

      if (IsNumberLine(line))
      {
        if (out_uiLUTSize == 0)
        {
          ezLog::Error("CUBE file has LUT data before LUT_3D_SIZE was specified.");
          return EZ_FAILURE;
        }

        ++uiNumValues;

        if (pValues)
        {
          ezVec3 vValue;
          if (!GetVec3FromLine(line, 0, vValue))
          {
            ezLog::Error("CUBE file has an invalid LUT data line.");
            return EZ_FAILURE;
          }

          pValues->PushBack(vValue);
        }

        continue;
      }

      // the spec allows unknown keywords (e.g. LUT_3D_INPUT_RANGE from other tools), ignore them
      ezLog::Dev("CUBE file: ignoring line starting with '{}'.", line[0]->m_DataView);
    }

    if (out_uiLUTSize == 0)
    {
      ezLog::Error("CUBE file has no LUT_3D_SIZE entry.");
      return EZ_FAILURE;
    }

    if (vDomainMin.x > vDomainMax.x || vDomainMin.y > vDomainMax.y || vDomainMin.z > vDomainMax.z)
    {
      ezLog::Error("CUBE file has invalid DOMAIN_MIN / DOMAIN_MAX values.");
      return EZ_FAILURE;
    }

    const ezUInt32 uiExpectedValues = out_uiLUTSize * out_uiLUTSize * out_uiLUTSize;
    if (uiNumValues != uiExpectedValues)
    {
      ezLog::Error("CUBE file data is incomplete, read {} values but expected {} values for a LUT size of {}.", uiNumValues, uiExpectedValues, out_uiLUTSize);
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  void SetupHeader(ezImageHeader& ref_header, ezUInt32 uiLUTSize)
  {
    ref_header = ezImageHeader();
    ref_header.SetImageFormat(ezImageFormat::R32G32B32A32_FLOAT);
    ref_header.SetWidth(uiLUTSize);
    ref_header.SetHeight(uiLUTSize);
    ref_header.SetDepth(uiLUTSize);
  }
} // namespace

ezResult ezCubeLutFileFormat::ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(sFileExtension);

  ezUInt32 uiLUTSize = 0;
  EZ_SUCCEED_OR_RETURN(ParseCubeFile(inout_stream, uiLUTSize, nullptr));

  SetupHeader(ref_header, uiLUTSize);
  return EZ_SUCCESS;
}

ezResult ezCubeLutFileFormat::ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(sFileExtension);

  ezUInt32 uiLUTSize = 0;
  ezDynamicArray<ezVec3> values;
  EZ_SUCCEED_OR_RETURN(ParseCubeFile(inout_stream, uiLUTSize, &values));

  ezImageHeader header;
  SetupHeader(header, uiLUTSize);
  ref_image.ResetAndAlloc(header);

  // CUBE data is ordered with red changing fastest, then green, then blue
  ezUInt32 uiIndex = 0;
  for (ezUInt32 b = 0; b < uiLUTSize; ++b)
  {
    for (ezUInt32 g = 0; g < uiLUTSize; ++g)
    {
      ezColor* pRow = ref_image.GetPixelPointer<ezColor>(0, 0, 0, 0, g, b);

      for (ezUInt32 r = 0; r < uiLUTSize; ++r, ++uiIndex)
      {
        const ezVec3& v = values[uiIndex];
        pRow[r] = ezColor(v.x, v.y, v.z, 1.0f);
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezCubeLutFileFormat::WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(inout_stream);
  EZ_IGNORE_UNUSED(image);
  EZ_IGNORE_UNUSED(sFileExtension);
  return EZ_FAILURE;
}

bool ezCubeLutFileFormat::CanReadFileType(ezStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("cube");
}

bool ezCubeLutFileFormat::CanWriteFileType(ezStringView sExtension) const
{
  EZ_IGNORE_UNUSED(sExtension);
  return false;
}

EZ_STATICLINK_FILE(Texture, Texture_Image_Formats_CubeLutFileFormat);
