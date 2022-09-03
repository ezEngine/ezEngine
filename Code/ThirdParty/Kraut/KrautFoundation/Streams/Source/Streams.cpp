#include "../Streams.h"

namespace AE_NS_FOUNDATION
{
  void aeStreamIn::ReadLine(aeString& out_sString)
  {
    out_sString.clear();

    char c;
    while (!IsEndOfStream())
    {
      if (Read(&c, sizeof(char)) == 0)
        return;

      if ((c == '\n') || (c == '\0'))
        return;

      if (c != '\r')
        out_sString += c;
    }
  }
} // namespace AE_NS_FOUNDATION
