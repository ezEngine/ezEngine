#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezAnimationClipAssetDocument;

struct ezRootMotionMode
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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezRootMotionMode);

class ezAnimationClipAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAssetProperties, ezReflectedClass);

public:
  ezAnimationClipAssetProperties();
  ~ezAnimationClipAssetProperties();

  ezString m_sSourceFile;
  ezString m_sAnimationClipToExtract;
  ezDynamicArray<ezString> m_AvailableClips;
  ezUInt32 m_uiFirstFrame = 0;
  ezUInt32 m_uiNumFrames = 0;
  ezString m_sPreviewMesh;
  ezEnum<ezRootMotionMode> m_RootMotionMode;
  ezVec3 m_vConstantRootMotion;
  // ezString m_sJoint1;
  // ezString m_sJoint2;
};

struct ezAnimationClipAssetEvent
{
  enum Type
  {
    Restart,
    LoopChanged,
    SimulationSpeedChanged,
  };

  ezAnimationClipAssetDocument* m_pDocument;
  Type m_Type;
};

//////////////////////////////////////////////////////////////////////////

class ezAnimationClipAssetDocument : public ezSimpleAssetDocument<ezAnimationClipAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAssetDocument, ezSimpleAssetDocument<ezAnimationClipAssetDocument>);

public:
  ezAnimationClipAssetDocument(const char* szDocumentPath);

  ezEvent<const ezAnimationClipAssetEvent&> m_Events;

  void TriggerRestart();

  void SetLoop(bool enable);
  bool GetLoop() const { return m_bLoop; }

  void SetSimulationPaused(bool bPaused);
  bool GetSimulationPaused() const { return m_bSimulationPaused; }

  void SetSimulationSpeed(float speed);
  float GetSimulationSpeed() const { return m_fSimulationSpeed; }


protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual ezStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  // void ApplyCustomRootMotion(ezAnimationClipResourceDescriptor& anim) const;
  // void ExtractRootMotionFromFeet(ezAnimationClipResourceDescriptor& anim, const ezSkeleton& skeleton) const;
  // void MakeRootMotionConstantAverage(ezAnimationClipResourceDescriptor& anim) const;

private:
  bool m_bSimulationPaused = false;
  bool m_bLoop = true;
  float m_fSimulationSpeed = 1.0f;
};

//////////////////////////////////////////////////////////////////////////

class ezAnimationClipAssetDocumentGenerator : public ezAssetDocumentGenerator
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAssetDocumentGenerator, ezAssetDocumentGenerator);

public:
  ezAnimationClipAssetDocumentGenerator();
  ~ezAnimationClipAssetDocumentGenerator();

  virtual void GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual ezStatus Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument) override;
  virtual const char* GetDocumentExtension() const override { return "ezAnimationClipAsset"; }
  virtual const char* GetGeneratorGroup() const override { return "AnimationClipGroup"; }
};
