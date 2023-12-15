#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Enum.h>

// C-style strings
// No read equivalent for C-style strings (but can be read as ezString & ezStringBuilder instances)

ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const char* szValue)
{
  ezStringView szView(szValue);
  inout_stream.WriteString(szView).AssertSuccess();

  return inout_stream;
}

ezStreamWriter& operator<<(ezStreamWriter& inout_stream, ezStringView sValue)
{
  inout_stream.WriteString(sValue).AssertSuccess();

  return inout_stream;
}

// ezStringBuilder

ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezStringBuilder& sValue)
{
  inout_stream.WriteString(sValue.GetView()).AssertSuccess();
  return inout_stream;
}

ezStreamReader& operator>>(ezStreamReader& inout_stream, ezStringBuilder& out_sValue)
{
  inout_stream.ReadString(out_sValue).AssertSuccess();
  return inout_stream;
}


