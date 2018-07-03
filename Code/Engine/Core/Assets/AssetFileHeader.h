#pragma once

#include <Core/Basics.h>
#include <Foundation/IO/Stream.h>

/// \brief Simple class to handle asset file headers (the very first bytes in all transformed asset files)
class EZ_CORE_DLL ezAssetFileHeader
{
public:
  ezAssetFileHeader();

  /// \brief Reads the hash from file. If the file is outdated, the hash is set to 0xFFFFFFFFFFFFFFFF.
  void Read(ezStreamReader& stream);

  /// \brief Writes the asset hash to file (plus a little version info)
  void Write(ezStreamWriter& stream) const;

  /// \brief Checks whether the stored file contains the same hash.
  bool IsFileUpToDate(ezUInt64 uiExpectedHash, ezUInt16 uiVersion) const
  {
    return (m_uiHash == uiExpectedHash && m_uiVersion == uiVersion);
  }

  /// \brief Returns the asset file hash
  ezUInt64 GetFileHash() const { return m_uiHash; }

  /// \brief Sets the asset file hash
  void SetFileHashAndVersion(ezUInt64 hash, ezUInt16 v)
  {
    m_uiHash = hash;
    m_uiVersion = v;
  }

  /// \brief Returns the asset type version
  ezUInt16 GetFileVersion() const { return m_uiVersion; }

  /// \brief Returns size of a serialized header. Needed to read header appended to files.
  ///
  /// \note Verified by unit test
  static EZ_ALWAYS_INLINE ezUInt16 GetSerializedSize() { return 18; }

private:
  ezUInt64 m_uiHash;
  ezUInt16 m_uiVersion;
};
