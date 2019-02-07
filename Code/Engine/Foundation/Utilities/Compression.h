#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>

///\brief The compression method to be used
enum class ezCompressionMethod : ezUInt16
{
  ZStd = 0 ///< Only available when ZStd support is enabled in the build (default)
};

/// \brief This namespace contains utilities which can be used to compress and decompress data.
namespace ezCompressionUtils
{
  ///\brief Compresses the given data using the compression method eMethod into the dynamic array given in out_Data.
  EZ_FOUNDATION_DLL ezResult Compress(ezArrayPtr<const ezUInt8> pUncompressedData, ezCompressionMethod eMethod, ezDynamicArray<ezUInt8>& out_Data);

  ///\brief Decompresses the given data using the compression method eMethod into the dynamic array given in out_Data.
  EZ_FOUNDATION_DLL ezResult Decompress(ezArrayPtr<const ezUInt8> pCompressedData, ezCompressionMethod eMethod, ezDynamicArray<ezUInt8>& out_Data);
}
