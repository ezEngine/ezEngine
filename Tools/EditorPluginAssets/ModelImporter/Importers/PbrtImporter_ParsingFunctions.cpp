#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/Importers/PbrtImporter_ParsingFunctions.h>

#include <Foundation/Logging/Log.h>

namespace ezModelImporter
{
  using namespace Pbrt;

  namespace PbrtParseHelper
  {
    void SkipCurrentLine(ezStringView& string)
    {
      while (string.IsValid() && string.GetCharacter() != '\0' && string.GetCharacter() != '\r' && string.GetCharacter() != '\n')
        ++string;
    }

    void SkipWhiteSpaces(ezStringView& string)
    {
      // Skip trailing whitespace.
      while (string.IsValid() && string.GetCharacter() != '\0' && ezStringUtils::IsWhiteSpace(string.GetCharacter()))
        ++string;

      // Skip comments as well.
      if (string.GetCharacter() == '#')
      {
        SkipCurrentLine(string);
        SkipWhiteSpaces(string);
      }
    }

    ezStringView ReadCommandName(ezStringView& remainingSceneText)
    {
      SkipWhiteSpaces(remainingSceneText);

      const char* begin = remainingSceneText.GetData();
      while (remainingSceneText.GetCharacter() != '\0' && remainingSceneText.GetCharacter() != '\r' && remainingSceneText.GetCharacter() != '\n' &&
        !ezStringUtils::IsWhiteSpace(remainingSceneText.GetCharacter()))
      {
        ++remainingSceneText;
      }

      ezStringView commandName = ezStringView(begin, remainingSceneText.GetData());

      SkipWhiteSpaces(remainingSceneText);

      // Manipulates the data source. This is semantically a bad thing to do, but the this const_cast saves us a lot of string copying.
      ezStringUtils::ToLowerString(const_cast<char*>(commandName.GetData()), commandName.GetEndPosition());

      return commandName;
    }

    ezStringView ReadBlock(ezStringView& remainingSceneText, char blockStart, char blockEnd)
    {
      SkipWhiteSpaces(remainingSceneText);

      if (remainingSceneText.GetCharacter() != blockStart)
        return nullptr;

      ++remainingSceneText;
      const char* begin = remainingSceneText.GetData();
      while (remainingSceneText.GetCharacter() != '\0' && remainingSceneText.GetCharacter() != blockEnd)
      {
        ++remainingSceneText;
      }

      const char* end = remainingSceneText.GetData();

      if (remainingSceneText.GetCharacter() == blockEnd)
        ++remainingSceneText;

      return ezStringView(begin, end);
    }

    ParamType GetParamType(const ezStringView& type)
    {
      ezUInt32 stringLen = static_cast<ezUInt32>(type.GetEndPosition() - type.GetStartPosition());

      if (ezStringUtils::IsEqualN_NoCase(type.GetData(), "float", stringLen, type.GetEndPosition()))
        return ParamType::FLOAT;
      else if (ezStringUtils::IsEqualN_NoCase(type.GetData(), "string", stringLen, type.GetEndPosition()))
        return ParamType::STRING;
      else if (ezStringUtils::IsEqualN_NoCase(type.GetData(), "spectrum", stringLen, type.GetEndPosition()))
        return ParamType::SPECTRUM;
      else if (ezStringUtils::IsEqualN_NoCase(type.GetData(), "texture", stringLen, type.GetEndPosition()))
        return ParamType::TEXTURE;
      else if (ezStringUtils::IsEqualN_NoCase(type.GetData(), "integer", stringLen, type.GetEndPosition()))
        return ParamType::INT;
      else if (ezStringUtils::IsEqualN_NoCase(type.GetData(), "point", stringLen, type.GetEndPosition()) ||
        ezStringUtils::IsEqualN_NoCase(type.GetData(), "normal", stringLen, type.GetEndPosition()) ||
        ezStringUtils::IsEqualN_NoCase(type.GetData(), "vector", stringLen, type.GetEndPosition()))
        return ParamType::VECTOR3;
      else if (ezStringUtils::IsEqualN_NoCase(type.GetData(), "rgb", stringLen, type.GetEndPosition()) ||
               ezStringUtils::IsEqualN_NoCase(type.GetData(), "color", stringLen, type.GetEndPosition()))
        return ParamType::COLOR;
      else if (ezStringUtils::IsEqualN_NoCase(type.GetData(), "bool", stringLen, type.GetEndPosition()))
        return ParamType::BOOL;
      else
        return ParamType::INVALID;
    }

    ezResult ParseVec3(ezStringView& params, ezVec3& out)
    {
      double x, y, z;
      const char* newStartPos;
      if (ezConversionUtils::StringToFloat(params.GetData(), x, &newStartPos).Failed() ||
        ezConversionUtils::StringToFloat(newStartPos, y, &newStartPos).Failed() ||
        ezConversionUtils::StringToFloat(newStartPos, z, &newStartPos).Failed())
        return EZ_FAILURE;
      else
      {
        out = ezVec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
        params.SetStartPosition(newStartPos);
        return EZ_SUCCESS;
      }
    }

    ezResult ParseFloats(ezStringView& params, ezArrayPtr<float> outFloats, int numExpectedFloats)
    {
      const char* startPos = params.GetStartPosition();
      for (int i = 0; i < numExpectedFloats; ++i)
      {
        SkipWhiteSpaces(params);
        if (*startPos == '[')
          ++startPos;
        if (*startPos == ']')
          return EZ_FAILURE;

        double value;
        if (ezConversionUtils::StringToFloat(startPos, value, &startPos).Failed())
          return EZ_FAILURE;

        outFloats[i] = static_cast<float>(value);
        params.SetStartPosition(startPos);
      }

      if (params.GetCharacter() == ']')
        ++params;

      return EZ_SUCCESS;
    }

    Parameter::DataArray ParseParameterBlock(ParamType type, ezStringView& remainingSceneText)
    {
      ezStringView params;
      if (type != ParamType::STRING && type != ParamType::TEXTURE && type != ParamType::SPECTRUM)
      {
        params = ReadBlock(remainingSceneText, '[', ']');
        if (!params.IsValid())
        {
          // No brackets, so it still can be a single value delimited by newline or whitespace.
          SkipWhiteSpaces(remainingSceneText);

          const char* begin = remainingSceneText.GetStartPosition();
          while (!ezStringUtils::IsWhiteSpace(*remainingSceneText.GetStartPosition()))
            ++remainingSceneText;

          params = ezStringView(begin, remainingSceneText.GetStartPosition());
        }
        SkipWhiteSpaces(params);
      }
      else
      {
        const char* start = remainingSceneText.FindSubString("\"") + 1;
        const char* end = remainingSceneText.FindSubString("\"", start);
        if (start == nullptr || end == nullptr)
        {
          ezLog::Error("Failed to parse parameter, expected string.");
          return Parameter::DataArray();
        }
        params = ezStringView(start, end);
        remainingSceneText.SetStartPosition(end + 1);
      }

      Parameter::DataArray entries;

      bool stopParsing = false;
      while (params.IsValid() && params.GetElementCount() > 0 && !stopParsing)
      {
        switch (type)
        {
        case ParamType::FLOAT:
        {
          double d = 0;
          const char* newStartPos = params.GetData();
          if (ezConversionUtils::StringToFloat(params.GetData(), d, &newStartPos).Failed())
            stopParsing = true;
          else
            entries.PushBack(static_cast<float>(d));
          params.SetStartPosition(newStartPos);
          break;
        }
        case ParamType::INT:
        {
          ezInt32 i = 0;
          const char* newStartPos = params.GetData();
          if (ezConversionUtils::StringToInt(params.GetData(), i, &newStartPos).Failed())
            stopParsing = true;
          else
            entries.PushBack(i);
          params.SetStartPosition(newStartPos);
          break;
        }

        case ParamType::STRING:
        case ParamType::TEXTURE:
        case ParamType::SPECTRUM:
        {
          ezString actualString(params);
          entries.PushBack(actualString);
          stopParsing = true;
          break;
        }

        case ParamType::VECTOR3:
        {
          ezVec3 vec;
          if (ParseVec3(params, vec).Failed())
            stopParsing = true;
          else
            entries.PushBack(vec);
          break;
        }
        case ParamType::COLOR:
        {
          ezVec3 vec;
          if (ParseVec3(params, vec).Failed())
            stopParsing = true;
          else
            entries.PushBack(ezColor(vec.x, vec.y, vec.z));
          break;
        }
        case ParamType::BOOL:
        {
          if (params.IsEqual_NoCase("\"true\""))
            entries.PushBack(true);
          else
            entries.PushBack(false);
          const char* next = params.FindSubString(" ");
          params.SetStartPosition(next ? next : params.GetEndPosition());
          break;
        }

        default:
          return Parameter::DataArray();
        }

        SkipWhiteSpaces(params);
      }

      return entries;
    }
  }
}
