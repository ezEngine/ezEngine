#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Strings/String.h>

ezStreamReader::ezStreamReader() = default;
ezStreamReader::~ezStreamReader() = default;

ezResult ezStreamReader::ReadString(ezStringBuilder& ref_sBuilder)
{
  if (auto context = ezStringDeduplicationReadContext::GetContext())
  {
    ref_sBuilder = context->DeserializeString(*this);
  }
  else
  {
    ezUInt32 uiCount = 0;
    EZ_SUCCEED_OR_RETURN(ReadDWordValue(&uiCount));

    if (uiCount > 0)
    {
      // We access the string builder directly here to
      // read the string efficiently with one allocation
      ref_sBuilder.m_Data.Reserve(uiCount + 1);
      ref_sBuilder.m_Data.SetCountUninitialized(uiCount);
      ReadBytes(ref_sBuilder.m_Data.GetData(), uiCount);
      ref_sBuilder.AppendTerminator();
    }
    else
    {
      ref_sBuilder.Clear();
    }
  }

  return EZ_SUCCESS;
}

ezResult ezStreamReader::ReadString(ezString& ref_sString)
{
  ezStringBuilder tmp;
  const ezResult res = ReadString(tmp);
  ref_sString = tmp;

  return res;
}

ezStreamWriter::ezStreamWriter() = default;
ezStreamWriter::~ezStreamWriter() = default;

ezResult ezStreamWriter::WriteString(const ezStringView sStringView)
{
  const ezUInt32 uiCount = sStringView.GetElementCount();

  if (auto context = ezStringDeduplicationWriteContext::GetContext())
  {
    context->SerializeString(sStringView, *this);
  }
  else
  {
    EZ_SUCCEED_OR_RETURN(WriteDWordValue(&uiCount));
    if (uiCount > 0)
    {
      EZ_SUCCEED_OR_RETURN(WriteBytes(sStringView.GetStartPointer(), uiCount));
    }
  }

  return EZ_SUCCESS;
}


