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
  if (m_WhitespaceMode >= WhitespaceMode::NoIndentation)
    return;

  ezInt32 iIndentation = m_iIndentation * 2;

  if (m_WhitespaceMode == WhitespaceMode::LessIndentation)
    iIndentation = m_iIndentation;

  // I need my space!
  const char* szIndentation = "                ";

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


void ezOpenDdlWriter::OutputPrimitiveTypeName(ezOpenDdlPrimitiveType type)
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
    //OutputString("unsigned_int8", 13);
    OutputString("uint8", 5);
    break;
  case ezOpenDdlPrimitiveType::UInt16:
    //OutputString("unsigned_int16", 14);
    OutputString("uint16", 6);
    break;
  case ezOpenDdlPrimitiveType::UInt32:
    //OutputString("unsigned_int32", 14);
    OutputString("uint32", 6);
    break;
  case ezOpenDdlPrimitiveType::UInt64:
    //OutputString("unsigned_int64", 14);
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

  m_WhitespaceMode = WhitespaceMode::All;
  m_iIndentation = 0;

  m_StateStack.ExpandAndGetRef().m_State = State::Invalid;
  m_StateStack.ExpandAndGetRef().m_State = State::Empty;
}

//All,              ///< All whitespace is output. This is the default, it should be used for files that are read by humans.
//LessIndentation,  ///< Saves some space by using less space for indentation
//NoIndentation,    ///< Saves even more space by dropping all indentation from the output. The result will be noticeably less readable.
//NewlinesOnly,     ///< All unnecessary whitespace, except for newlines, is not output.
//None,             ///< No whitespace, not even newlines, is output. This should be used when JSON is used for data exchange, but probably not read by humans.

void ezOpenDdlWriter::BeginObject(const char* szType, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  const auto state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEBUG(state == State::Empty || state == State::Object, "DDL Writer is in a state where no further objects may be created");

  OutputIndentation();
  OutputString(szType);

  OutputObjectName(szName, bGlobalName);

  if (m_WhitespaceMode != WhitespaceMode::None)
  {
    OutputString("\n", 1);
    OutputIndentation();
    OutputString("{\n", 2);
  }
  else
  {
    OutputString("{", 1);
  }

  m_iIndentation++;

  m_StateStack.ExpandAndGetRef().m_State = State::Object;
}


void ezOpenDdlWriter::OutputObjectName(const char* szName, bool bGlobalName)
{
  if (!ezStringUtils::IsNullOrEmpty(szName))
  {
    EZ_ASSERT_DEBUG(ezStringUtils::FindSubString(szName, " ") == nullptr, "Spaces are not allowed in DDL object names: '%s'", szName);

    if (m_WhitespaceMode == WhitespaceMode::None)
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

    OutputString(szName);
  }
}

void ezOpenDdlWriter::EndObject()
{
  const auto state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEBUG(state == State::Object, "No object is open");

  m_iIndentation--;

  if (m_WhitespaceMode == WhitespaceMode::None)
    OutputString("}", 1);
  else
  {
    OutputIndentation();
    OutputString("}\n", 2);
  }

  m_StateStack.PopBack();
}

void ezOpenDdlWriter::BeginPrimitiveList(ezOpenDdlPrimitiveType type, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  const auto state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEBUG(state == State::Empty || state == State::Object, "DDL Writer is in a state where no primitive list may be created");

  OutputIndentation();
  OutputPrimitiveTypeName(type);

  OutputObjectName(szName, bGlobalName);

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("{", 1);
  else
    OutputString(" { ", 3);

  m_StateStack.ExpandAndGetRef().m_State = static_cast<State>(type);

}

void ezOpenDdlWriter::EndPrimitiveList()
{
  const auto state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEBUG(state >= State::PrimitivesBool && state <= State::PrimitivesString, "No primitive list is open");

  if (m_WhitespaceMode == WhitespaceMode::None)
    OutputString("}", 1);
  else if (m_WhitespaceMode == WhitespaceMode::NewlinesOnly)
    OutputString("}\n", 2);
  else
    OutputString(" }\n", 3);

  m_StateStack.PopBack();
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

void ezOpenDdlWriter::WriteBool(const bool* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesBool);

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

  m_Temp.Format("%f", pValues[0]);
  OutputString(m_Temp.GetData());

  for (ezUInt32 i = 1; i < count; ++i)
  {
    m_Temp.Format(",%f", pValues[i]);
    OutputString(m_Temp.GetData());
  }
}

void ezOpenDdlWriter::WriteDouble(const double* pValues, ezUInt32 count /*= 1*/)
{
  EZ_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  EZ_ASSERT_DEBUG(count > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesDouble);

  m_Temp.Format("%f", pValues[0]);
  OutputString(m_Temp.GetData());

  for (ezUInt32 i = 1; i < count; ++i)
  {
    m_Temp.Format(",%f", pValues[i]);
    OutputString(m_Temp.GetData());
  }
}

void ezOpenDdlWriter::WriteString(const ezStringView& string)
{
  WritePrimitiveType(State::PrimitivesString);

  OutputEscapedString(string);
}


