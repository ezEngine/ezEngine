#include <CoreUtils/PCH.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

static const char* g_szAssetTag = "ezAsset";

ezAssetFileHeader::ezAssetFileHeader()
{
  // initialize to a 'valid' hash
  // this may get stored, unless someone sets the hash
  m_uiHash = 0;
}

enum ezAssetFileHeaderVersion : ezUInt8
{
  Version1 = 1,

  VersionCount,
  VersionCurrent = VersionCount - 1
};

void ezAssetFileHeader::Write(ezStreamWriterBase& stream)
{
  EZ_ASSERT_DEBUG(m_uiHash != 0xFFFFFFFFFFFFFFFF, "Cannot write an invalid hash to file");

  // 8 Bytes for identification + version
  stream.WriteBytes(g_szAssetTag, 7);

  const ezUInt8 uiVersion = ezAssetFileHeaderVersion::VersionCurrent;
  stream << uiVersion;

  // 8 Bytes for the hash
  stream << m_uiHash;
}

void ezAssetFileHeader::Read(ezStreamReaderBase& stream)
{
  // initialize to 'invalid'
  m_uiHash = 0xFFFFFFFFFFFFFFFF;

  char szTag[8];
  stream.ReadBytes(szTag, 7);
  szTag[7] = '\0';

  // invalid asset file ... this is not going to end well
  EZ_ASSERT_DEBUG(ezStringUtils::IsEqual(szTag, g_szAssetTag), "The stream does not contain a valid asset file header");

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  ezUInt64 uiHash = 0;
  stream >> uiHash;

  // future version?
  EZ_ASSERT_DEV(uiVersion <= ezAssetFileHeaderVersion::VersionCurrent, "Unknown asset header version %u", uiVersion);

  // older version? set the hash to 'invalid'
  if (uiVersion != ezAssetFileHeaderVersion::VersionCurrent)
    return;

  m_uiHash = uiHash;
}