#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

class ezAnimationClipAssetDocument;
struct ezPropertyMetaStateEvent;

struct ezRootMotionSource
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,
    Constant,
    // FromFeet,
    // AvgFromFeet,

    Default = None
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezRootMotionSource);

class ezAnimationClipAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAssetProperties, ezReflectedClass);

public:
  ezAnimationClipAssetProperties();
  ~ezAnimationClipAssetProperties();

  ezString m_sSourceFile;
  ezString m_sAnimationClipToExtract;
  ezDynamicArray<ezString> m_AvailableClips;
  bool m_bAdditive = false;
  ezUInt32 m_uiFirstFrame = 0;
  ezUInt32 m_uiNumFrames = 0;
  ezString m_sPreviewMesh;
  ezEnum<ezRootMotionSource> m_RootMotionMode;
  ezVec3 m_vConstantRootMotion;
  // ezString m_sJoint1;
  // ezString m_sJoint2;

  ezEventTrackData m_EventTrack;

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);
};

//////////////////////////////////////////////////////////////////////////

class ezAnimationClipAssetDocument : public ezSimpleAssetDocument<ezAnimationClipAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAssetDocument, ezSimpleAssetDocument<ezAnimationClipAssetProperties>);

public:
  ezAnimationClipAssetDocument(const char* szDocumentPath);

  virtual void SetCommonAssetUiState(ezCommonAssetUiState::Enum state, double value) override;
  virtual double GetCommonAssetUiState(ezCommonAssetUiState::Enum state) const override;

  ezUuid InsertEventTrackCpAt(ezInt64 iTickX, const char* szValue);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  // void ApplyCustomRootMotion(ezAnimationClipResourceDescriptor& anim) const;
  // void ExtractRootMotionFromFeet(ezAnimationClipResourceDescriptor& anim, const ezSkeleton& skeleton) const;
  // void MakeRootMotionConstantAverage(ezAnimationClipResourceDescriptor& anim) const;

private:
  float m_fSimulationSpeed = 1.0f;
};

//////////////////////////////////////////////////////////////////////////

class ezAnimationClipAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezAnimationClipAssetDocumentGenerator();
  ~ezAnimationClipAssetDocumentGenerator();

  virtual void GetImportModes(ezStringView sParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_modes) const override;
  virtual ezStatus Generate(ezStringView sDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual ezStringView GetDocumentExtension() const override { return "ezAnimationClipAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "AnimationClipGroup"; }
};
