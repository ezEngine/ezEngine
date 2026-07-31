#include <Foundation/FoundationPCH.h>

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
  EZ_ASSERT_DEV(state == ezStandardJSONWriter::Array || state == ezStandardJSONWriter::NamedArray || state == ezStandardJSONWriter::Variable,
    "Values can only be written inside BeginVariable() / EndVariable() and BeginArray() / EndArray().");

  m_pWriter = pWriter;

  if (m_pWriter->m_StateStack.PeekBack().m_bRequireComma)
  {
    // we are writing the comma now, so it is not required anymore
    m_pWriter->m_StateStack.PeekBack().m_bRequireComma = false;

    if (m_pWriter->m_StateStack.PeekBack().m_State == ezStandardJSONWriter::Array ||
        m_pWriter->m_StateStack.PeekBack().m_State == ezStandardJSONWriter::NamedArray)
    {
      if (pWriter->m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::NewlinesOnly)
      {
        if (pWriter->m_ArrayMode == ezJSONWriter::ArrayMode::InOneLine)
          m_pWriter->OutputString(",");
        else
          m_pWriter->OutputString(",\n");
      }
      else
      {
        if (pWriter->m_ArrayMode == ezJSONWriter::ArrayMode::InOneLine)
          m_pWriter->OutputString(", ");
        else
        {
          m_pWriter->OutputString(",\n");
          m_pWriter->OutputIndentation();
        }
      }
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
  if (!HadWriteError())
  {
    EZ_ASSERT_DEV(m_StateStack.PeekBack().m_State == ezStandardJSONWriter::Empty, "The JSON stream must be closed properly.");
  }
}

void ezStandardJSONWriter::SetOutputStream(ezStreamWriter* pOutput)
{
  m_pOutput = pOutput;
}

void ezStandardJSONWriter::OutputString(ezStringView s)
{
  EZ_ASSERT_DEBUG(m_pOutput != nullptr, "No output stream has been set yet.");

  if (m_pOutput->WriteBytes(s.GetStartPointer(), s.GetElementCount()).Failed())
  {
    SetWriteErrorState();
  }
}

void ezStandardJSONWriter::OutputEscapedString(ezStringView s)
{
  ezStringBuilder sEscaped = s;
  sEscaped.ReplaceAll("\\", "\\\\");
  // sEscaped.ReplaceAll("/", "\\/"); // this is not necessary to escape
  sEscaped.ReplaceAll("\"", "\\\"");
  sEscaped.ReplaceAll("\b", "\\b");
  sEscaped.ReplaceAll("\r", "\\r");
  sEscaped.ReplaceAll("\f", "\\f");
  sEscaped.ReplaceAll("\n", "\\n");
  sEscaped.ReplaceAll("\t", "\\t");

  OutputString("\"");
  OutputString(sEscaped);
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
  s.SetPrintf("%*s", iIndentation, "");

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
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteUInt32(ezUInt32 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteInt64(ezInt64 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteUInt64(ezUInt64 value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteFloat(float value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteDouble(double value)
{
  CommaWriter cw(this);

  ezStringBuilder s;
  s.SetFormat("{0}", value);

  OutputString(s.GetData());
}

void ezStandardJSONWriter::WriteString(ezStringView value)
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

void ezStandardJSONWriter::WriteFloatComponents(const float* pValues, ezUInt32 uiCount, ezStringView sComponentNames)
{
  EZ_ASSERT_DEBUG(uiCount <= sComponentNames.GetElementCount(), "Not enough component names for {} components.", uiCount);

  BeginObject();

  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    const char* szName = sComponentNames.GetStartPointer() + i;
    AddVariableFloat(ezStringView(szName, szName + 1), pValues[i]);
  }

  EndObject();
}

void ezStandardJSONWriter::WriteColor(const ezColor& value)
{
  // The linear/gamma distinction is not part of the output, so a reader has to know which one it is
  // getting from the context - same as for the component types below, which do not record their type
  // either.
  WriteFloatComponents(&value.r, 4, "rgba");
}

void ezStandardJSONWriter::WriteColorGamma(const ezColorGammaUB& value)
{
  BeginObject();
  AddVariableUInt32("r", value.r);
  AddVariableUInt32("g", value.g);
  AddVariableUInt32("b", value.b);
  AddVariableUInt32("a", value.a);
  EndObject();
}

void ezStandardJSONWriter::WriteVec2(const ezVec2& value)
{
  WriteFloatComponents(&value.x, 2, "xyzw");
}

void ezStandardJSONWriter::WriteVec3(const ezVec3& value)
{
  WriteFloatComponents(&value.x, 3, "xyzw");
}

void ezStandardJSONWriter::WriteVec4(const ezVec4& value)
{
  WriteFloatComponents(&value.x, 4, "xyzw");
}

void ezStandardJSONWriter::WriteVec2I32(const ezVec2I32& value)
{
  BeginObject();
  AddVariableInt32("x", value.x);
  AddVariableInt32("y", value.y);
  EndObject();
}

void ezStandardJSONWriter::WriteVec3I32(const ezVec3I32& value)
{
  BeginObject();
  AddVariableInt32("x", value.x);
  AddVariableInt32("y", value.y);
  AddVariableInt32("z", value.z);
  EndObject();
}

void ezStandardJSONWriter::WriteVec4I32(const ezVec4I32& value)
{
  BeginObject();
  AddVariableInt32("x", value.x);
  AddVariableInt32("y", value.y);
  AddVariableInt32("z", value.z);
  AddVariableInt32("w", value.w);
  EndObject();
}

void ezStandardJSONWriter::WriteQuat(const ezQuat& value)
{
  WriteFloatComponents(&value.x, 4, "xyzw");
}

void ezStandardJSONWriter::WriteMatrix(const float* pValues, ezUInt32 uiRowsAndColumns)
{
  BeginArray();

  for (ezUInt32 uiRow = 0; uiRow < uiRowsAndColumns; ++uiRow)
  {
    BeginArray();

    for (ezUInt32 uiColumn = 0; uiColumn < uiRowsAndColumns; ++uiColumn)
    {
      WriteFloat(pValues[uiRow * uiRowsAndColumns + uiColumn]);
    }

    EndArray();
  }

  EndArray();
}

void ezStandardJSONWriter::WriteMat3(const ezMat3& value)
{
  float f[9];
  value.GetAsArray(f, ezMatrixLayout::RowMajor);

  WriteMatrix(f, 3);
}

void ezStandardJSONWriter::WriteMat4(const ezMat4& value)
{
  float f[16];
  value.GetAsArray(f, ezMatrixLayout::RowMajor);

  WriteMatrix(f, 4);
}

void ezStandardJSONWriter::WriteUuid(const ezUuid& value)
{
  ezStringBuilder s;
  ezConversionUtils::ToString(value, s);

  WriteString(s);
}

void ezStandardJSONWriter::WriteAngle(ezAngle value)
{
  WriteFloat(value.GetDegree());
}

void ezStandardJSONWriter::WriteDataBuffer(const ezDataBuffer& value)
{
  // Hex, because Foundation has no base64 encoder. This doubles the size of the data, so consider
  // writing a reference to the data instead of the data itself.
  ezStringBuilder sHex;

  for (ezUInt8 uiByte : value)
  {
    sHex.AppendFormat("{}", ezArgU(uiByte, 2, true, 16));
  }

  WriteString(sHex);
}

void ezStandardJSONWriter::BeginVariable(ezStringView sName)
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV(state == ezStandardJSONWriter::Empty || state == ezStandardJSONWriter::Object || state == ezStandardJSONWriter::NamedObject,
    "Variables can only be written inside objects.");

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= ezJSONWriter::WhitespaceMode::None)
      OutputString(",");
    else
      OutputString(",\n");

    OutputIndentation();
  }

  OutputEscapedString(sName);

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

void ezStandardJSONWriter::BeginArray(ezStringView sName)
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV((state == ezStandardJSONWriter::Empty) ||
                  ((state == ezStandardJSONWriter::Object || state == ezStandardJSONWriter::NamedObject) && !sName.IsEmpty()) ||
                  ((state == ezStandardJSONWriter::Array || state == ezStandardJSONWriter::NamedArray) && sName.IsEmpty()) ||
                  (state == ezStandardJSONWriter::Variable && sName == nullptr),
    "Inside objects you can only begin arrays when also giving them a (non-empty) name.\n"
    "Inside arrays you can only nest anonymous arrays, so names are forbidden.\n"
    "Inside variables you cannot specify a name again.");

  if (sName != nullptr)
    BeginVariable(sName);

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
  s.m_State = (sName == nullptr) ? ezStandardJSONWriter::Array : ezStandardJSONWriter::NamedArray;
  m_StateStack.PushBack(s);
  ++m_iIndentation;
}

void ezStandardJSONWriter::EndArray()
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV(
    state == ezStandardJSONWriter::Array || state == ezStandardJSONWriter::NamedArray, "EndArray() must be called in sync with BeginArray().");


  const State CurState = m_StateStack.PeekBack().m_State;

  End();

  if (CurState == ezStandardJSONWriter::NamedArray)
    EndVariable();
}

void ezStandardJSONWriter::BeginObject(ezStringView sName)
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV((state == ezStandardJSONWriter::Empty) ||
                  ((state == ezStandardJSONWriter::Object || state == ezStandardJSONWriter::NamedObject) && !sName.IsEmpty()) ||
                  ((state == ezStandardJSONWriter::Array || state == ezStandardJSONWriter::NamedArray) && sName.IsEmpty()) ||
                  (state == ezStandardJSONWriter::Variable && sName == nullptr),
    "Inside objects you can only begin objects when also giving them a (non-empty) name.\n"
    "Inside arrays you can only nest anonymous objects, so names are forbidden.\n"
    "Inside variables you cannot specify a name again.");

  if (sName != nullptr)
    BeginVariable(sName);

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
  s.m_State = (sName == nullptr) ? ezStandardJSONWriter::Object : ezStandardJSONWriter::NamedObject;
  m_StateStack.PushBack(s);
  ++m_iIndentation;

  OutputIndentation();
}

void ezStandardJSONWriter::WriteRawJson(ezStringView sJson)
{
  // places the separating comma and marks the value as written, exactly as for any other value
  CommaWriter cw(this);

  // not OutputEscapedString(), because the text is JSON already and must not be quoted or escaped
  OutputString(sJson);
}

void ezStandardJSONWriter::AddVariableRawJson(ezStringView sName, ezStringView sJson)
{
  BeginVariable(sName);
  WriteRawJson(sJson);
  EndVariable();
}

void ezStandardJSONWriter::EndAll()
{
  // Works top down through whatever is open, using the public functions so that commas, indentation
  // and the NamedObject/NamedArray cascade into EndVariable() all behave as during normal writing.
  while (m_StateStack.GetCount() > 1)
  {
    switch (m_StateStack.PeekBack().m_State)
    {
      case ezStandardJSONWriter::Variable:
        // EndVariable() asserts unless something was written for it, and an object member without a
        // value is not representable, so the unfinished variable becomes null.
        if (!m_StateStack.PeekBack().m_bValueWasWritten)
          WriteNULL();

        EndVariable();
        break;

      case ezStandardJSONWriter::Object:
      case ezStandardJSONWriter::NamedObject:
        EndObject();
        break;

      case ezStandardJSONWriter::Array:
      case ezStandardJSONWriter::NamedArray:
        EndArray();
        break;

      default:
        // Nothing else can be on the stack above the initial Empty state. Bailing out rather than
        // looping forever, since this runs on an error path where an assert would be unhelpful.
        return;
    }
  }
}

void ezStandardJSONWriter::EndObject()
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  EZ_IGNORE_UNUSED(state);
  EZ_ASSERT_DEV(
    state == ezStandardJSONWriter::Object || state == ezStandardJSONWriter::NamedObject, "EndObject() must be called in sync with BeginObject().");

  const State CurState = m_StateStack.PeekBack().m_State;

  End();

  if (CurState == ezStandardJSONWriter::NamedObject)
    EndVariable();
}

void ezStandardJSONWriter::End()
{
  const ezStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;

  if (m_StateStack.PeekBack().m_State == ezStandardJSONWriter::Array || m_StateStack.PeekBack().m_State == ezStandardJSONWriter::NamedArray)
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


void ezStandardJSONWriter::WriteBinaryData(ezStringView sDataType, const void* pData, ezUInt32 uiBytes, ezStringView sValueString)
{
  CommaWriter cw(this);

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("{\"$t\":\"");
  else
    OutputString("{ \"$t\" : \"");

  OutputString(sDataType);

  if (!sValueString.IsEmpty())
  {
    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString("\",\"$v\":\"");
    else
      OutputString("\", \"$v\" : \"");

    OutputString(sValueString);
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("\",\"$b\":\"0x");
  else
    OutputString("\", \"$b\" : \"0x");

  ezStringBuilder s;

  ezUInt8* pBytes = (ezUInt8*)pData;

  for (ezUInt32 i = 0; i < uiBytes; ++i)
  {
    s.SetFormat("{0}", ezArgU((ezUInt32)*pBytes, 2, true, 16, true));
    ++pBytes;

    OutputString(s.GetData());
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("\"}");
  else
    OutputString("\" }");
}
