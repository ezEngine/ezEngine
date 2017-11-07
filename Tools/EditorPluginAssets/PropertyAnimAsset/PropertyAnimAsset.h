#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <Foundation/Communication/Event.h>

struct ezGameObjectContextEvent;
class ezPropertyAnimObjectAccessor;
class ezPropertyAnimAssetDocument;
struct ezCommandHistoryEvent;

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

struct ezPropertyAnimAssetDocumentEvent
{
  enum class Type
  {
    AnimationLengthChanged,
    ScrubberPositionChanged,
  };

  const ezPropertyAnimAssetDocument* m_pDocument;
  Type m_Type;
};

class ezPropertyAnimAssetDocument : public ezSimpleAssetDocument<ezPropertyAnimationTrackGroup, ezGameObjectContextDocument>
{
  typedef ezSimpleAssetDocument<ezPropertyAnimationTrackGroup, ezGameObjectContextDocument> BaseClass;
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimAssetDocument, BaseClass);

public:
  ezPropertyAnimAssetDocument(const char* szDocumentPath);
  ~ezPropertyAnimAssetDocument();

  virtual const char* QueryAssetType() const override { return "PropertyAnim"; }
  virtual ezObjectAccessorBase* GetObjectAccessor() const override;

  ezInt64 GetAnimationDurationTicks() const;
  ezTime GetAnimationDurationTime() const;
  void ClearCachedAnimationDuration() { m_iCachedAnimationDuration = 0; }

  void SetScrubberPosition(ezUInt64 uiTick);
  ezUInt64 GetScrubberPosition() const { return m_uiScrubberTickPos; }

  ezEvent<const ezPropertyAnimAssetDocumentEvent&> m_PropertyAnimEvents;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;

private:
  void GameObjectContextEventHandler(const ezGameObjectContextEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);

  ezUniquePtr<ezPropertyAnimObjectAccessor> m_pAccessor;

  ezUInt64 m_uiScrubberTickPos = 0;
  mutable ezInt64 m_iCachedAnimationDuration = 0;
  mutable ezInt64 m_iLastAnimationDuration = 0;
};
