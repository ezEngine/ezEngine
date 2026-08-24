#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Utilities/AssetInfoFile.h>

// Version of the file layout, not of the values inside it.
static constexpr ezUInt16 s_uiAssetInfoFileVersion = 1;

static constexpr ezStringView s_sRootObject = "AssetInfo"_ezsv;
static constexpr ezStringView s_sValuesObject = "Values"_ezsv;
static constexpr ezStringView s_sVersion = "Version"_ezsv;
static constexpr ezStringView s_sAssetHash = "AssetHash"_ezsv;
static constexpr ezStringView s_sTypeVersion = "TypeVersion"_ezsv;

void ezAssetInfoFile::SetValue(ezStringView sKey, const ezVariant& value)
{
  if (!value.IsValid())
  {
    m_Values.Remove(sKey);
    return;
  }

  m_Values[sKey] = value;
}

ezVariant ezAssetInfoFile::GetValue(ezStringView sKey) const
{
  auto it = m_Values.Find(sKey);

  if (!it.IsValid())
    return ezVariant();

  return it.Value();
}

ezResult ezAssetInfoFile::Write(ezStreamWriter& inout_stream, const ezAssetFileHeader& header) const
{
  ezOpenDdlWriter ddl;
  ddl.SetOutputStream(&inout_stream);

  ddl.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);

  ddl.BeginObject(s_sRootObject);
  {
    ezOpenDdlUtils::StoreUInt16(ddl, s_uiAssetInfoFileVersion, s_sVersion);

    // Stored here rather than in a binary ezAssetFileHeader, so that the entire file stays readable text.
    ezOpenDdlUtils::StoreUInt64(ddl, header.GetFileHash(), s_sAssetHash);
    ezOpenDdlUtils::StoreUInt16(ddl, header.GetFileVersion(), s_sTypeVersion);

    ddl.BeginObject(s_sValuesObject);
    {
      for (auto it = m_Values.GetIterator(); it.IsValid(); ++it)
      {
        ezOpenDdlUtils::StoreVariant(ddl, it.Value(), it.Key());
      }
    }
    ddl.EndObject();
  }
  ddl.EndObject();

  return EZ_SUCCESS;
}

ezResult ezAssetInfoFile::Read(ezStreamReader& inout_stream, ezAssetFileHeader& out_header)
{
  m_Values.Clear();

  ezOpenDdlReader ddl;
  if (ddl.ParseDocument(inout_stream).Failed())
    return EZ_FAILURE;

  const ezOpenDdlReaderElement* pRoot = ddl.GetRootElement()->FindChildOfType(s_sRootObject);

  if (pRoot == nullptr)
    return EZ_FAILURE;

  const ezOpenDdlReaderElement* pVersion = pRoot->FindChildOfType(ezOpenDdlPrimitiveType::UInt16, s_sVersion);

  // A newer version may be structured differently, so it is not read at all.
  if (pVersion == nullptr || *pVersion->GetPrimitivesUInt16() > s_uiAssetInfoFileVersion)
    return EZ_FAILURE;

  const ezOpenDdlReaderElement* pHash = pRoot->FindChildOfType(ezOpenDdlPrimitiveType::UInt64, s_sAssetHash);
  const ezOpenDdlReaderElement* pTypeVersion = pRoot->FindChildOfType(ezOpenDdlPrimitiveType::UInt16, s_sTypeVersion);

  if (pHash == nullptr || pTypeVersion == nullptr)
    return EZ_FAILURE;

  out_header.SetFileHashAndVersion(*pHash->GetPrimitivesUInt64(), *pTypeVersion->GetPrimitivesUInt16());

  if (const ezOpenDdlReaderElement* pValues = pRoot->FindChildOfType(s_sValuesObject))
  {
    for (const ezOpenDdlReaderElement* pChild = pValues->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
    {
      if (pChild->GetName().IsEmpty())
        continue;

      ezVariant value;

      // Skip a value of a type this build cannot represent, the remaining ones are still useful.
      if (ezOpenDdlUtils::ConvertToVariant(pChild, value).Failed())
        continue;

      m_Values[pChild->GetName()] = value;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezAssetInfoFile::WriteToFile(ezStringView sAbsolutePath, const ezAssetFileHeader& header) const
{
  // Remove a file from an earlier transform, it would be mistaken for current information.
  if (m_Values.IsEmpty())
  {
    ezOSFile::DeleteFile(sAbsolutePath).IgnoreResult();
    return EZ_SUCCESS;
  }

  ezDeferredFileWriter file;
  file.SetOutput(sAbsolutePath);

  if (Write(file, header).Failed())
  {
    file.Discard();
    return EZ_FAILURE;
  }

  return file.Close();
}

ezResult ezAssetInfoFile::ReadFromFile(ezStringView sAbsolutePath, ezUInt64 uiExpectedHash, ezUInt16 uiExpectedTypeVersion)
{
  m_Values.Clear();

  ezFileReader file;
  if (file.Open(sAbsolutePath).Failed())
    return EZ_FAILURE;

  ezAssetFileHeader header;
  EZ_SUCCEED_OR_RETURN(Read(file, header));

  if (!header.IsFileUpToDate(uiExpectedHash, uiExpectedTypeVersion))
  {
    m_Values.Clear();
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezStringBuilder ezAssetInfoFile::GetInfoFilePathForOutput(ezStringView sAbsoluteOutputPath)
{
  ezStringBuilder sPath = sAbsoluteOutputPath;
  sPath.Append(".ezAssetInfo");
  return sPath;
}

// Keys that are only ever shown as part of a combined line, and must not also be listed on their own.
static bool IsPartOfCombinedLine(ezStringView sKey)
{
  return sKey == ezAssetInfoFile::Keys::ImageHeight;
}

bool ezAssetInfoFile::AppendValueToDisplayString(ezStringBuilder& ref_sOut, ezStringView sKey, ezStringView sLinePrefix) const
{
  const ezVariant value = GetValue(sKey);

  if (!value.IsValid() || IsPartOfCombinedLine(sKey))
    return false;

  if (sKey == Keys::ImageWidth)
  {
    const ezVariant height = GetValue(Keys::ImageHeight);

    if (!height.IsValid())
      return false;

    ref_sOut.AppendFormat("{}Resolution: {} x {}", sLinePrefix, value, height);
    return true;
  }

  // The size is more useful than the half extents that it is stored as.
  if (sKey == Keys::BoundsHalfExtents)
  {
    if (!value.IsA<ezVec3>())
      return false;

    const ezVec3 vHalf = value.Get<ezVec3>();
    ref_sOut.AppendFormat("{}Size: {} x {} x {}", sLinePrefix, ezArgF(vHalf.x * 2, 3), ezArgF(vHalf.y * 2, 3), ezArgF(vHalf.z * 2, 3));
    return true;
  }

  // The center is shown as the range it produces, which reveals whether the origin sits inside the object or at its base.
  if (sKey == Keys::BoundsCenter)
  {
    const ezVariant halfExtents = GetValue(Keys::BoundsHalfExtents);

    if (!value.IsA<ezVec3>() || !halfExtents.IsValid() || !halfExtents.IsA<ezVec3>())
      return false;

    const ezVec3 vCenter = value.Get<ezVec3>();
    const ezVec3 vHalf = halfExtents.Get<ezVec3>();
    ref_sOut.AppendFormat("{}Bounds: {} {} {} to {} {} {}", sLinePrefix,
      ezArgF(vCenter.x - vHalf.x, 3), ezArgF(vCenter.y - vHalf.y, 3), ezArgF(vCenter.z - vHalf.z, 3),
      ezArgF(vCenter.x + vHalf.x, 3), ezArgF(vCenter.y + vHalf.y, 3), ezArgF(vCenter.z + vHalf.z, 3));
    return true;
  }

  const ezStringView sLabel = ezTranslate(sKey);

  // Recorded lists are long enough to swamp everything else, so only the element count is shown.
  if (value.IsA<ezVariantArray>())
  {
    ref_sOut.AppendFormat("{}{}: {}", sLinePrefix, sLabel, value.Get<ezVariantArray>().GetCount());
    return true;
  }

  ref_sOut.AppendFormat("{}{}: {}", sLinePrefix, sLabel, value);
  return true;
}

void ezAssetInfoFile::AppendToDisplayString(ezStringBuilder& ref_sOut, ezStringView sLinePrefix) const
{
  for (auto it = m_Values.GetIterator(); it.IsValid(); ++it)
  {
    AppendValueToDisplayString(ref_sOut, it.Key(), sLinePrefix);
  }
}

void ezAssetInfoFile::AppendValuesToDisplayString(ezStringBuilder& ref_sOut, ezArrayPtr<const ezStringView> keys, ezStringView sLinePrefix) const
{
  for (ezStringView sKey : keys)
  {
    AppendValueToDisplayString(ref_sOut, sKey, sLinePrefix);
  }
}
