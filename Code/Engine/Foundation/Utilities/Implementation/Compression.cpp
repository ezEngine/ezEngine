#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/Compression.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  define ZSTD_STATIC_LINKING_ONLY // ZSTD_findDecompressedSize
#  include <zstd/zstd.h>
#endif

namespace ezCompressionUtils
{
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  static ezResult CompressZStd(ezArrayPtr<const ezUInt8> uncompressedData, ezDynamicArray<ezUInt8>& out_data)
  {
    size_t uiSizeBound = ZSTD_compressBound(uncompressedData.GetCount());
    if (uiSizeBound > ezMath::MaxValue<ezUInt32>())
    {
      ezLog::Error("Can't compress since the output container can't hold enough elements ({0})", static_cast<ezUInt64>(uiSizeBound));
      return EZ_FAILURE;
    }

    out_data.SetCountUninitialized(static_cast<ezUInt32>(uiSizeBound));

    size_t const cSize = ZSTD_compress(out_data.GetData(), uiSizeBound, uncompressedData.GetPtr(), uncompressedData.GetCount(), 1);
    if (ZSTD_isError(cSize))
    {
      ezLog::Error("Compression failed with error: '{0}'.", ZSTD_getErrorName(cSize));
      return EZ_FAILURE;
    }

    out_data.SetCount(static_cast<ezUInt32>(cSize));

    return EZ_SUCCESS;
  }

  static ezResult DecompressZStd(ezArrayPtr<const ezUInt8> compressedData, ezDynamicArray<ezUInt8>& out_data)
  {
    ezUInt64 uiSize = ZSTD_findDecompressedSize(compressedData.GetPtr(), compressedData.GetCount());

    if (uiSize == ZSTD_CONTENTSIZE_ERROR)
    {
      ezLog::Error("Can't decompress since it wasn't compressed with ZStd");
      return EZ_FAILURE;
    }
    else if (uiSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
      ezLog::Error("Can't decompress since the original size can't be determined, was the data compressed using the streaming variant?");
      return EZ_FAILURE;
    }

    if (uiSize > ezMath::MaxValue<ezUInt32>())
    {
      ezLog::Error("Can't compress since the output container can't hold enough elements ({0})", uiSize);
      return EZ_FAILURE;
    }

    out_data.SetCountUninitialized(static_cast<ezUInt32>(uiSize));

    size_t const uiActualSize = ZSTD_decompress(out_data.GetData(), ezMath::SafeConvertToSizeT(uiSize), compressedData.GetPtr(), compressedData.GetCount());

    if (uiActualSize != uiSize)
    {
      ezLog::Error("Error during ZStd decompression: '{0}'.", ZSTD_getErrorName(uiActualSize));
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }
#endif

  ezResult Compress(ezArrayPtr<const ezUInt8> uncompressedData, ezCompressionMethod method, ezDynamicArray<ezUInt8>& out_data)
  {
    out_data.Clear();

    if (uncompressedData.IsEmpty())
    {
      return EZ_SUCCESS;
    }

    switch (method)
    {
      case ezCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return CompressZStd(uncompressedData, out_data);
#else
        ezLog::Error("ZStd compression disabled in build settings!");
        return EZ_FAILURE;
#endif
      default:
        ezLog::Error("Unsupported compression method {0}!", static_cast<ezUInt32>(method));
        return EZ_FAILURE;
    }
  }

  ezResult Decompress(ezArrayPtr<const ezUInt8> compressedData, ezCompressionMethod method, ezDynamicArray<ezUInt8>& out_data)
  {
    out_data.Clear();

    if (compressedData.IsEmpty())
    {
      return EZ_SUCCESS;
    }

    switch (method)
    {
      case ezCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return DecompressZStd(compressedData, out_data);
#else
        ezLog::Error("ZStd compression disabled in build settings!");
        return EZ_FAILURE;
#endif
      default:
        ezLog::Error("Unsupported compression method {0}!", static_cast<ezUInt32>(method));
        return EZ_FAILURE;
    }
  }
} // namespace ezCompressionUtils


EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Compression);
