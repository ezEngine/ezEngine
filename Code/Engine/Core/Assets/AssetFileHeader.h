#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>

/// \brief Simple class to handle asset file headers (the very first bytes in all transformed asset files)
class EZ_CORE_DLL ezAssetFileHeader
{
public:
  ezAssetFileHeader();

  /// \brief Reads the hash from file. If the file is outdated, the hash is set to 0xFFFFFFFFFFFFFFFF.
  ezResult Read(ezStreamReader& stream);

  /// \brief Writes the asset hash to file (plus a little version info)
  ezResult Write(ezStreamWriter& stream) const;

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

  /// \brief Returns the generator which was used to produce the asset file
  const ezHashedString& GetGenerator() { return m_sGenerator; }

  /// \brief Allows to set the generator string
  void SetGenerator(const char* szGenerator) { m_sGenerator.Assign(szGenerator); }

private:
  ezUInt64 m_uiHash;
  ezUInt16 m_uiVersion;
  ezHashedString m_sGenerator;
};
