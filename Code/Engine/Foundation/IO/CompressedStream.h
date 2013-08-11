
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/IBinaryStream.h>
#include <Foundation/Containers/DynamicArray.h>

struct z_stream_s;

/// \brief A stream reader that will decompress data that was stored using the ezCompressedStreamWriter.
///
/// The reader takes another reader as its data source (e.g. a file or a memory stream). The compressed reader
/// uses a cache of 1 KB internally to prevent excessive reads from its source and to improve decompression speed.
/// Note that currently neither ezCompressedStreamReader nor ezCompressedStreamWriter are able to handle streams that are larger than 4 GB.
class EZ_FOUNDATION_DLL ezCompressedStreamReader : public ezIBinaryStreamReader
{
public:
  /// \brief Takes an input stream as the source from which to read the compressed data.
  ezCompressedStreamReader(ezIBinaryStreamReader& InputStream); // [tested]

  ~ezCompressedStreamReader(); // [tested]

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass NULL for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) EZ_OVERRIDE; // [tested]

private:
  ezUInt32 m_uiUncompressedSize;
  ezUInt32 m_uiCompressedSize;
  ezIBinaryStreamReader& m_InputStream;
  z_stream_s* m_pZLibStream;

  ezUInt8 m_CompressedCache[1024];
};

/// \brief A stream writer that will compress all incoming data and then passes it on into another stream.
///
/// The compressed data is cached in an internal array and only written to the output stream when the compressed stream is destroyed.
/// This is necessary to be able to write the length of the compressed data at the beginning of the output data, which is important
/// for the compressed stream reader.
/// Therefore Flush() has no effect on a compressed stream.
/// Note that currently neither ezCompressedStreamReader nor ezCompressedStreamWriter are able to handle streams that are larger than 4 GB.
class EZ_FOUNDATION_DLL ezCompressedStreamWriter : public ezIBinaryStreamWriter
{
public:

  /// \brief Specifies the compression level of the stream.
  enum Compression
  {
    Uncompressed    = 0,
    Fastest         = 1,
    Fast            = 3,
    Average         = 5,
    High            = 7,
    Highest         = 9,
    Default         = Fastest   ///< Should be preferred, good compression and good speed. Higher compression ratios save not much space but take considerably longer.
  };

  /// \brief The constructor takes another stream writer to pass the output into, and a compression level.
  ezCompressedStreamWriter(ezIBinaryStreamWriter& OutputStream, Compression Ratio); // [tested]

  /// \brief The data is only written to the ouptput stream during destruction of this stream.
  ~ezCompressedStreamWriter(); // [tested]

  /// \brief Compresses \a uiBytesToWrite from \a pWriteBuffer.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) EZ_OVERRIDE; // [tested]

  /// \brief Finishes the stream and writes all data to the output stream.
  ///
  /// After calling this function, no more data can be written to the stream. GetCompressedSize() will return the final compressed size
  /// of the data.
  /// Note that this function is not the same as Flush(), since Flush() assumes that more data can be written to the stream afterwards,
  /// which is not the case for CloseStream().
  void CloseStream(); // [tested]

  /// \brief Returns the size of the data in its uncompressed state.
  ezUInt32 GetUncompressedSize() const { return m_uiUncompressedSize; } // [tested]

  /// \brief Returns the current compressed size of the data.
  ///
  /// This value is only accurate after CloseStream() has been called. Before that it is only a rough value, because a lot of data
  /// might still be cached and not yet accounted for.
  /// Note that GetCompressedSize() returns the compressed size of the data, not the size of the data that was written to the output stream,
  /// which will be 8 byte more, because some book keeping information is written to it, as well.
  ezUInt32 GetCompressedSize() const; // [tested]

private:
  ezUInt32 m_uiUncompressedSize;
  ezUInt32 m_uiCompressedSize;

  ezIBinaryStreamWriter& m_OutputStream;
  z_stream_s* m_pZLibStream;

  ezDynamicArray<ezUInt8> m_CompressedData;
};