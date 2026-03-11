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
  ezSkeletonAssetDocument(ezStringView sDocumentPath);
  ~ezSkeletonAssetDocument();

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezStatus WriteResource(ezStreamWriter& inout_stream, const ezEditableSkeleton& skeleton) const;

  bool m_bIsTransforming = false;

  virtual ezManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return ezManipulatorSearchStrategy::SelectedObject;
  }

  const ezEvent<const ezSkeletonAssetEvent&>& Events() const { return m_Events; }

  void SetRenderBones(bool bEnable);
  bool GetRenderBones() const { return m_bRenderBones; }

  void SetRenderColliders(bool bEnable);
  bool GetRenderColliders() const { return m_bRenderColliders; }

  void SetRenderJoints(bool bEnable);
  bool GetRenderJoints() const { return m_bRenderJoints; }

  void SetRenderSwingLimits(bool bEnable);
  bool GetRenderSwingLimits() const { return m_bRenderSwingLimits; }

  void SetRenderTwistLimits(bool bEnable);
  bool GetRenderTwistLimits() const { return m_bRenderTwistLimits; }

  void SetRenderPreviewMesh(bool bEnable);
  bool GetRenderPreviewMesh() const { return m_bRenderPreviewMesh; }

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  const ezEditableSkeleton* MergeWithNewSkeleton(ezEditableSkeleton& newSkeleton);

  ezEvent<const ezSkeletonAssetEvent&> m_Events;
  bool m_bRenderBones = true;
  bool m_bRenderColliders = true;
  bool m_bRenderJoints = false; // currently not exposed
  bool m_bRenderSwingLimits = true;
  bool m_bRenderTwistLimits = true;
  bool m_bRenderPreviewMesh = true;
};

//////////////////////////////////////////////////////////////////////////

class ezSkeletonAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezSkeletonAssetDocumentGenerator();
  ~ezSkeletonAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual ezStringView GetDocumentExtension() const override { return "ezSkeletonAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) override;
};
