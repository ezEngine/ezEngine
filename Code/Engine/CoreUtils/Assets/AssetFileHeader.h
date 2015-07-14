#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/IO/Stream.h>

/// \brief Simple class to handle asset file headers (the very first bytes in all transformed asset files)
class EZ_COREUTILS_DLL ezAssetFileHeader
{
public:
  ezAssetFileHeader();

  /// \brief Reads the hash from file. If the file is outdated, the hash is set to 0xFFFFFFFFFFFFFFFF.
  void Read(ezStreamReaderBase& stream);

  /// \brief Writes the asset hash to file (plus a little version info)
  void Write(ezStreamWriterBase& stream);

  /// \brief Checks whether the stored file contains the same hash.
  bool IsFileUpToDate(ezUInt64 uiExpectedHash) const { return m_uiHash == uiExpectedHash; }

  /// \brief Returns the asset file hash
  ezUInt64 GetFileHash() const { return m_uiHash; }

  /// \brief Sets the asset file hash
  void SetFileHash(ezUInt64 hash) { m_uiHash = hash; }

private:
  ezUInt64 m_uiHash;
};
