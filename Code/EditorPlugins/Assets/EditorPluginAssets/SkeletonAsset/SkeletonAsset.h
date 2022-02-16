#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

//////////////////////////////////////////////////////////////////////////

struct ezPropertyMetaStateEvent;
class ezSkeletonAssetDocument;

struct ezSkeletonAssetEvent
{
  enum Type
  {
    RenderStateChanged,
    Transformed,
  };

  ezSkeletonAssetDocument* m_pDocument = nullptr;
  Type m_Type;
};

class ezSkeletonAssetDocument : public ezSimpleAssetDocument<ezEditableSkeleton>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonAssetDocument, ezSimpleAssetDocument<ezEditableSkeleton>);

public:
  ezSkeletonAssetDocument(const char* szDocumentPath);
  ~ezSkeletonAssetDocument();

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezStatus WriteResource(ezStreamWriter& stream) const;

  bool m_bIsTransforming = false;

  virtual ezManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return ezManipulatorSearchStrategy::SelectedObject;
  }

  const ezEvent<const ezSkeletonAssetEvent&>& Events() const { return m_Events; }

  void SetRenderBones(bool enable);
  bool GetRenderBones() const { return m_bRenderBones; }

  void SetRenderColliders(bool enable);
  bool GetRenderColliders() const { return m_bRenderColliders; }

  void SetRenderJoints(bool enable);
  bool GetRenderJoints() const { return m_bRenderJoints; }

  void SetRenderSwingLimits(bool enable);
  bool GetRenderSwingLimits() const { return m_bRenderSwingLimits; }

  void SetRenderTwistLimits(bool enable);
  bool GetRenderTwistLimits() const { return m_bRenderTwistLimits; }

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual ezStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  void MergeWithNewSkeleton(ezEditableSkeleton& newSkeleton);

  ezEvent<const ezSkeletonAssetEvent&> m_Events;
  bool m_bRenderBones = true;
  bool m_bRenderColliders = true;
  bool m_bRenderJoints = false; // currently not exposed
  bool m_bRenderSwingLimits = true;
  bool m_bRenderTwistLimits = true;
};

//////////////////////////////////////////////////////////////////////////

class ezSkeletonAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezSkeletonAssetDocumentGenerator();
  ~ezSkeletonAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(
    const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezSkeletonAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "AnimationSkeletonGroup"; }
};
