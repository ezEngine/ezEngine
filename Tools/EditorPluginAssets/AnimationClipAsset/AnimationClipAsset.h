#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

struct ezAnimationClipResourceDescriptor;
class ezSkeleton;

struct ezRootMotionExtractionMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None,
    Custom,
    FromFeet,
    AvgFromFeet,

    Default = None
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezRootMotionExtractionMode);

class ezAnimationClipAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAssetProperties, ezReflectedClass);

public:
  ezAnimationClipAssetProperties();

  ezString m_sAnimationFile;
  ezUInt32 m_uiFirstFrame = 0;
  ezUInt32 m_uiNumFrames = 0;
  ezEnum<ezRootMotionExtractionMode> m_RootMotionExtraction;
  ezVec3 m_vCustomRootMotion;
  ezString m_sJoint1;
  ezString m_sJoint2;
};

//////////////////////////////////////////////////////////////////////////

class ezAnimationClipAssetDocument : public ezSimpleAssetDocument<ezAnimationClipAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAssetDocument, ezSimpleAssetDocument<ezAnimationClipAssetDocument>);

public:
  ezAnimationClipAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Animation Clip"; }

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezAssetProfile* pAssetProfile,
                                          const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

  void ApplyCustomRootMotion(ezAnimationClipResourceDescriptor& anim) const;
  void ExtractRootMotionFromFeet(ezAnimationClipResourceDescriptor& anim, const ezSkeleton& skeleton) const;
  void MakeRootMotionConstantAverage(ezAnimationClipResourceDescriptor& anim) const;
};

//////////////////////////////////////////////////////////////////////////

class ezAnimationClipAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezAnimationClipAssetDocumentGenerator();
  ~ezAnimationClipAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath,
                              ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                            ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezAnimationClipAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "AnimationClipGroup"; }
};
