#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginKraut/Actions/KrautActions.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

struct ezKrautTreeResourceDescriptor;
struct ezKrautGeneratorResourceDescriptor;

namespace ezModelImporter2
{
  enum class TextureSemantic : ezInt8;
}

struct ezKrautTreeAssetEvent
{
  enum class Type
  {
    WindStrengthChanged,
    FrondsLeavesVisibilityChanged,
  };

  Type m_Type;
};

class ezKrautTreeAssetDocument : public ezSimpleAssetDocument<ezKrautTreeAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeAssetDocument, ezSimpleAssetDocument<ezKrautTreeAssetProperties>);

public:
  ezKrautTreeAssetDocument(ezStringView sDocumentPath);

  ezStatus WriteKrautAsset(ezStreamWriter& ref_stream) const;

  ezKrautWindStrength::Enum GetWindStrength() const { return m_WindStrength; }
  void SetWindStrength(ezKrautWindStrength::Enum strength);

  bool GetShowFrondsLeaves() const { return m_bShowFrondsLeaves; }
  void SetShowFrondsLeaves(bool bShow);

  ezEvent<const ezKrautTreeAssetEvent&> m_Events;

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  void SyncBackAssetProperties(ezKrautTreeAssetProperties*& pProp, const ezKrautGeneratorResourceDescriptor& desc);

  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

private:
  ezKrautWindStrength::Enum m_WindStrength = ezKrautWindStrength::Light;
  bool m_bShowFrondsLeaves = true;
};

//////////////////////////////////////////////////////////////////////////


class ezKrautTreeAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezKrautTreeAssetDocumentGenerator();
  ~ezKrautTreeAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual ezStringView GetDocumentExtension() const override { return "ezKrautTreeAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "KrautTrees"; }
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) override;
};
