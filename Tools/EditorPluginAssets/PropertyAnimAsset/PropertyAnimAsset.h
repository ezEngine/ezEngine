#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>

class ezPropertyAnimationTrack : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimationTrack, ezReflectedClass);
public:

  ezString m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  ezString m_sComponentType; ///< Empty to reference the game object properties (position etc.)
  ezString m_sPropertyPath;
  ezEnum<ezPropertyAnimTarget> m_Target;

  ezSingleCurveData m_FloatCurve;
  ezColorGradientAssetData m_ColorGradient;
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
  ezEnum<ezPropertyAnimMode> m_Mode;
  ezDynamicArray<ezPropertyAnimationTrack*> m_Tracks;
};

class ezPropertyAnimAssetDocument : public ezSimpleAssetDocument<ezPropertyAnimationTrackGroup, ezGameObjectContextDocument>
{
  typedef ezSimpleAssetDocument<ezPropertyAnimationTrackGroup, ezGameObjectContextDocument> BaseClass;
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimAssetDocument, BaseClass);

public:
  ezPropertyAnimAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "PropertyAnim"; }

  ezTime GetAnimationDuration() const;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
};
