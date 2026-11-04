#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezSoundBankAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSoundBankAssetProperties, ezReflectedClass);

public:
  ezSoundBankAssetProperties() = default;

  ezString m_sSoundBank;
};

class ezSoundBankAssetDocument : public ezSimpleAssetDocument<ezSoundBankAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSoundBankAssetDocument, ezSimpleAssetDocument<ezSoundBankAssetProperties>);

public:
  ezSoundBankAssetDocument(ezStringView sDocumentPath);

  virtual void OnSubAssetChosen(const ezUuid& subAssetGuid) override { m_SubAssetChosenEvent.Broadcast(subAssetGuid); }

  /// Used by the window to select the sound event that the user has chosen in the asset browser.
  ezEvent<const ezUuid&> m_SubAssetChosenEvent;

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};
