#pragma once

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezAudioControlCollectionAssetEntry : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioControlCollectionAssetEntry, ezReflectedClass);

public:
  ezString m_sName;
  ezEnum<ezAudioSystemControlType> m_Type;
  ezString m_sControlFile;
};

class ezAudioControlCollectionAsset : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioControlCollectionAsset, ezReflectedClass);

public:
  ezDynamicArray<ezAudioControlCollectionAssetEntry> m_Entries;
};

class ezAudioControlCollectionAssetDocument : public ezSimpleAssetDocument<ezAudioControlCollectionAsset>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioControlCollectionAssetDocument, ezSimpleAssetDocument<ezAudioControlCollectionAsset>);

public:
  ezAudioControlCollectionAssetDocument(const char* szDocumentPath);

protected:
  void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};
