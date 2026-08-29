#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>

/// Simple class to handle asset file headers (the very first bytes in all transformed asset files)
class EZ_FOUNDATION_DLL ezAssetFileHeader
{
public:
  ezAssetFileHeader();

  /// Reads the hash from file. If the file is outdated, the hash is set to 0xFFFFFFFFFFFFFFFF.
  ezResult Read(ezStreamReader& inout_stream);

  /// Writes the asset hash to file (plus a little version info)
  ezResult Write(ezStreamWriter& inout_stream) const;

  /// Checks whether the stored file contains the same hash.
  bool IsFileUpToDate(ezUInt64 uiExpectedHash, ezUInt16 uiVersion) const { return (m_uiHash == uiExpectedHash && m_uiVersion == uiVersion); }

  /// Returns the asset file hash
  ezUInt64 GetFileHash() const { return m_uiHash; }

  /// Sets the asset file hash
  void SetFileHashAndVersion(ezUInt64 uiHash, ezUInt16 v)
  {
    m_uiHash = uiHash;
    m_uiVersion = v;
  }

  /// Returns the asset type version
  ezUInt16 GetFileVersion() const { return m_uiVersion; }

  /// Returns the generator which was used to produce the asset file
  const ezHashedString& GetGenerator() { return m_sGenerator; }

  /// Allows to set the generator string
  void SetGenerator(ezStringView sGenerator) { m_sGenerator.Assign(sGenerator); }

private:
  // initialize to a 'valid' hash
  // this may get stored, unless someone sets the hash
  ezUInt64 m_uiHash = 0;
  ezUInt16 m_uiVersion = 0;
  ezHashedString m_sGenerator;
};
