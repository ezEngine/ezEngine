#include <Foundation/PCH.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/ConversionUtils.h>

void ezOpenDdlWriter::OutputEscapedString(const ezStringView& string)
{
  m_Temp = string;
  m_Temp.ReplaceAll("\\", "\\\\");
  m_Temp.ReplaceAll("\"", "\\\"");
  m_Temp.ReplaceAll("\b", "\\b");
  m_Temp.ReplaceAll("\r", "\\r");
  m_Temp.ReplaceAll("\f", "\\f");
  m_Temp.ReplaceAll("\n", "\\n");
  m_Temp.ReplaceAll("\t", "\\t");

  OutputString("\"", 1);
  OutputString(m_Temp.GetData());
  OutputString("\"", 1);
}

void ezOpenDdlWriter::OutputIndentation()
{
  if (m_bCompactMode)
    return;

  ezInt32 iIndentation = m_iIndentation;

  // I need my space!
  const char* szIndentation = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

  while (iIndentation >= 16)
  {
    OutputString(szIndentation, 16);
    iIndentation -= 16;
  }

  if (iIndentation > 0)
  {
    OutputString(szIndentation, iIndentation);
  }
}

void ezOpenDdlWriter::OutputPrimitiveTypeNameCompliant(ezOpenDdlPrimitiveType type)
{
  switch (type)
  {
  case ezOpenDdlPrimitiveType::Bool:
    OutputString("bool", 4);
    break;
  case ezOpenDdlPrimitiveType::Int8:
    OutputString("int8", 4);
    break;
  case ezOpenDdlPrimitiveType::Int16:
    OutputString("int16", 5);
    break;
  case ezOpenDdlPrimitiveType::Int32:
    OutputString("int32", 5);
    break;
  case ezOpenDdlPrimitiveType::Int64:
    OutputString("int64", 5);
    break;
  case ezOpenDdlPrimitiveType::UInt8:
    OutputString("unsigned_int8", 13);
    break;
  case ezOpenDdlPrimitiveType::UInt16:
    OutputString("unsigned_int16", 14);
    break;
  case ezOpenDdlPrimitiveType::UInt32:
    OutputString("unsigned_int32", 14);
    break;
  case ezOpenDdlPrimitiveType::UInt64:
    OutputString("unsigned_int64", 14);
    break;
  case ezOpenDdlPrimitiveType::Float:
    OutputString("float", 5);
    break;
  case ezOpenDdlPrimitiveType::Double:
    OutputString("double", 6);
    break;
  case ezOpenDdlPrimitiveType::String:
    OutputString("string", 6);
    break;

  default:
    EZ_REPORT_FAILURE("Unknown DDL primitive type %u", type);
    break;
  }
}
void ezOpenDdlWriter::OutputPrimitiveTypeNameShort(ezOpenDdlPrimitiveType type)
{
  // Change to OpenDDL: We write uint8 etc. instead of unsigned_int

  switch (type)
  {
  case ezOpenDdlPrimitiveType::Bool:
    OutputString("bool", 4);
    break;
  case ezOpenDdlPrimitiveType::Int8:
    OutputString("int8", 4);
    break;
  case ezOpenDdlPrimitiveType::Int16:
    OutputString("int16", 5);
    break;
  case ezOpenDdlPrimitiveType::Int32:
    OutputString("int32", 5);
    break;
  case ezOpenDdlPrimitiveType::Int64:
    OutputString("int64", 5);
    break;
  case ezOpenDdlPrimitiveType::UInt8:
    OutputString("uint8", 5);
    break;
  case ezOpenDdlPrimitiveType::UInt16:
    OutputString("uint16", 6);
    break;
  case ezOpenDdlPrimitiveType::UInt32:
    OutputString("uint32", 6);
    break;
  case ezOpenDdlPrimitiveType::UInt64:
    OutputString("uint64", 6);
    break;
  case ezOpenDdlPrimitiveType::Float:
    OutputString("float", 5);
    break;
  case ezOpenDdlPrimitiveType::Double:
    OutputString("double", 6);
    break;
  case ezOpenDdlPrimitiveType::String:
    OutputString("string", 6);
    break;

  default:
    EZ_REPORT_FAILURE("Unknown DDL primitive type %u", type);
    break;
  }
}

void ezOpenDdlWriter::OutputPrimitiveTypeNameShortest(ezOpenDdlPrimitiveType type)
{
  // Change to OpenDDL: We write super short type strings

  switch (type)
  {
  case ezOpenDdlPrimitiveType::Bool:
    OutputString("b", 1);
    break;
  case ezOpenDdlPrimitiveType::Int8:
    OutputString("i1", 2);
    break;
  case ezOpenDdlPrimitiveType::Int16:
    OutputString("i2", 2);
    break;
  case ezOpenDdlPrimitiveType::Int32:
    OutputString("i3", 2);
    break;
  case ezOpenDdlPrimitiveType::Int64:
    OutputString("i4", 2);
    break;
  case ezOpenDdlPrimitiveType::UInt8:
    OutputString("u1", 2);
    break;
  case ezOpenDdlPrimitiveType::UInt16:
    OutputString("u2", 2);
    break;
  case ezOpenDdlPrimitiveType::UInt32:
    OutputString("u3", 2);
    break;
  case ezOpenDdlPrimitiveType::UInt64:
    OutputString("u4", 2);
    break;
  case ezOpenDdlPrimitiveType::Float:
    OutputString("f", 1);
    break;
  case ezOpenDdlPrimitiveType::Double:
    OutputString("d", 1);
    break;
  case ezOpenDdlPrimitiveType::String:
    OutputString("s", 1);
    break;

  default:
    EZ_REPORT_FAILURE("Unknown DDL primitive type %u", type);
    break;
  }
}

ezOpenDdlWriter::ezOpenDdlWriter()
{
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesBool == (int)ezOpenDdlPrimitiveType::Bool);
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesInt8 == (int)ezOpenDdlPrimitiveType::Int8);
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesInt16 == (int)ezOpenDdlPrimitiveType::Int16);
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesInt32 == (int)ezOpenDdlPrimitiveType::Int32);
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesInt64 == (int)ezOpenDdlPrimitiveType::Int64);
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesUInt8 == (int)ezOpenDdlPrimitiveType::UInt8);
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesUInt16 == (int)ezOpenDdlPrimitiveType::UInt16);
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesUInt32 == (int)ezOpenDdlPrimitiveType::UInt32);
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesUInt64 == (int)ezOpenDdlPrimitiveType::UInt64);
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesFloat == (int)ezOpenDdlPrimitiveType::Float);
  EZ_CHECK_AT_COMPILETIME((int)ezOpenDdlWriter::State::PrimitivesString == (int)ezOpenDdlPrimitiveType::String);

  m_bCompactMode = false;
  m_TypeStringMode = TypeStringMode::ShortenedUnsignedInt;
  m_FloatPrecisionMode = FloatPrecisionMode::Exact;
  m_iIndentation = 0;

  m_StateStack.ExpandAndGetRef().m_State = State::Invalid;
  m_StateStack.ExpandAndGetRef().m_State = State::Empty;
}

//All,              ///< All whitespace is output. This is the default, it should be used for files that are read by humans.
//LessIndentation,  ///< Saves some space by using less space for indentation
//NoIndentation,    ///< Saves even more space by dropping all indentation from the output. The result will be noticeably less readable.
//NewlinesOnly,     ///< All unnecessary whitespace, except for newlines, is not output.
//None,             ///< No whitespace, not even newlines, is output. This should be used when JSON is used for data exchange, but probably not read by humans.

void ezOpenDdlWriter::BeginObject(const char* szType, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/, bool bSingleLine /*= false*/)
{
  {
    const auto state = m_StateStack.PeekBack().m_State;
    EZ_IGNORE_UNUSED(state);
    EZ_ASSERT_DEBUG(state == State::Empty || state == State::ObjectMultiLine || state == State::ObjectStart, "DDL Writer is in a state where no further objects may be created");
  }

  OutputObjectBeginning();

  {
    const auto state = m_StateStack.PeekBack().m_State;
    EZ_IGNORE_UNUSED(state);
    EZ_ASSERT_DEBUG(state != State::ObjectSingleLine, "Cannot put an object into another single-line object");
    EZ_ASSERT_DEBUG(state != State::ObjectStart, "Object beginning should have been written");
  }

  OutputIndentation();
  OutputString(szType);

  OutputObjectName(szName, bGlobalName);

  if (bSingleLine)
  {
    m_StateStack.ExpandAndGetRef().m_State = State::ObjectSingleLine;
  }
  else
  {
    m_StateStack.ExpandAndGetRef().m_State = State::ObjectMultiLine;
  }

  m_StateStack.ExpandAndGetRef().m_State = State::ObjectStart;
}


void ezOpenDdlWriter::OutputObjectBeginning()
{
  if (m_StateStack.PeekBack().m_State != State::ObjectStart)
    return;

  m_StateStack.PopBack();

  const auto state = m_StateStack.PeekBack().m_State;

  if (state == State::ObjectSingleLine)
  {
    //if (m_bCompactMode)
      OutputString("{", 1); // more compact
    //else
      //OutputString(" { ", 3);
  }
  else if (state == State::ObjectMultiLine)
  {
    if (m_bCompactMode)
    {
      OutputString("{", 1);
    }
    else
    {
      OutputString("\n", 1);
      OutputIndentation();
      OutputString("{\n", 2);
    }
  }

  m_iIndentation++;
}

bool IsDdlIdentifierCharacter(ezUInt8 byte);

void ezOpenDdlWriter::OutputObjectName(const char* szName, bool bGlobalName)
{
  if (!ezStringUtils::IsNullOrEmpty(szName))
  {
    //EZ_ASSERT_DEBUG(ezStringUtils::FindSubString(szName, " ") == nullptr, "Spaces are not allowed in DDL object names: '%s'", szName);


    /// \test This code path is untested
    bool bEscape = false;
    for (const char* szNameCpy = szName; *szNameCpy != '\0'; ++szNameCpy)
    {
      if (!IsDdlIdentifierCharacter(*szNameCpy))
      {
        bEscape = true;
        break;
      }
    }

    if (m_bCompactMode)
    {
      // even remove the whitespace between type and name

      if (bGlobalName)
        OutputString("$", 1);
      else
        OutputString("%", 1);
    }
    else
    {
      if (bGlobalName)
        OutputString(" $", 2);
      else
        OutputString(" %", 2);
    }

    if (bEscape)
      OutputString("\'", 1);

    OutputString(szName);

    if (bEscape)
      OutputString("\'", 1);
  }
}

void ezOpenDdlWriter::EndObject()
{
  const auto state = m_StateStack.PeekBack().m_State;
  EZ_ASSERT_DEBUG(state == State::ObjectSingleLine || state == State::ObjectMultiLine || state == State::ObjectStart, "No object is open");

  if (state == State::ObjectStart)
  {
    // object is empty

    OutputString("{}\n", 3);
    m_StateStack.PopBack();

    const auto newState = m_StateStack.PeekBack().m_State;
    EZ_IGNORE_UNUSED(newState);
    EZ_ASSERT_DEBUG(newState == State::ObjectSingleLine || newState == State::ObjectMultiLine, "No object is open");
  }
  else
  {
    m_iIndentation--;

    if (m_bCompactMode)
      OutputString("}", 1);
    else
    {
      if (state == State::ObjectMultiLine)
      {
        OutputIndentation();
        OutputString("}\n", 2);
      }
      else
      {
        //OutputString(" }\n", 3);
        OutputString("}\n", 2); // more compact
      }
    }
  }

  m_StateStack.PopBack();
}

void ezOpenDdlWriter::BeginPrimitiveList(ezOpenDdlPrimitiveType type, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  OutputObjectBeginning();

  const auto state = m_StateStack.PeekBack().m_State;
  EZ_ASSERT_DEBUG(state == State::Empty || state == State::ObjectSingleLine || state == State::ObjectMultiLine, "DDL Writer is in a state where no primitive list may be created");

  if (state == State::ObjectMultiLine)
  {
    OutputIndentation();
  }

  if (m_TypeStringMode == TypeStringMode::Shortest)
    OutputPrimitiveTypeNameShortest(type);
  else if (m_TypeStringMode == TypeStringMode::ShortenedUnsignedInt)
    OutputPrimitiveTypeNameShort(type);
  else
    OutputPrimitiveTypeNameCompliant(type);

  OutputObjectName(szName, bGlobalName);

  // more compact
  //if (m_bCompactMode)
    OutputString("{", 1);
  //else
    //OutputString(" {", 2);

  m_StateStack.ExpandAndGetRef().m_State = static_cast<State>(type);

}

void ezOpenDdlWriter::EndPrimitiveList()
{
  const auto state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEBUG(state >= State::PrimitivesBool && state <= State::PrimitivesString, "No primitive list is open");

  m_StateStack.PopBack();

  if (m_bCompactMode)
    OutputString("}", 1);
  else
  {
    if (m_StateStack.PeekBack().m_State == State::ObjectSingleLine)
      OutputString("}", 1);
    else
      OutputString("}\n", 2);
  }
}

void ezOpenDdlWriter::WritePrimitiveType(ezOpenDdlWriter::State exp)
{
  auto& state = m_StateStack.PeekBack();
  EZ_ASSERT_DEBUG(state.m_State == exp, "Cannot write thie primitive type without have the correct primitive list open");

  if (state.m_bPrimitivesWritten)
  {
    // already wrote some primitives, so append a comma
    OutputString(",", 1);
  }

  state.m_bPrimitivesWritten = true;
}


void ezOpenDdlWriter::WriteBinaryAsHex(const void* pData, ezUInt32 uiBytes)
{
  char tmp[4];

  ezUInt8* pBytes = (ezUInt8*)pData;

  for (ezUInt32 i = 0; i < uiBytes; ++i)
  {
    ezStringUtils::snprintf(tmp, 4, "%02X", (ezUInt32)*pBytes);
    ++pBytes;

    OutputString(tmp, 2);
  }
}

void ezOpenDdlWriter::WriteBool(const bool* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesBool);

  if (m_bCompactMode || m_TypeStringMode == TypeStringMode::Shortest)
  {
    // Extension to OpenDDL: We write only '1' or '0' in compact mode

    if (pValues[0])
      OutputString("1", 1);
    else
      OutputString("0", 1);

    for (ezUInt32 i = 1; i < count; ++i)
    {
      if (pValues[i])
        OutputString(",1", 2);
      else
        OutputString(",0", 2);
    }
  }
  else
  {
    if (pValues[0])
      OutputString("true", 4);
    else
      OutputString("false", 5);

    for (ezUInt32 i = 1; i < count; ++i)
    {
      if (pValues[i])
        OutputString(",true", 5);
      else
        OutputString(",false", 6);
    }
  }
}

void ezOpenDdlWriter::WriteInt8(const ezInt8* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt8);

  m_Temp.Format("%i", pValues[0]);
  OutputString(m_Temp.GetData());

  for (ezUInt32 i = 1; i < count; ++i)
  {
    m_Temp.Format(",%i", pValues[i]);
    OutputString(m_Temp.GetData());
  }
}

void ezOpenDdlWriter::WriteInt16(const ezInt16* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt16);

  m_Temp.Format("%i", pValues[0]);
  OutputString(m_Temp.GetData());

  for (ezUInt32 i = 1; i < count; ++i)
  {
    m_Temp.Format(",%i", pValues[i]);
    OutputString(m_Temp.GetData());
  }
}

void ezOpenDdlWriter::WriteInt32(const ezInt32* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt32);

  m_Temp.Format("%i", pValues[0]);
  OutputString(m_Temp.GetData());

  for (ezUInt32 i = 1; i < count; ++i)
  {
    m_Temp.Format(",%i", pValues[i]);
    OutputString(m_Temp.GetData());
  }
}

void ezOpenDdlWriter::WriteInt64(const ezInt64* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt64);

  m_Temp.Format("%lli", pValues[0]);
  OutputString(m_Temp.GetData());

  for (ezUInt32 i = 1; i < count; ++i)
  {
    m_Temp.Format(",%lli", pValues[i]);
    OutputString(m_Temp.GetData());
  }
}


void ezOpenDdlWriter::WriteUInt8(const ezUInt8* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt8);

  m_Temp.Format("%u", pValues[0]);
  OutputString(m_Temp.GetData());

  for (ezUInt32 i = 1; i < count; ++i)
  {
    m_Temp.Format(",%u", pValues[i]);
    OutputString(m_Temp.GetData());
  }
}

void ezOpenDdlWriter::WriteUInt16(const ezUInt16* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt16);

  m_Temp.Format("%u", pValues[0]);
  OutputString(m_Temp.GetData());

  for (ezUInt32 i = 1; i < count; ++i)
  {
    m_Temp.Format(",%u", pValues[i]);
    OutputString(m_Temp.GetData());
  }
}

void ezOpenDdlWriter::WriteUInt32(const ezUInt32* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt32);

  m_Temp.Format("%u", pValues[0]);
  OutputString(m_Temp.GetData());

  for (ezUInt32 i = 1; i < count; ++i)
  {
    m_Temp.Format(",%u", pValues[i]);
    OutputString(m_Temp.GetData());
  }
}

void ezOpenDdlWriter::WriteUInt64(const ezUInt64* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt64);

  m_Temp.Format("%llu", pValues[0]);
  OutputString(m_Temp.GetData());

  for (ezUInt32 i = 1; i < count; ++i)
  {
    m_Temp.Format(",%llu", pValues[i]);
    OutputString(m_Temp.GetData());
  }
}

void ezOpenDdlWriter::WriteFloat(const float* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesFloat);

  if (m_FloatPrecisionMode == FloatPrecisionMode::Readable)
  {
    m_Temp.Format("%f", pValues[0]);
    OutputString(m_Temp.GetData());

    for (ezUInt32 i = 1; i < count; ++i)
    {
      m_Temp.Format(",%f", pValues[i]);
      OutputString(m_Temp.GetData());
    }
  }
  else
  {
    // zeros are so common that writing them in HEX blows up file size, so write them as decimals

    if (pValues[0] == 0)
    {
      OutputString("0", 1);
    }
    else
    {
      OutputString("0x", 2);
      WriteBinaryAsHex(&pValues[0], 4);
    }

    for (ezUInt32 i = 1; i < count; ++i)
    {
      if (pValues[i] == 0)
      {
        OutputString(",0", 2);
      }
      else
      {
        OutputString(",0x", 3);
        WriteBinaryAsHex(&pValues[i], 4);
      }
    }
  }
}

void ezOpenDdlWriter::WriteDouble(const double* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesDouble);

  if (m_FloatPrecisionMode == FloatPrecisionMode::Readable)
  {
    m_Temp.Format("%f", pValues[0]);
    OutputString(m_Temp.GetData());

    for (ezUInt32 i = 1; i < count; ++i)
    {
      m_Temp.Format(",%f", pValues[i]);
      OutputString(m_Temp.GetData());
    }
  }
  else
  {
    // zeros are so common that writing them in HEX blows up file size, so write them as decimals

    if (pValues[0] == 0)
    {
      OutputString("0", 1);
    }
    else
    {
      OutputString("0x", 2);
      WriteBinaryAsHex(&pValues[0], 8);
    }

    for (ezUInt32 i = 1; i < count; ++i)
    {
      if (pValues[i] == 0)
      {
        OutputString(",0", 2);
      }
      else
      {
        OutputString(",0x", 3);
        WriteBinaryAsHex(&pValues[i], 8);
      }
    }
  }
}

void ezOpenDdlWriter::WriteString(const ezStringView& string)
{
  WritePrimitiveType(State::PrimitivesString);

  OutputEscapedString(string);
}

void ezOpenDdlWriter::WriteBinaryAsString(const void* pData, ezUInt32 uiBytes)
{
  /// \test ezOpenDdlWriter::WriteBinaryAsString

  WritePrimitiveType(State::PrimitivesString);

  OutputString("\"", 1);
  WriteBinaryAsHex(pData, uiBytes);
  OutputString("\"", 1);
}


