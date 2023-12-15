#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/StreamUtils.h>

void ezStreamUtils::ReadAllAndAppend(ezStreamReader& inout_stream, ezDynamicArray<ezUInt8>& ref_destination)
{
  ezUInt8 temp[1024 * 4];

  while (true)
  {
    const ezUInt32 uiRead = (ezUInt32)inout_stream.ReadBytes(temp, EZ_ARRAY_SIZE(temp));

    if (uiRead == 0)
      return;

    ref_destination.PushBackRange(ezArrayPtr<ezUInt8>(temp, uiRead));
  }
}


