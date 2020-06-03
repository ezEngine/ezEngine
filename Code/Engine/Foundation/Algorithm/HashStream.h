#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>

typedef struct XXH32_state_s XXH32_state_t;
typedef struct XXH64_state_s XXH64_state_t;

/// \brief A stream writer that hashes the data written to it.
///
/// This stream writer allows to conveniently generate a 32 bit hash value for any kind of data.
class EZ_FOUNDATION_DLL ezHashStreamWriter32 : public ezStreamWriter
{
public:
  /// \brief Pass an initial seed for the hash calculation.
  ezHashStreamWriter32(ezUInt32 seed = 0);
  ~ezHashStreamWriter32();

  /// \brief Writes bytes directly to the stream.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override;

  /// \brief Returns the current hash value. You can read this at any time between write operations, or after writing is done to get the final hash value.
  ezUInt32 GetHashValue() const;

private:
  XXH32_state_t* m_state;
};


/// \brief A stream writer that hashes the data written to it.
///
/// This stream writer allows to conveniently generate a 64 bit hash value for any kind of data.
class EZ_FOUNDATION_DLL ezHashStreamWriter64 : public ezStreamWriter
{
public:
  /// \brief Pass an initial seed for the hash calculation.
  ezHashStreamWriter64(ezUInt64 seed = 0);
  ~ezHashStreamWriter64();

  /// \brief Writes bytes directly to the stream.
  virtual ezResult WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite) override;

  /// \brief Returns the current hash value. You can read this at any time between write operations, or after writing is done to get the final hash value.
  ezUInt64 GetHashValue() const;

private:
  XXH64_state_t* m_state;
};
