#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>

static const char* g_szAssetTag = "ezAsset";

ezAssetFileHeader::ezAssetFileHeader() = default;

enum ezAssetFileHeaderVersion : ezUInt8
{
  Version1 = 1,
  Version2,
  Version3,

  VersionCount,
  VersionCurrent = VersionCount - 1
};

ezResult ezAssetFileHeader::Write(ezStreamWriter& inout_stream) const
{
  EZ_ASSERT_DEBUG(m_uiHash != 0xFFFFFFFFFFFFFFFF, "Cannot write an invalid hash to file");

  // 8 Bytes for identification + version
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(g_szAssetTag, 7));

  const ezUInt8 uiVersion = ezAssetFileHeaderVersion::VersionCurrent;
  inout_stream << uiVersion;

  // 8 Bytes for the hash
  inout_stream << m_uiHash;
  // 2 for the type version
  inout_stream << m_uiVersion;

  inout_stream << m_sGenerator;
  return EZ_SUCCESS;
}

ezResult ezAssetFileHeader::Read(ezStreamReader& inout_stream)
{
  // initialize to 'invalid'
  m_uiHash = 0xFFFFFFFFFFFFFFFF;
  m_uiVersion = 0;

  char szTag[8] = {0};
  if (inout_stream.ReadBytes(szTag, 7) < 7)
  {
    EZ_REPORT_FAILURE("The stream does not contain a valid asset file header");
    return EZ_FAILURE;
  }

  szTag[7] = '\0';

  // invalid asset file ... this is not going to end well
  EZ_ASSERT_DEBUG(ezStringUtils::IsEqual(szTag, g_szAssetTag), "The stream does not contain a valid asset file header");

  if (!ezStringUtils::IsEqual(szTag, g_szAssetTag))
    return EZ_FAILURE;

  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  ezUInt64 uiHash = 0;
  inout_stream >> uiHash;

  // future version?
  EZ_ASSERT_DEV(uiVersion <= ezAssetFileHeaderVersion::VersionCurrent, "Unknown asset header version {0}", uiVersion);

  if (uiVersion >= ezAssetFileHeaderVersion::Version2)
  {
    inout_stream >> m_uiVersion;
  }

  if (uiVersion >= ezAssetFileHeaderVersion::Version3)
  {
    inout_stream >> m_sGenerator;
  }

  // older version? set the hash to 'invalid'
  if (uiVersion != ezAssetFileHeaderVersion::VersionCurrent)
    return EZ_FAILURE;

  m_uiHash = uiHash;

  return EZ_SUCCESS;
}
