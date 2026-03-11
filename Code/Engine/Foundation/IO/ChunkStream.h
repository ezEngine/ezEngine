#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/Stream.h>

/// \brief Stream writer that organizes data into named, versioned chunks
///
/// This stream writer subdivides data into discrete chunks, each with a name, version, and byte size.
/// Chunks act like logical sub-streams within a single stream, enabling structured data storage where
/// different parts can be versioned independently. The chunk format includes headers with metadata,
/// making it suitable for file formats that need backward compatibility or selective reading.
/// Use BeginStream/EndStream to wrap the entire operation and BeginChunk/EndChunk for individual chunks.
class EZ_FOUNDATION_DLL ezChunkStreamWriter : public ezStreamWriter
{
public:
  /// \brief Pass the underlying stream writer to the constructor.
  ezChunkStreamWriter(ezStreamWriter& inout_stream); // [tested]

  /// \brief Writes bytes directly to the stream. Only allowed when a chunk is open (between BeginChunk / EndChunk).
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override; // [tested]

  /// \brief Initializes the chunk stream with a format version
  ///
  /// Must be called first before writing any chunks. The version number is stored in the stream header
  /// and can be retrieved by ezChunkStreamReader::BeginStream() for format compatibility checks.
  virtual void BeginStream(ezUInt16 uiVersion); // [tested]

  /// \brief Stops writing to the chunk file. Has to be the last thing that is called.
  virtual void EndStream(); // [tested]

  /// \brief Begins a new named chunk with its own version
  ///
  /// Chunks cannot be nested (use multiple chunk writers for nested structures).
  /// The chunk name and version are written to the header for identification and compatibility.
  virtual void BeginChunk(ezStringView sName, ezUInt32 uiVersion); // [tested]

  /// \brief Closes the current chunk.
  virtual void EndChunk(); // [tested]


private:
  bool m_bWritingFile;
  bool m_bWritingChunk;
  ezString m_sChunkName;
  ezDeque<ezUInt8> m_Storage;
  ezStreamWriter& m_Stream;
};


/// \brief Stream reader that parses chunked data written by ezChunkStreamWriter
///
/// This reader provides sequential access to chunks within a stream, allowing selective reading
/// based on chunk names and versions. Each chunk can be read completely or skipped entirely.
/// The reader tracks chunk boundaries automatically and provides metadata about each chunk.
/// Use BeginStream/EndStream to initialize and cleanup, then iterate through chunks with NextChunk().
class EZ_FOUNDATION_DLL ezChunkStreamReader : public ezStreamReader
{
public:
  /// \brief Pass the underlying stream writer to the constructor.
  ezChunkStreamReader(ezStreamReader& inout_stream); // [tested]

  /// \brief Reads data from the current chunk
  ///
  /// Only allowed while a valid chunk is available. Returns 0 bytes when the end of a chunk
  /// is reached, even if there are more chunks to come. Use NextChunk() to advance to the next chunk.
  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead) override; // [tested]

  /// \brief Controls how the reader behaves when ending the chunk file
  enum class EndChunkFileMode
  {
    SkipToEnd,                                                                   ///< Makes sure all data is properly read, so that the stream read position is after the chunk file data. Useful if the chunk file is
                                                                                 ///< embedded in another file stream.
    JustClose                                                                    ///< Just stops, leaving the stream at the last read position. This should be used if definitely nothing more needs to be read from all
                                                                                 ///< underlying streams.
  };

  void SetEndChunkFileMode(EndChunkFileMode mode) { m_EndChunkFileMode = mode; } // [tested]

  /// \brief Initializes reading and returns the stream format version
  ///
  /// Returns the version number that was passed to ezChunkStreamWriter::BeginStream().
  /// Use this version for compatibility checking before processing chunks.
  virtual ezUInt16 BeginStream(); // [tested]

  /// \brief Stops reading from the chunk file. Optionally skips the remaining bytes, so that the underlying streams read position is after the chunk
  /// file content.
  virtual void EndStream(); // [tested]

  /// \brief Metadata and status information for the current chunk
  struct ChunkInfo
  {
    ChunkInfo()
    {
      m_bValid = false;
      m_uiChunkVersion = 0;
      m_uiChunkBytes = 0;
      m_uiUnreadChunkBytes = 0;
    }

    bool m_bValid;                 ///< If this is false, the end of the chunk file has been reached and no further chunk is available.
    ezString m_sChunkName;         ///< The name of the chunk.
    ezUInt32 m_uiChunkVersion;     ///< The version number of the chunk.
    ezUInt32 m_uiChunkBytes;       ///< The total size of the chunk.
    ezUInt32 m_uiUnreadChunkBytes; ///< The number of bytes in the chunk that have not yet been read.
  };

  /// \brief Returns information about the current chunk.
  const ChunkInfo& GetCurrentChunk() const { return m_ChunkInfo; } // [tested]

  /// \brief Skips the rest of the current chunk and starts reading the next chunk.
  void NextChunk(); // [tested]

private:
  void TryReadChunkHeader();

  EndChunkFileMode m_EndChunkFileMode;
  ChunkInfo m_ChunkInfo;

  ezStreamReader& m_Stream;
};
