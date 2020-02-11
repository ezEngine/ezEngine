#include <FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Strings/String.h>

ezStreamReader::ezStreamReader() = default;
ezStreamReader::~ezStreamReader() = default;

ezResult ezStreamReader::ReadString(ezStringBuilder& builder)
{
  auto context = ezStringDeduplicationReadContext::GetContext();

  if (!context)
  {
    ezUInt32 uiCount = 0;
    EZ_SUCCEED_OR_RETURN(ReadDWordValue(&uiCount));

    if (uiCount > 0)
    {
      // We access the string builder directly here to
      // read the string efficiently with one allocation
      builder.m_Data.Reserve(uiCount + 1);
      builder.m_Data.SetCountUninitialized(uiCount);
      ReadBytes(builder.m_Data.GetData(), uiCount);
      builder.m_uiCharacterCount = uiCount;
      builder.AppendTerminator();
    }
    else
    {
      builder.Clear();
    }
  }
  else
  {
    builder = context->DeserializeString(*this);
  }

  return EZ_SUCCESS;
}

ezResult ezStreamReader::ReadString(ezString& string)
{
  ezStringBuilder tmp;
  const ezResult res = ReadString(tmp);
  string = tmp;

  return res;
}

ezStreamWriter::ezStreamWriter() = default;

ezStreamWriter::~ezStreamWriter() = default;

ezResult ezStreamWriter::WriteString(const ezStringView szStringView)
{
  const ezUInt32 uiCount = szStringView.GetElementCount();

  auto context = ezStringDeduplicationWriteContext::GetContext();

  if (!context)
  {
    EZ_SUCCEED_OR_RETURN(WriteDWordValue(&uiCount));
    if (uiCount > 0)
    {
      EZ_SUCCEED_OR_RETURN(WriteBytes(szStringView.GetStartPointer(), uiCount));
    }
  }
  else
  {
    context->SerializeString(szStringView, *this);
  }

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_Stream);
