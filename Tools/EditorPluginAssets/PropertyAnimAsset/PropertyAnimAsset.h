#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

class ezPropertyAnimationTrack : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimationTrack, ezReflectedClass);
public:

  ezString m_sPropertyName;
  ezEnum<ezPropertyAnimTarget> m_Target;

  ezSingleCurveData m_FloatCurve;
};

class ezPropertyAnimationTrackGroup : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimationTrackGroup, ezReflectedClass);
public:
  ezPropertyAnimationTrackGroup() = default;
  ezPropertyAnimationTrackGroup(const ezPropertyAnimationTrackGroup&) = delete;
  ezPropertyAnimationTrackGroup& operator=(const ezPropertyAnimationTrackGroup& rhs) = delete;
  ~ezPropertyAnimationTrackGroup();

  ezUInt32 m_uiFramesPerSecond = 60;
  ezDynamicArray<ezPropertyAnimationTrack*> m_Tracks;
};

class ezPropertyAnimAssetDocument : public ezSimpleAssetDocument<ezPropertyAnimationTrackGroup>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimAssetDocument, ezSimpleAssetDocument<ezPropertyAnimationTrackGroup>);

public:
  ezPropertyAnimAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "PropertyAnim"; }

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
};
