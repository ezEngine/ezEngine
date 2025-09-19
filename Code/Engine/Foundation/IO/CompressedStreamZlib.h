
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/Stream.h>

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

struct z_stream_s;

/// \brief Stream reader for ZIP-compressed data with known size
///
/// Specialized reader for ZIP/APK archive support, particularly for Android APK file access.
/// Unlike the general-purpose ezCompressedStreamReaderZlib, this reader requires the exact
/// compressed input size to be known in advance. Used internally by ezArchiveReader for
/// reading individual compressed entries from archive files.
class EZ_FOUNDATION_DLL ezCompressedStreamReaderZip : public ezStreamReader
{
public:
  ezCompressedStreamReaderZip();
  ~ezCompressedStreamReaderZip();

  /// \brief Configures the reader with input stream and exact compressed size
  ///
  /// The exact compressed input size must be known in advance for proper decompression.
  /// This method can be called multiple times to reuse the decoder instance, which is
  /// more efficient than creating new instances for each decompression operation.
  void SetInputStream(ezStreamReader* pInputStream, ezUInt64 uiInputSize);

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  /// However, since this is a compressed stream, the decompression still needs to be done, so this won't save any time.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) override; // [tested]

private:
  ezUInt64 m_uiRemainingInputSize = 0;
  bool m_bReachedEnd = false;
  ezDynamicArray<ezUInt8> m_CompressedCache;
  ezStreamReader* m_pInputStream = nullptr;
  z_stream_s* m_pZLibStream = nullptr;
};


/// \brief General-purpose zlib decompression stream reader
///
/// Decompresses data that was compressed using ezCompressedStreamWriterZlib or any zlib-compatible format.
/// The reader wraps another stream (file, memory, etc.) as its data source and handles decompression transparently.
/// Uses an internal 256-byte cache to minimize source stream reads and optimize decompression performance.
/// Unlike ezCompressedStreamReaderZip, this reader does not require knowing the compressed size in advance.
class EZ_FOUNDATION_DLL ezCompressedStreamReaderZlib : public ezStreamReader
{
public:
  /// \brief Takes an input stream as the source from which to read the compressed data.
  ezCompressedStreamReaderZlib(ezStreamReader* pInputStream); // [tested]

  ~ezCompressedStreamReaderZlib();                            // [tested]

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  /// However, since this is a compressed stream, the decompression still needs to be done, so this won't save any time.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) override; // [tested]

private:
  bool m_bReachedEnd = false;
  ezDynamicArray<ezUInt8> m_CompressedCache;
  ezStreamReader* m_pInputStream = nullptr;
  z_stream_s* m_pZLibStream = nullptr;
};

/// \brief Zlib compression stream writer for efficient data compression
///
/// Compresses incoming data using zlib and forwards it to another stream (file, memory, etc.).
/// Uses an internal 255-byte cache for efficient compression without requiring the entire dataset
/// in memory or dynamic allocations. Data is compressed incrementally as it arrives.
///
/// Note about Flush(): Calling Flush() writes available compressed data but does not guarantee
/// all input data becomes readable from the output stream, as significant data may remain in
/// the compressor's internal buffers. Use CloseStream() to ensure complete data output.
class EZ_FOUNDATION_DLL ezCompressedStreamWriterZlib : public ezStreamWriter
{
public:
  /// \brief Compression level settings balancing speed vs. compression ratio
  enum Compression
  {
    Uncompressed = 0,
    Fastest = 1,
    Fast = 3,
    Average = 5,
    High = 7,
    Highest = 9,
    Default = Fastest ///< Recommended setting: good compression with good speed. Higher levels provide minimal space savings but significantly longer compression times.
  };

  /// \brief The constructor takes another stream writer to pass the output into, and a compression level.
  ezCompressedStreamWriterZlib(ezStreamWriter* pOutputStream, Compression ratio = Compression::Default); // [tested]

  /// \brief Calls CloseStream() internally.
  ~ezCompressedStreamWriterZlib(); // [tested]

  /// \brief Compresses \a uiBytesToWrite from \a pWriteBuffer.
  ///
  /// Will output bursts of 256 bytes to the output stream every once in a while.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override; // [tested]

  /// \brief Finishes the stream and writes all remaining data to the output stream.
  ///
  /// After calling this function, no more data can be written to the stream. GetCompressedSize() will return the final compressed size
  /// of the data.
  /// Note that this function is not the same as Flush(), since Flush() assumes that more data can be written to the stream afterwards,
  /// which is not the case for CloseStream().
  ezResult CloseStream(); // [tested]

  /// \brief Returns the size of the data in its uncompressed state.
  ezUInt64 GetUncompressedSize() const { return m_uiUncompressedSize; } // [tested]

  /// \brief Returns the compressed data size
  ///
  /// Only accurate after CloseStream() has been called. Before that, the value is approximate
  /// because data may still be cached internally. Note that this returns the compressed data size,
  /// not the total bytes written to the output stream, which includes additional framing overhead
  /// (approximately 1 byte per 255 compressed bytes, plus one terminator byte).
  ezUInt64 GetCompressedSize() const { return m_uiCompressedSize; } // [tested]

  /// \brief Writes the currently available compressed data to the stream.
  ///
  /// This does NOT guarantee that you can read all the uncompressed data from the output stream afterwards, because a lot of data
  /// will still be inside the compressor and thus not yet written to the stream.
  virtual ezResult Flush() override;

private:
  ezUInt64 m_uiUncompressedSize = 0;
  ezUInt64 m_uiCompressedSize = 0;

  ezStreamWriter* m_pOutputStream = nullptr;
  z_stream_s* m_pZLibStream = nullptr;

  ezDynamicArray<ezUInt8> m_CompressedCache;
};

#endif // BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
