#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONReader.h>


ezJSONReader::ezJSONReader()
{
  m_bParsingError = false;
}

ezResult ezJSONReader::Parse(ezStreamReader& ref_inputStream, ezUInt32 uiFirstLineOffset)
{
  m_bParsingError = false;
  m_Stack.Clear();
  m_sLastName.Clear();

  SetInputStream(ref_inputStream, uiFirstLineOffset);

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
  {
    Element& e = m_Stack.ExpandAndGetRef();
    e.m_Mode = ElementType::None;
  }

  return EZ_SUCCESS;
}

bool ezJSONReader::OnVariable(ezStringView sVarName)
{
  m_sLastName = sVarName;

  return true;
}

void ezJSONReader::OnReadValue(ezStringView sValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(std::move(ezString(sValue)));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = std::move(ezString(sValue));

  m_sLastName.Clear();
}

void ezJSONReader::OnReadValue(double fValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(ezVariant(fValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = ezVariant(fValue);

  m_sLastName.Clear();
}

void ezJSONReader::OnReadValue(bool bValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(ezVariant(bValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = ezVariant(bValue);

  m_sLastName.Clear();
}

void ezJSONReader::OnReadValueNULL()
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(ezVariant());
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = ezVariant();

  m_sLastName.Clear();
}

void ezJSONReader::OnBeginObject()
{
  m_Stack.PushBack(Element());
  m_Stack.PeekBack().m_Mode = ElementType::Dictionary;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void ezJSONReader::OnEndObject()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];

  if (m_Stack.GetCount() > 1)
  {
    Element& Parent = m_Stack[m_Stack.GetCount() - 2];

    if (Parent.m_Mode == ElementType::Array)
    {
      Parent.m_Array.PushBack(Child.m_Dictionary);
    }
    else
    {
      Parent.m_Dictionary[Child.m_sName] = std::move(Child.m_Dictionary);
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
  m_Stack.PeekBack().m_Mode = ElementType::Array;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void ezJSONReader::OnEndArray()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];

  if (m_Stack.GetCount() > 1)
  {
    Element& Parent = m_Stack[m_Stack.GetCount() - 2];

    if (Parent.m_Mode == ElementType::Array)
    {
      Parent.m_Array.PushBack(Child.m_Array);
    }
    else
    {
      Parent.m_Dictionary[Child.m_sName] = std::move(Child.m_Array);
    }

    m_Stack.PopBack();
  }
  else
  {
    // do nothing, keep the top-level array
  }
}

void ezJSONReader::OnParsingError(ezStringView sMessage, bool bFatal, ezUInt32 uiLine, ezUInt32 uiColumn)
{
  EZ_IGNORE_UNUSED(sMessage);
  EZ_IGNORE_UNUSED(bFatal);
  EZ_IGNORE_UNUSED(uiLine);
  EZ_IGNORE_UNUSED(uiColumn);

  m_bParsingError = true;
}
