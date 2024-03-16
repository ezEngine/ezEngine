#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

class ezAnimationClipAssetDocument;
struct ezPropertyMetaStateEvent;
class ezPlatformProfile;
class ezSkeleton;

struct ezRootMotionSource
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,
    Constant,
    Custom,
    FromFeet,
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

  ezEventTrackData m_EventTrack;
  ezDynamicArray<ezSingleCurveData> m_Curves;
  // ezSingleCurveData m_RootMotionX;
  // ezSingleCurveData m_RootMotionY;
  // ezSingleCurveData m_RootMotionZ;

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);
};

//////////////////////////////////////////////////////////////////////////

class ezAnimationClipAssetDocument : public ezSimpleAssetDocument<ezAnimationClipAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAssetDocument, ezSimpleAssetDocument<ezAnimationClipAssetProperties>);

public:
  ezAnimationClipAssetDocument(ezStringView sDocumentPath);

  virtual void SetCommonAssetUiState(ezCommonAssetUiState::Enum state, double value) override;
  virtual double GetCommonAssetUiState(ezCommonAssetUiState::Enum state) const override;

  ezUuid InsertEventTrackCpAt(ezInt64 iTickX, const char* szValue);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  // void ApplyCustomRootMotion(ezAnimationClipResourceDescriptor& anim) const;
  //void ExtractRootMotionFromFeet(ezAnimationClipResourceDescriptor& anim, const ezSkeleton& skeleton) const;
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

  virtual void GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual ezStringView GetDocumentExtension() const override { return "ezAnimationClipAsset"; }
  virtual ezStringView GetGeneratorGroup() const override { return "AnimationClipGroup"; }
  virtual ezStatus Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments) override;
};
