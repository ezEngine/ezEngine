
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>

namespace ezStreamUtils
{
  /// \brief Reads all the remaining data in \a stream and appends it to \a destination.
  EZ_FOUNDATION_DLL void ReadAllAndAppend(ezStreamReader& stream, ezDynamicArray<ezUInt8>& destination);

} // namespace ezStreamUtils
