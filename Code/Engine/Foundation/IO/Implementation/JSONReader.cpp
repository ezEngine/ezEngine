#include <Foundation/PCH.h>
#include <Foundation/IO/JSONReader.h>

ezResult ezJSONReader::Parse(ezStreamReaderBase& InputStream, ezUInt32 uiFirstLineOffset)
{
  m_bParsingError = false;
  m_Stack.Clear();
  m_sLastName.Clear();

  SetInputStream(InputStream, uiFirstLineOffset);

  while (!m_bParsingError && ContinueParsing())
  {
  }

  if (m_bParsingError)
  {
    m_Stack.Clear();
    m_Stack.PushBack(Element());

    return EZ_FAILURE;
  }

  // make sure there is one top level element
  if (m_Stack.IsEmpty())
    m_Stack.PushBack(Element());

  return EZ_SUCCESS;
}

bool ezJSONReader::OnVariable(const char* szVarName)
{
  m_sLastName = szVarName;

  return true;
}

void ezJSONReader::OnReadValue(const char* szValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(ezVariant(szValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = ezVariant(szValue);

  m_sLastName.Clear();
}

void ezJSONReader::OnReadValue(double fValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(ezVariant(fValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = ezVariant(fValue);

  m_sLastName.Clear();
}

void ezJSONReader::OnReadValue(bool bValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(ezVariant(bValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = ezVariant(bValue);

  m_sLastName.Clear();
}

void ezJSONReader::OnReadValueNULL()
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(ezVariant());
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = ezVariant();

  m_sLastName.Clear();
}

void ezJSONReader::OnBeginObject()
{
  m_Stack.PushBack(Element());
  m_Stack.PeekBack().m_Mode = ElementMode::Dictionary;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void ezJSONReader::OnEndObject()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];

  if (m_Stack.GetCount() > 1)
  {
    Element& Parent = m_Stack[m_Stack.GetCount() - 2];

    if (Parent.m_Mode == ElementMode::Array)
    {
      Parent.m_Array.PushBack(Child.m_Dictionary);
    }
    else
    {
      Parent.m_Dictionary[Child.m_sName] = Child.m_Dictionary;
    }

    m_Stack.PopBack();
  }
  else
  {
    // do nothing, keep the top-level dictionary
  }
}

void ezJSONReader::OnBeginArray()
{
  m_Stack.PushBack(Element());
  m_Stack.PeekBack().m_Mode = ElementMode::Array;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void ezJSONReader::OnEndArray()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];
  Element& Parent = m_Stack[m_Stack.GetCount() - 2];

  if (Parent.m_Mode == ElementMode::Array)
  {
    Parent.m_Array.PushBack(Child.m_Array);
  }
  else
  {
    Parent.m_Dictionary[Child.m_sName] = Child.m_Array;
  }

  m_Stack.PopBack();
}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_JSONReader);

