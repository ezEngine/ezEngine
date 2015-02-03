#include <Foundation/PCH.h>
#include <Foundation/IO/JSONParser.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Foundation/Logging/Log.h>

ezJSONParser::ezJSONParser()
{
  m_uiCurByte = '\0';
  m_uiNextByte = '\0';
  m_pInput = nullptr;
  m_bSkippingMode = false;
  m_pLogInterface = nullptr;
}

void ezJSONParser::SetInputStream(ezStreamReaderBase& stream)
{
  m_StateStack.Clear();
  m_uiCurByte = '\0';
  m_TempString.Clear();
  m_bSkippingMode = false;

  m_pInput = &stream;

  m_uiNextByte = ' ';
  ReadCharacter(true);

  // go to the start of the document
  SkipWhitespace();

  // put the NotStarted state onto the stack
  {
    JSONState s;
    s.m_State = NotStarted;
    m_StateStack.PushBack(s);
  }
}

void ezJSONParser::StartParsing()
{
  // remove the NotStarted state
  m_StateStack.PopBack();

  // put the Finished state onto the stack
  {
    JSONState s;
    s.m_State = Finished;
    m_StateStack.PushBack(s);
  }

  switch (m_uiCurByte)
  {
  case '\0':
    // document is empty
    return;

  case '{':
    {
      JSONState s;
      s.m_State = ReadingObject;
      m_StateStack.PushBack(s);

      SkipWhitespace();

      if (!m_bSkippingMode)
        OnBeginObject();
    }
    return;

  default:
    {
      // document is malformed

      ezStringBuilder s;
      s.Format("Start of document: Expected a { or an empty document. Got '%c' instead.", m_uiCurByte);
      ParsingError(s.GetData(), true);
    }
    return;
  }
}

void ezJSONParser::ParseAll()
{
  while (ContinueParsing())
  {
  }
}

void ezJSONParser::ParsingError(const char* szMessage, bool bFatal)
{
  if (bFatal)
  {
    // prevent further error messages
    m_uiCurByte = '\0';
    m_StateStack.Clear();
  }

  if (bFatal)
    ezLog::Error(m_pLogInterface, szMessage);
  else
    ezLog::Warning(m_pLogInterface, szMessage);

  OnParsingError(szMessage, bFatal);
}

void ezJSONParser::SkipObject()
{
  SkipStack(ReadingObject);
}

void ezJSONParser::SkipArray()
{
  SkipStack(ReadingArray);
}

void ezJSONParser::SkipStack(State s)
{
  m_bSkippingMode = true;

  ezUInt32 iSkipToStackHeight = m_StateStack.GetCount();

  for (ezUInt32 top = m_StateStack.GetCount(); top > 1; --top)
  {
    if (m_StateStack[top - 1].m_State == s)
    {
      iSkipToStackHeight = top - 1;
      break;
    }
  }

  while (m_StateStack.GetCount() > iSkipToStackHeight)
    ContinueParsing();

  m_bSkippingMode = false;
}

bool ezJSONParser::ContinueParsing()
{
  if (m_uiCurByte == '\0')
  {
    // there's always the 'finished' state on the top of the stack when everything went fine
    if (m_StateStack.GetCount() > 1)
    {
      ParsingError("End of the document reached without closing all objects.", true);
    }

    return false;
  }

  switch (m_StateStack.PeekBack().m_State)
  {
  case NotStarted:
    StartParsing();
    break;

  case Finished:
    return false;

  case ReadingObject:
    ContinueObject();
    break;

  case ReadingArray:
    ContinueArray();
    break;

  case ReadingVariable:
    ContinueVariable();
    break;

  case ReadingValue:
    ContinueValue();
    break;

  case ExpectSeparator:
    ContinueSeparator();
    break;

  default:
    EZ_REPORT_FAILURE("Unknown State in JSON parser state machine.");
    break;
  }

  return true;
}

void ezJSONParser::ContinueObject()
{
  switch (m_uiCurByte)
  {
  case '\"':
    {
      JSONState s;
      s.m_State = ReadingVariable;
      m_StateStack.PushBack(s);
    }
    return;

  case '}':
    SkipWhitespace();

    m_StateStack.PopBack();

    if (!m_bSkippingMode)
      OnEndObject();
    return;

  case ',': // ignore superfluous commas
    SkipWhitespace();
    return;

  default:
    {
      ezStringBuilder s;
      s.Format("While parsing object: Expected \" to begin a new variable, or } to close the object. Got '%c' instead.", m_uiCurByte);
      ParsingError(s.GetData(), true);
    }
    return;
  }
}

void ezJSONParser::ContinueArray()
{
  switch (m_uiCurByte)
  {
  case ']':
    {
      SkipWhitespace();

      m_StateStack.PopBack();

      if (!m_bSkippingMode)
        OnEndArray();
    }
    return;

  default:
    {
      JSONState s;

      s.m_State = ExpectSeparator;
      m_StateStack.PushBack(s);

      s.m_State = ReadingValue;
      m_StateStack.PushBack(s);
    }
    return;
  }
}

void ezJSONParser::ContinueVariable()
{
  if (!m_bSkippingMode)
    ReadString();
  else
    SkipString();

  SkipWhitespace();

  if (m_uiCurByte != ':')
  {
    ezStringBuilder s;
    s.Format("After parsing variable name: Expected : to separate variable and value, Got '%c' instead.", m_uiCurByte);
    ParsingError(s.GetData(), false);
  }
  else
    SkipWhitespace();

  // remove ReadingVariable from the stack
  m_StateStack.PopBack();

  JSONState s;

  s.m_State = ExpectSeparator;
  m_StateStack.PushBack(s);

  s.m_State = ReadingValue;
  m_StateStack.PushBack(s);

  if (!m_bSkippingMode)
  {
    if (!OnVariable((const char*) &m_TempString[0]))
      SkipStack(ExpectSeparator);
  }
}


void ezJSONParser::ContinueValue()
{
  switch (m_uiCurByte)
  {
  case '\0':
    return;

  case '\"':
    {
      if (!m_bSkippingMode)
        ReadString();
      else
        SkipString();

      SkipWhitespace();

      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      if (!m_bSkippingMode)
        OnReadValue((const char*) &m_TempString[0]);
    }
    return;

  case '+':
  case '-':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case '.':
    {
      const double fValue = ReadNumber();

      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      if (!m_bSkippingMode)
        OnReadValue(fValue);
    }
    return;

  case 't':
  case 'f':
    {
      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      ReadWord();

      bool bRes = false;
      if (ezConversionUtils::StringToBool((const char*) &m_TempString[0], bRes) == EZ_FAILURE)
      {
        ezStringBuilder s;
        s.Format("Parsing value: Expected 'true' or 'false', Got '%s' instead.", (const char*) &m_TempString[0]);
        ParsingError(s.GetData(), false);
      }

      if (!m_bSkippingMode)
        OnReadValue(bRes);
    }
    return;

  case 'n':
  case 'N':
    {
      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      ReadWord();

      bool bIsNull = false;

      // if it's 'null' but with the wrong casing, output an error, but it is not fatal
      if (ezStringUtils::IsEqual_NoCase((const char*) &m_TempString[0], "null"))
        bIsNull = true;

      if (!ezStringUtils::IsEqual((const char*) &m_TempString[0], "null"))
      {
        ezStringBuilder s;
        s.Format("Parsing value: Expected 'null', Got '%s' instead.", (const char*) &m_TempString[0]);
        ParsingError(s.GetData(), !bIsNull);
      }

      if (!m_bSkippingMode)
        OnReadValueNULL();
    }
    return;

  case '[':
    {
      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      JSONState s;
      s.m_State = ReadingArray;
      m_StateStack.PushBack(s);

      SkipWhitespace();

      if (!m_bSkippingMode)
        OnBeginArray();
    }
    return;

  case '{':
    {
      // remove ReadingValue from the stack
      m_StateStack.PopBack();

      JSONState s;
      s.m_State = ReadingObject;
      m_StateStack.PushBack(s);

      SkipWhitespace();

      if (!m_bSkippingMode)
        OnBeginObject();
    }
    return;

  default:
    {
      ezStringBuilder s;
      s.Format("Parsing value: Expected [, {, f, t, \", 0-1, ., +, -, or even 'e'. Got '%c' instead", m_uiCurByte);
      ParsingError(s.GetData(), true);
    }
    return;
  }

}

void ezJSONParser::ContinueSeparator()
{
  if (ezStringUtils::IsWhiteSpace(m_uiCurByte))
    SkipWhitespace();

  // remove ExpectSeparator from the stack
  m_StateStack.PopBack();

  switch (m_uiCurByte)
  {
  case '\0':
    return;

  case ',':
    SkipWhitespace();
    return;

  case ']':
  case '}':
    return;

  default:
    {
      ezStringBuilder s;
      s.Format("After parsing value: Expected a comma or closing brackets/braces (], }). Got '%c' instead.", m_uiCurByte);
      ParsingError(s.GetData(), true);
    }
    return;
  }
}

bool ezJSONParser::ReadCharacter(bool bSkipComments)
{
  m_uiCurByte = m_uiNextByte;

  m_uiNextByte = '\0';
  m_pInput->ReadBytes(&m_uiNextByte, sizeof(ezUInt8));

  // skip comments
  if (m_uiCurByte == '/' && bSkipComments)
  {
    // line comment, read till line break
    if (m_uiNextByte == '/')
    {
      while (m_uiNextByte != '\0' && m_uiNextByte != '\n')
      {
        m_uiNextByte = '\0';
        m_pInput->ReadBytes(&m_uiNextByte, sizeof(ezUInt8));
      }

      ReadCharacter(true);
    }
    else if (m_uiNextByte == '*') // block comment, read till */
    {
      m_uiNextByte = ' ';

      while (m_uiNextByte != '\0' && (m_uiCurByte != '*' || m_uiNextByte != '/'))
      {
        m_uiCurByte = m_uiNextByte;

        m_uiNextByte = '\0';
        m_pInput->ReadBytes(&m_uiNextByte, sizeof(ezUInt8));
      }

      // replace the current end-comment by whitespace
      m_uiCurByte = ' ';
      m_uiNextByte = ' ';

      ReadCharacter(true);
      ReadCharacter(true); // might trigger another comment skipping
    }
  }

  return m_uiCurByte != '\0';
}

void ezJSONParser::SkipWhitespace()
{
  EZ_ASSERT_DEBUG(m_pInput != nullptr, "Input Stream is not set up.");

  do
  {
    m_uiCurByte = '\0';

    if (!ReadCharacter(true))
      return; // stop when end of stream is encountered
  }
  while (ezStringUtils::IsWhiteSpace(m_uiCurByte));
}

void ezJSONParser::SkipString()
{
  EZ_ASSERT_DEBUG(m_pInput != nullptr, "Input Stream is not set up.");

  m_TempString.Clear();
  m_TempString.PushBack('\0');

  bool bEscapeSequence = false;

  do
  {
    bEscapeSequence = (m_uiCurByte == '\\');

    m_uiCurByte = '\0';

    if (!ReadCharacter(false))
    {
      ParsingError("While skipping string: Reached end of document before end of string was found.", true);

      return; // stop when end of stream is encountered
    }
  }
  while (bEscapeSequence || m_uiCurByte != '\"');

}

void ezJSONParser::ReadString()
{
  EZ_ASSERT_DEBUG(m_pInput != nullptr, "Input Stream is not set up.");

  m_TempString.Clear();

  bool bEscapeSequence = false;

  while (true)
  {
    bEscapeSequence = (m_uiCurByte == '\\');

    m_uiCurByte = '\0';

    if (!ReadCharacter(false))
    {
      ParsingError("While reading string: Reached end of document before end of string was found.", true);

      break; // stop when end of stream is encountered
    }

    if (!bEscapeSequence && m_uiCurByte == '\"')
      break;

    if (bEscapeSequence)
    {
      switch (m_uiCurByte)
      {
      case '\"':
        m_TempString.PushBack('\"');
        break;
      case '\\':
        m_TempString.PushBack('\\');
        m_uiCurByte = '\0'; // make sure the next character isn't interpreted as an escape sequence
        break;
      case '/':
        m_TempString.PushBack('/');
        break;
      case 'b':
        m_TempString.PushBack('\b');
        break;
      case 'f':
        m_TempString.PushBack('\f');
        break;
      case 'n':
        m_TempString.PushBack('\n');
        break;
      case 'r':
        m_TempString.PushBack('\r');
        break;
      case 't':
        m_TempString.PushBack('\t');
        break;
      case 'u':
        ParsingError("Unicode literals are not supported.", false);
        /// \todo Support escaped Unicode literals? (\u1234)
        break;
      default:
        {
          ezStringBuilder s;
          s.Format("Unknown escape-sequence '\\%c'", m_uiCurByte);
          ParsingError(s, false);
        }
        break;
      }
    }
    else if (m_uiCurByte != '\\')
    {
      m_TempString.PushBack(m_uiCurByte);
    }
  }

  m_TempString.PushBack('\0');
}

void ezJSONParser::ReadWord()
{
  EZ_ASSERT_DEBUG(m_pInput != nullptr, "Input Stream is not set up.");

  m_TempString.Clear();

  do
  {
    m_TempString.PushBack(m_uiCurByte);

    m_uiCurByte = '\0';

    if (!ReadCharacter(true))
      break; // stop when end of stream is encountered
  }
  while (!ezStringUtils::IsWhiteSpace(m_uiCurByte) && m_uiCurByte != ',' && m_uiCurByte != ']' && m_uiCurByte != '}');

  m_TempString.PushBack('\0');
}

double ezJSONParser::ReadNumber()
{
  EZ_ASSERT_DEBUG(m_pInput != nullptr, "Input Stream is not set up.");

  m_TempString.Clear();

  do
  {
    m_TempString.PushBack(m_uiCurByte);

    m_uiCurByte = '\0';

    if (!ReadCharacter(true))
      break; // stop when end of stream is encountered
  }
  while ((m_uiCurByte >= '0' && m_uiCurByte <= '9') || m_uiCurByte == '.' || m_uiCurByte == 'e' || m_uiCurByte == 'E' || m_uiCurByte == '-' || m_uiCurByte == '+');

  m_TempString.PushBack('\0');

  double fResult = 0;
  if (ezConversionUtils::StringToFloat((const char*) &m_TempString[0], fResult) == EZ_FAILURE)
  {
    ezStringBuilder s;
    s.Format("Reading number failed: Could not convert '%s' to a floating point value.", (const char*) &m_TempString[0]);
    ParsingError(s.GetData(), true);
  }

  return fResult;
}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_JSONParser);

