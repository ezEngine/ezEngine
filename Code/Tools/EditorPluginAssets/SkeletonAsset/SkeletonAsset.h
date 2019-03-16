#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

//////////////////////////////////////////////////////////////////////////

struct ezPropertyMetaStateEvent;

class ezSkeletonAssetDocument : public ezSimpleAssetDocument<ezEditableSkeleton>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonAssetDocument, ezSimpleAssetDocument<ezSkeletonAssetDocument>);

public:
  ezSkeletonAssetDocument(const char* szDocumentPath);
  ~ezSkeletonAssetDocument();

  virtual const char* QueryAssetType() const override { return "Skeleton"; }

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual ezStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  void MergeWithNewSkeleton(ezEditableSkeleton& newSkeleton);
};

//////////////////////////////////////////////////////////////////////////

class ezSkeletonAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezSkeletonAssetDocumentGenerator();
  ~ezSkeletonAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezSkeletonAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "AnimationSkeletonGroup"; }
};
