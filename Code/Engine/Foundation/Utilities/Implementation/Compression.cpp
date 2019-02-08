
#include <FoundationPCH.h>
#include <Foundation/Utilities/Compression.h>
#include <Foundation/Logging/Log.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#define ZSTD_STATIC_LINKING_ONLY // ZSTD_findDecompressedSize
#  include <ThirdParty/zstd/zstd.h>
#endif


namespace ezCompressionUtils
{


#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  static ezResult CompressZStd(ezArrayPtr<const ezUInt8> pUncompressedData, ezDynamicArray<ezUInt8>& out_Data)
  {
    size_t uiSizeBound = ZSTD_compressBound(pUncompressedData.GetCount());
    if(uiSizeBound > std::numeric_limits<ezUInt32>::max())
    {
      ezLog::Error("Can't compress since the output container can't hold enough elements ({0})", static_cast<ezUInt64>(uiSizeBound));
      return EZ_FAILURE;
    }

    out_Data.SetCountUninitialized(static_cast<ezUInt32>(uiSizeBound));

    size_t const cSize = ZSTD_compress(out_Data.GetData(), uiSizeBound, pUncompressedData.GetPtr(), pUncompressedData.GetCount(), 1);
    if (ZSTD_isError(cSize))
    {
      ezLog::Error("Compression failed with error: '{0}'.", ZSTD_getErrorName(cSize));
      return EZ_FAILURE;
    }

    out_Data.SetCount(static_cast<ezUInt32>(cSize));

    return EZ_SUCCESS;
  }

  static ezResult DecompressZStd(ezArrayPtr<const ezUInt8> pCompressedData, ezDynamicArray<ezUInt8>& out_Data)
  {
    size_t uiSize = ZSTD_findDecompressedSize(pCompressedData.GetPtr(), pCompressedData.GetCount());

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

    if (uiSize > std::numeric_limits<ezUInt32>::max())
    {
      ezLog::Error("Can't compress since the output container can't hold enough elements ({0})", static_cast<ezUInt64>(uiSize));
      return EZ_FAILURE;
    }

    out_Data.SetCountUninitialized(static_cast<ezUInt32>(uiSize));

    size_t const uiActualSize = ZSTD_decompress(out_Data.GetData(), uiSize, pCompressedData.GetPtr(), pCompressedData.GetCount());

    if (uiActualSize != uiSize)
    {
      ezLog::Error("Error during ZStd decompression: '{0}'.", ZSTD_getErrorName(uiActualSize));
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }
#endif

  ezResult Compress(ezArrayPtr<const ezUInt8> pUncompressedData, ezCompressionMethod eMethod, ezDynamicArray<ezUInt8>& out_Data)
  {
    out_Data.Clear();

    if (pUncompressedData.IsEmpty())
    {
      return EZ_SUCCESS;
    }

    switch (eMethod)
    {
      case ezCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return CompressZStd(pUncompressedData, out_Data);
#else
        ezLog::Error("ZStd compression disabled in build settings!");
        return EZ_FAILURE;
#endif
      default:
        ezLog::Error("Unsupported compression method {0}!", static_cast<ezUInt32>(eMethod));
        return EZ_FAILURE;
    }
  }

  ezResult Decompress(ezArrayPtr<const ezUInt8> pCompressedData, ezCompressionMethod eMethod, ezDynamicArray<ezUInt8>& out_Data)
  {
    out_Data.Clear();

    if (pCompressedData.IsEmpty())
    {
      return EZ_SUCCESS;
    }

    switch (eMethod)
    {
      case ezCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return DecompressZStd(pCompressedData, out_Data);
#else
        ezLog::Error("ZStd compression disabled in build settings!");
        return EZ_FAILURE;
#endif
      default:
        ezLog::Error("Unsupported compression method {0}!", static_cast<ezUInt32>(eMethod));
        return EZ_FAILURE;
    }
  }
}
