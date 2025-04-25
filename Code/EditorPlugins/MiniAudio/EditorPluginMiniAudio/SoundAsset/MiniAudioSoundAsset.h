#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezMiniAudioSoundAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMiniAudioSoundAssetProperties, ezReflectedClass);

public:
  ezMiniAudioSoundAssetProperties() = default;

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezString m_sGroup;
  bool m_bLoop = false;
  float m_fMinVolume = 1.0f;
  float m_fMaxVolume = 1.0f;
  float m_fMinPitch = 1.0f;
  float m_fMaxPitch = 1.0f;
  bool m_bSpatialize = true;
  float m_fMinDistance = 0.1f;
  float m_fMaxDistance = 10.0f;
  float m_fRolloff = 1.0f;
  float m_fDopplerFactor = 0.0f;
  ezDynamicArray<ezString> m_SoundFiles;
  // disable pitch
  // no global pitch
  // looping
  // fade out duration ?
  // fully decode / stream
};

class ezMiniAudioSoundAssetDocument : public ezSimpleAssetDocument<ezMiniAudioSoundAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMiniAudioSoundAssetDocument, ezSimpleAssetDocument<ezMiniAudioSoundAssetProperties>);

public:
  ezMiniAudioSoundAssetDocument(ezStringView sDocumentPath);

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};

//////////////////////////////////////////////////////////////////////////


class ezMiniAudioSoundAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMiniAudioSoundAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezMiniAudioSoundAssetDocumentGenerator();
  ~ezMiniAudioSoundAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual ezStringView GetDocumentExtension() const override { return "ezMiniAudioSoundAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "Sounds"; }
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) override;
};
