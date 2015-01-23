#include <Foundation/PCH.h>
#include <Foundation/IO/JSONWriter.h>

ezStandardJSONWriter::JSONState::JSONState()
{
  m_State = Invalid;
  m_bRequireComma = false;
  m_bValueWasWritten = false;
}

ezStandardJSONWriter::CommaWriter::CommaWriter(ezStandardJSONWriter* pWriter)
{
  const ezStandardJSONWriter::State state = pWriter->m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV(state == ezStandardJSONWriter::Array      ||
            state == ezStandardJSONWriter::NamedArray ||
            state == ezStandardJSONWriter::Variable,  "Values can only be written inside BeginVariable() / EndVariable() and BeginArray() / EndArray().");

  m_pWriter = pWriter;

  if (m_pWriter->m_StateStack.PeekBack().m_bRequireComma)
  {
    // we are writing the comma now, so it is not required anymore
    m_pWriter->m_StateStack.PeekBack().m_bRequireComma = false;

    if (m_pWriter->m_StateStack.PeekBack().m_State == ezStandardJSONWriter::Array || 
        m_pWriter->m_StateStack.PeekBack().m_State == ezStandardJSONWriter::NamedArray)
    {
      if (pWriter->m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::NewlinesOnly)
        m_pWriter->OutputString(",");
      else
        m_pWriter->OutputString(", ");
    }
    else
    {
      if (pWriter->m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::None)
        m_pWriter->OutputString(",");
      else
        m_pWriter->OutputString(",\n");

      m_pWriter->OutputIndentation();
    }
  }
}

ezStandardJSONWriter::CommaWriter::~CommaWriter()
{
  m_pWriter->m_StateStack.PeekBack().m_bRequireComma = true;
  m_pWriter->m_StateStack.PeekBack().m_bValueWasWritten = true;
}

ezStandardJSONWriter::ezStandardJSONWriter()
{
  m_iIndentation = 0;
  m_pOutput = nullptr;
  JSONState s;
  s.m_State = ezStandardJSONWriter::Empty;
  m_StateStack.PushBack(s);
}

ezStandardJSONWriter::~ezStandardJSONWriter()
{
  EZ_ASSERT_DEV(m_StateStack.PeekBack().m_State == ezStandardJSONWriter::Empty, "The JSON stream must be closed properly.");
}

void ezStandardJSONWriter::SetOutputStream(ezStreamWriterBase* pOutput)
{
  m_pOutput = pOutput;
}

void ezStandardJSONWriter::OutputString(const char* sz)
{
  EZ_ASSERT_DEBUG(m_pOutput != nullptr, "No output stream has been set yet.");

  m_pOutput->WriteBytes(sz, ezStringUtils::GetStringElementCount(sz));
}

void ezStandardJSONWriter::OutputEscapedString(const char* sz)
{
  ezStringBuilder sEscaped = sz;
  sEscaped.ReplaceAll("\\", "\\\\");
  //sEscaped.ReplaceAll("/", "\\/"); // this is not necessary to escape
  sEscaped.ReplaceAll("\"", "\\\"");
  sEscaped.ReplaceAll("\b", "\\b");
  sEscaped.ReplaceAll("\r", "\\r");
  sEscaped.ReplaceAll("\f", "\\f");
  sEscaped.ReplaceAll("\n", "\\n");
  sEscaped.ReplaceAll("\t", "\\t");

  OutputString("\"");
  OutputString(sEscaped.GetData());
  OutputString("\"");
}

void ezStandardJSONWriter::OutputIndentation()
{
  if (m_WhitespaceMode >= WhitespaceMode::NoIndentation)
    return;

  ezInt32 iIndentation = m_iIndentation * 2;

  if (m_WhitespaceMode == WhitespaceMode::LessIndentation)
    iIndentation = m_iIndentation;

  ezStringBuilder s;
  s.Format("%*s", iIndentation, "");

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteBool(bool value)
{
  CommaWriter cw(this);

  if (value)
    OutputString("true");
  else
    OutputString("false");
}

void ezStandardJSONWriter::WriteInt32(ezInt32 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("%i", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteUInt32(ezUInt32 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("%u", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteInt64(ezInt64 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("%lli", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteUInt64(ezUInt64 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("%llu", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteFloat(float value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("%f", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteDouble(double value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.Format("%f", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteString(const char* value)
{
  CommaWriter cw(this);

  OutputEscapedString(value);
}

void ezStandardJSONWriter::WriteNULL()
{
  CommaWriter cw(this);

  OutputString("null");
}

void ezStandardJSONWriter::WriteTime(ezTime value)
{
  WriteDouble(value.GetSeconds());
}

void ezStandardJSONWriter::WriteColor(const ezColor& value)
{
  ezVec4 temp(value.r, value.g, value.b, value.a);

  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &temp, sizeof(temp) / sizeof(float));

  ezStringBuilder s;

  if (m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("(%.4f,%.4f,%.4f,%.4f)", value.r, value.g, value.b, value.a);
  else
    s.Format("(%.4f, %.4f, %.4f, %.4f)", value.r, value.g, value.b, value.a);

  WriteBinaryData("color", &temp, sizeof(temp), s.GetData());

}

void ezStandardJSONWriter::WriteVec2(const ezVec2& value)
{
  ezVec2 temp = value;

  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &temp, sizeof(temp) / sizeof(float));

  ezStringBuilder s;

  if (m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("(%.4f,%.4f)", value.x, value.y);
  else
    s.Format("(%.4f, %.4f)", value.x, value.y);

  WriteBinaryData("vec2", &temp, sizeof(temp), s.GetData());
}

void ezStandardJSONWriter::WriteVec3(const ezVec3& value)
{
  ezVec3 temp = value;

  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &temp, sizeof(temp) / sizeof(float));

  ezStringBuilder s;

  if (m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("(%.4f,%.4f,%.4f)", value.x, value.y, value.z);
  else
    s.Format("(%.4f, %.4f, %.4f)", value.x, value.y, value.z);

  WriteBinaryData("vec3", &temp, sizeof(temp), s.GetData());
}

void ezStandardJSONWriter::WriteVec4(const ezVec4& value)
{
  ezVec4 temp = value;

  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &temp, sizeof(temp) / sizeof(float));

  ezStringBuilder s;

  if (m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("(%.4f,%.4f,%.4f,%.4f)", value.x, value.y, value.z, value.w);
  else
    s.Format("(%.4f, %.4f, %.4f, %.4f)", value.x, value.y, value.z, value.w);

  WriteBinaryData("vec4", &temp, sizeof(temp), s.GetData());
}

void ezStandardJSONWriter::WriteQuat(const ezQuat& value)
{
  ezQuat temp = value;

  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &temp, sizeof(temp) / sizeof(float));

  WriteBinaryData("quat", &temp, sizeof(temp));
}

void ezStandardJSONWriter::WriteMat3(const ezMat3& value)
{
  ezMat3 temp = value;

  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &temp, sizeof(temp) / sizeof(float));

  WriteBinaryData("mat3", &temp, sizeof(temp));
}

void ezStandardJSONWriter::WriteMat4(const ezMat4& value)
{
  ezMat4 temp = value;

  ezEndianHelper::NativeToLittleEndian((ezUInt32*) &temp, sizeof(temp) / sizeof(float));

  WriteBinaryData("mat4", &temp, sizeof(temp));
}

void ezStandardJSONWriter::WriteUuid(const ezUuid& value)
{
  CommaWriter cw(this);

  ezUuid temp = value;

  ezEndianHelper::NativeToLittleEndian((ezUInt64*) &temp, sizeof(temp) / sizeof(ezUInt64));

  WriteBinaryData("uuid", &temp, sizeof(temp));
}

void ezStandardJSONWriter::BeginVariable(const char* szName)
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV(state == ezStandardJSONWriter::Empty ||
            state == ezStandardJSONWriter::Object ||
            state == ezStandardJSONWriter::NamedObject, "Variables can only be written inside objects.");

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::None)
      OutputString(",");
    else
      OutputString(",\n");

    OutputIndentation();
  }

  OutputEscapedString(szName);

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString(":");
  else
    OutputString(" : ");

  JSONState s;
  s.m_State = ezStandardJSONWriter::Variable;
  m_StateStack.PushBack(s);
}

void ezStandardJSONWriter::EndVariable()
{
  EZ_ASSERT_DEV(m_StateStack.PeekBack().m_State == ezStandardJSONWriter::Variable, "EndVariable() must be called in sync with BeginVariable().");
  EZ_ASSERT_DEV(m_StateStack.PeekBack().m_bValueWasWritten, "EndVariable() cannot be called without writing any value in between.");

  End();
}

void ezStandardJSONWriter::BeginArray(const char* szName)
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV((state == ezStandardJSONWriter::Empty) ||
            (state == ezStandardJSONWriter::Object || state == ezStandardJSONWriter::NamedObject) && !ezStringUtils::IsNullOrEmpty(szName) ||
            (state == ezStandardJSONWriter::Array  || state == ezStandardJSONWriter::NamedArray) && szName == nullptr ||
            (state == ezStandardJSONWriter::Variable && szName == nullptr),
            "Inside objects you can only begin arrays when also giving them a (non-empty) name.\n"
            "Inside arrays you can only nest anonymous arrays, so names are forbidden.\n"
            "Inside variables you cannot specify a name again.");

  if (szName != nullptr)
    BeginVariable(szName);

  m_StateStack.PeekBack().m_bValueWasWritten = true;

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString(",");
    else
      OutputString(", ");
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("[");
  else
    OutputString("[ ");

  JSONState s;
  s.m_State = (szName == nullptr) ? ezStandardJSONWriter::Array : ezStandardJSONWriter::NamedArray;
  m_StateStack.PushBack(s);
  ++m_iIndentation;
}

void ezStandardJSONWriter::EndArray()
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV(state == ezStandardJSONWriter::Array  || state == ezStandardJSONWriter::NamedArray, "EndArray() must be called in sync with BeginArray().");


  const State CurState = m_StateStack.PeekBack().m_State;

  End();

  if (CurState == ezStandardJSONWriter::NamedArray)
    EndVariable();
}

void ezStandardJSONWriter::BeginObject(const char* szName)
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV((state == ezStandardJSONWriter::Empty) ||
            (state == ezStandardJSONWriter::Object || state == ezStandardJSONWriter::NamedObject) && !ezStringUtils::IsNullOrEmpty(szName) ||
            (state == ezStandardJSONWriter::Array  || state == ezStandardJSONWriter::NamedArray) && szName == nullptr ||
            (state == ezStandardJSONWriter::Variable && szName == nullptr),
            "Inside objects you can only begin objects when also giving them a (non-empty) name.\n"
            "Inside arrays you can only nest anonymous objects, so names are forbidden.\n"
            "Inside variables you cannot specify a name again.");

  if (szName != nullptr)
    BeginVariable(szName);

  m_StateStack.PeekBack().m_bValueWasWritten = true;

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::None)
      OutputString(",");
    else
      OutputString(",\n");

    OutputIndentation();
  }

  if (m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::None)
    OutputString("{");
  else
    OutputString("{\n");

  JSONState s;
  s.m_State = (szName == nullptr) ? ezStandardJSONWriter::Object : ezStandardJSONWriter::NamedObject;
  m_StateStack.PushBack(s);
  ++m_iIndentation;

  OutputIndentation();
}

void ezStandardJSONWriter::EndObject()
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV(state == ezStandardJSONWriter::Object || state == ezStandardJSONWriter::NamedObject, "EndObject() must be called in sync with BeginObject().");

  const State CurState = m_StateStack.PeekBack().m_State;

  End();

  if (CurState == ezStandardJSONWriter::NamedObject)
    EndVariable();
}

void ezStandardJSONWriter::End()
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;

  if (m_StateStack.PeekBack().m_State == ezStandardJSONWriter::Array || 
      m_StateStack.PeekBack().m_State == ezStandardJSONWriter::NamedArray)
  {
    --m_iIndentation;

    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString("]");
    else
      OutputString(" ]");
  }

  
  m_StateStack.PopBack();
  m_StateStack.PeekBack().m_bRequireComma = true;

  if (state == ezStandardJSONWriter::Object || state == ezStandardJSONWriter::NamedObject)
  {
    --m_iIndentation;

    if (m_WhitespaceMode < ezJSONWriter::WhitespaceMode::None)
      OutputString("\n");

    OutputIndentation();
    OutputString("}");
  }
}


void ezStandardJSONWriter::WriteBinaryData(const char* szDataType, const void* pData, ezUInt32 uiBytes, const char* szValueString)
{
  CommaWriter cw(this);

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("{\"$t\":\"");
  else
    OutputString("{ \"$t\" : \"");

  OutputString(szDataType);

  if (szValueString != nullptr)
  {
    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString("\",\"$v\":\"");
    else
      OutputString("\", \"$v\" : \"");

    OutputString(szValueString);
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("\",\"$b\":\"0x");
  else
    OutputString("\", \"$b\" : \"0x");

  ezStringBuilder s;

  ezUInt8* pBytes = (ezUInt8*) pData;

  for (ezUInt32 i = 0; i < uiBytes; ++i)
  {
    s.Format("%02X", (ezUInt32) *pBytes);
    ++pBytes;

    OutputString(s.GetData());
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("\"}");
  else
    OutputString("\" }");
}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StandardJSONWriter);

