#include <FoundationPCH.h>

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Memory/CommonAllocators.h>

const char* ezTokenType::EnumNames[ezTokenType::ENUM_COUNT] = {
  "Unknown",
  "Whitespace",
  "Identifier",
  "NonIdentifier",
  "Newline",
  "LineComment",
  "BlockComment",
  "String1",
  "String2",
  "Integer",
  "Float",
};

namespace
{
  // This allocator is used to get rid of some of the memory allocation tracking
  // that would otherwise occur for allocations made by the tokenizer.
  thread_local ezAllocator<ezMemoryPolicies::ezHeapAllocation, ezMemoryTrackingFlags::None> s_ClassAllocator("ezTokenizer", ezFoundation::GetDefaultAllocator());
}


ezTokenizer::ezTokenizer()
  : m_Tokens(&s_ClassAllocator)
  , m_Data(&s_ClassAllocator)
{
  m_pLog = nullptr;
  m_CurMode = ezTokenType::Unknown;
  m_uiCurLine = 1;
  m_uiCurColumn = -1;
  m_uiCurChar = '\0';
  m_uiNextChar = '\0';
  m_uiLastLine = 1;
  m_uiLastColumn = 1;

  m_szCurCharStart = nullptr;
  m_szNextCharStart = nullptr;
  m_szTokenStart = nullptr;
}

void ezTokenizer::NextChar()
{
  m_uiCurChar = m_uiNextChar;
  m_szCurCharStart = m_szNextCharStart;
  ++m_uiCurColumn;

  if (m_uiCurChar == '\n')
  {
    ++m_uiCurLine;
    m_uiCurColumn = 0;
  }

  if (!m_Iterator.IsValid())
  {
    m_szNextCharStart = m_Iterator.GetEndPointer();
    m_uiNextChar = '\0';
    return;
  }

  m_uiNextChar = m_Iterator.GetCharacter();
  m_szNextCharStart = m_Iterator.GetStartPointer();

  ++m_Iterator;
}

void ezTokenizer::AddToken()
{
  const char* szEnd = m_szCurCharStart;

  ezToken t;
  t.m_uiLine = m_uiLastLine;
  t.m_uiColumn = m_uiLastColumn;
  t.m_iType = m_CurMode;
  t.m_DataView = ezStringView(m_szTokenStart, szEnd);

  m_uiLastLine = m_uiCurLine;
  m_uiLastColumn = m_uiCurColumn;

  m_Tokens.PushBack(t);

  m_szTokenStart = szEnd;

  m_CurMode = ezTokenType::Unknown;
}

void ezTokenizer::Tokenize(ezArrayPtr<const ezUInt8> Data, ezLogInterface* pLog)
{
  if (Data.GetCount() >= 3)
  {
    const char* dataStart = reinterpret_cast<const char*>(Data.GetPtr());

    if (ezUnicodeUtils::SkipUtf8Bom(dataStart))
    {
      ezLog::Error(pLog, "Data to tokenize contains a Utf-8 BOM.");

      // although the tokenizer should get data without a BOM, it's easy enough to work around that here
      // that's what the tokenizer does in other error cases as well - complain, but continue
      Data = ezArrayPtr<const ezUInt8>((const ezUInt8*)dataStart, Data.GetCount() - 3);
    }
  }

  m_Data.Clear();
  m_Data.Reserve(m_Data.GetCount() + 1);
  m_Data = Data;

  if (m_Data.IsEmpty() || m_Data[m_Data.GetCount() - 1] != 0)
    m_Data.PushBack('\0'); // make sure the string is zero terminated

  m_Tokens.Clear();
  m_pLog = pLog;

  {
    m_CurMode = ezTokenType::Unknown;
    m_uiCurLine = 1;
    m_uiCurColumn = -1;
    m_uiCurChar = '\0';
    m_uiNextChar = '\0';
    m_uiLastLine = 1;
    m_uiLastColumn = 1;

    m_szCurCharStart = nullptr;
    m_szNextCharStart = nullptr;
    m_szTokenStart = nullptr;
  }

  m_Iterator = ezStringView((const char*)&m_Data[0], (const char*)&m_Data[0] + m_Data.GetCount() - 1);

  if (!m_Iterator.IsValid())
  {
    ezToken t;
    t.m_uiLine = 1;
    t.m_iType = ezTokenType::EndOfFile;
    m_Tokens.PushBack(t);
    return;
  }

  NextChar();
  NextChar();

  m_szTokenStart = m_szCurCharStart;

  while (m_szTokenStart != nullptr && *m_szTokenStart != '\0')
  {
    switch (m_CurMode)
    {
      case ezTokenType::Unknown:
        HandleUnknown();
        break;

      case ezTokenType::String1:
        HandleString('\"');
        break;

      case ezTokenType::String2:
        HandleString('\'');
        break;

      case ezTokenType::Integer:
      case ezTokenType::Float:
        HandleNumber();
        break;

      case ezTokenType::LineComment:
        HandleLineComment();
        break;

      case ezTokenType::BlockComment:
        HandleBlockComment();
        break;

      case ezTokenType::Whitespace:
        HandleWhitespace();
        break;

      case ezTokenType::Identifier:
        HandleIdentifier();
        break;

      case ezTokenType::NonIdentifier:
        HandleNonIdentifier();
        break;

      case ezTokenType::Newline:
      case ezTokenType::EndOfFile:
      case ezTokenType::ENUM_COUNT:
        break;
    }
  }

  ezToken t;
  t.m_uiLine = m_uiCurLine;
  t.m_iType = ezTokenType::EndOfFile;
  m_Tokens.PushBack(t);
}

void ezTokenizer::HandleUnknown()
{
  m_szTokenStart = m_szCurCharStart;

  if ((m_uiCurChar == '/') && (m_uiNextChar == '/'))
  {
    m_CurMode = ezTokenType::LineComment;
    NextChar();
    NextChar();
    return;
  }

  if ((m_uiCurChar == '/') && (m_uiNextChar == '*'))
  {
    m_CurMode = ezTokenType::BlockComment;
    NextChar();
    NextChar();
    return;
  }

  if (m_uiCurChar == '\"')
  {
    m_CurMode = ezTokenType::String1;
    NextChar();
    return;
  }

  if (m_uiCurChar == '\'')
  {
    m_CurMode = ezTokenType::String2;
    NextChar();
    return;
  }

  if ((m_uiCurChar == ' ') || (m_uiCurChar == '\t'))
  {
    m_CurMode = ezTokenType::Whitespace;
    NextChar();
    return;
  }

  if (ezStringUtils::IsDecimalDigit(m_uiCurChar) || (m_uiCurChar == '.' && ezStringUtils::IsDecimalDigit(m_uiNextChar)))
  {
    m_CurMode = m_uiCurChar == '.' ? ezTokenType::Float : ezTokenType::Integer;
    // Do not advance to next char here since we need the first character in HandleNumber
    return;
  }

  if (!ezStringUtils::IsIdentifierDelimiter_C_Code(m_uiCurChar))
  {
    m_CurMode = ezTokenType::Identifier;
    NextChar();
    return;
  }

  if (m_uiCurChar == '\n')
  {
    m_CurMode = ezTokenType::Newline;
    NextChar();
    AddToken();
    return;
  }

  if ((m_uiCurChar == '\r') && (m_uiNextChar == '\n'))
  {
    NextChar();
    NextChar();
    m_CurMode = ezTokenType::Newline;
    AddToken();
    return;
  }

  // else
  m_CurMode = ezTokenType::NonIdentifier;
  NextChar();
}

void ezTokenizer::HandleString(char terminator)
{
  while (m_uiCurChar != '\0')
  {
    // Escaped quote \"
    if ((m_uiCurChar == '\\') && (m_uiNextChar == terminator))
    {
      // skip this one
      NextChar();
      NextChar();
    }
    // escaped line break in string
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\n'))
    {
      AddToken();

      // skip this one entirely
      NextChar();
      NextChar();

      m_CurMode = terminator == '\"' ? ezTokenType::String1 : ezTokenType::String2;
      m_szTokenStart = m_szCurCharStart;
    }
    // escaped line break in string
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\r'))
    {
      // this might be a 3 character sequence of \\ \r \n -> skip them all
      AddToken();

      // skip \\ and \r
      NextChar();
      NextChar();

      // skip \n
      if (m_uiCurChar == '\n')
        NextChar();

      m_CurMode = terminator == '\"' ? ezTokenType::String1 : ezTokenType::String2;
      m_szTokenStart = m_szCurCharStart;
    }
    // not-escaped line break in string
    else if (m_uiCurChar == '\n')
    {
      ezLog::Error(m_pLog, "Unescaped Newline in string");
      //NextChar(); // not sure whether to include the newline in the string or not
      AddToken();
      return;
    }
    // end of string
    else if (m_uiCurChar == terminator)
    {
      NextChar();
      AddToken();
      return;
    }
    else
    {
      NextChar();
    }
  }

  ezLog::Error(m_pLog, "String not closed at end of file");
  AddToken();
}

void ezTokenizer::HandleNumber()
{
  if (m_uiCurChar == '0' && (m_uiNextChar == 'x' || m_uiNextChar == 'X'))
  {
    NextChar();
    NextChar();

    ezUInt32 uiDigitsRead = 0;
    while (ezStringUtils::IsHexDigit(m_uiCurChar))
    {
      NextChar();
      ++uiDigitsRead;
    }

    if (uiDigitsRead < 1)
    {
      ezLog::Error(m_pLog, "Invalid hex literal");
    }
  }
  else
  {
    NextChar();

    while (ezStringUtils::IsDecimalDigit(m_uiCurChar))
    {
      NextChar();
    }

    if (m_CurMode != ezTokenType::Float && (m_uiCurChar == '.' || m_uiCurChar == 'e' || m_uiCurChar == 'E'))
    {
      m_CurMode = ezTokenType::Float;
      bool bAllowExponent = true;

      if (m_uiCurChar == '.')
      {
        NextChar();

        ezUInt32 uiDigitsRead = 0;
        while (ezStringUtils::IsDecimalDigit(m_uiCurChar))
        {
          NextChar();
          ++uiDigitsRead;
        }

        bAllowExponent = uiDigitsRead > 0;
      }

      if ((m_uiCurChar == 'e' || m_uiCurChar == 'E') && bAllowExponent)
      {
        NextChar();
        if (m_uiCurChar == '+' || m_uiCurChar == '-')
        {
          NextChar();
        }

        ezUInt32 uiDigitsRead = 0;
        while (ezStringUtils::IsDecimalDigit(m_uiCurChar))
        {
          NextChar();
          ++uiDigitsRead;
        }

        if (uiDigitsRead < 1)
        {
          ezLog::Error(m_pLog, "Invalid float literal");
        }
      }

      if (m_uiCurChar == 'f') // skip float suffix
      {
        NextChar();
      }
    }
  }

  AddToken();
}

void ezTokenizer::HandleLineComment()
{
  while (m_uiCurChar != '\0')
  {
    if ((m_uiCurChar == '\r') || (m_uiCurChar == '\n'))
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // comment at end of file
  AddToken();
}

void ezTokenizer::HandleBlockComment()
{
  while (m_uiCurChar != '\0')
  {
    if ((m_uiCurChar == '*') && (m_uiNextChar == '/'))
    {
      NextChar();
      NextChar();
      AddToken();
      return;
    }

    NextChar();
  }

  ezLog::Error(m_pLog, "Block comment not closed at end of file.");
  AddToken();
}

void ezTokenizer::HandleWhitespace()
{
  while (m_uiCurChar != '\0')
  {
    if (m_uiCurChar != ' ' && m_uiCurChar != '\t')
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // whitespace at end of file
  AddToken();
}

void ezTokenizer::HandleIdentifier()
{
  while (m_uiCurChar != '\0')
  {
    if (ezStringUtils::IsIdentifierDelimiter_C_Code(m_uiCurChar))
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // identifier at end of file
  AddToken();
}

void ezTokenizer::HandleNonIdentifier()
{
  AddToken();
}

void ezTokenizer::GetAllLines(ezHybridArray<const ezToken*, 32>& Tokens) const
{
  Tokens.Clear();
  Tokens.Reserve(m_Tokens.GetCount());

  for (const ezToken& curToken : m_Tokens)
  {
    if (curToken.m_iType != ezTokenType::Newline)
    {
      Tokens.PushBack(&curToken);
    }
  }
}

ezResult ezTokenizer::GetNextLine(ezUInt32& uiFirstToken, ezHybridArray<ezToken*, 32>& Tokens)
{
  Tokens.Clear();

  ezHybridArray<const ezToken*, 32> Tokens0;
  ezResult r = GetNextLine(uiFirstToken, Tokens0);

  Tokens.SetCountUninitialized(Tokens0.GetCount());
  for (ezUInt32 i = 0; i < Tokens0.GetCount(); ++i)
    Tokens[i] = const_cast<ezToken*>(Tokens0[i]); // soo evil !

  return r;
}

ezResult ezTokenizer::GetNextLine(ezUInt32& uiFirstToken, ezHybridArray<const ezToken*, 32>& Tokens) const
{
  Tokens.Clear();

  const ezUInt32 uiMaxTokens = m_Tokens.GetCount() - 1;

  while (uiFirstToken < uiMaxTokens)
  {
    const ezToken& tCur = m_Tokens[uiFirstToken];

    // found a backslash
    if (tCur.m_iType == ezTokenType::NonIdentifier && tCur.m_DataView == "\\")
    {
      const ezToken& tNext = m_Tokens[uiFirstToken + 1];

      // and a newline!
      if (tNext.m_iType == ezTokenType::Newline)
      {
        /// \todo Theoretically, if the line ends with an identifier, and the next directly starts with one again,
        // we would need to merge the two into one identifier name, because the \ \n combo means it is not a
        // real line break
        // for now we ignore this and assume there is a 'whitespace' between such identifiers

        // we could maybe at least output a warning, if we detect it
        if (uiFirstToken > 0 && m_Tokens[uiFirstToken - 1].m_iType == ezTokenType::Identifier &&
            uiFirstToken + 2 < uiMaxTokens && m_Tokens[uiFirstToken + 2].m_iType == ezTokenType::Identifier)
        {
          ezStringBuilder s1 = m_Tokens[uiFirstToken - 1].m_DataView;
          ezStringBuilder s2 = m_Tokens[uiFirstToken + 2].m_DataView;
          ezLog::Warning("Line {0}: The \\ at the line end is in the middle of an identifier name ('{1}' and '{2}'). However, merging identifier names is currently not supported.", m_Tokens[uiFirstToken].m_uiLine, s1, s2);
        }

        // ignore this
        uiFirstToken += 2;
        continue;
      }
    }

    Tokens.PushBack(&tCur);

    if (m_Tokens[uiFirstToken].m_iType == ezTokenType::Newline)
    {
      ++uiFirstToken;
      return EZ_SUCCESS;
    }

    ++uiFirstToken;
  }

  if (Tokens.IsEmpty())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_Tokenizer);

