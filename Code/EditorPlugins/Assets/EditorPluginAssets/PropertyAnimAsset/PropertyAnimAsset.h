#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <Foundation/Communication/Event.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

struct ezGameObjectContextEvent;
class ezPropertyAnimObjectAccessor;
class ezPropertyAnimAssetDocument;
struct ezCommandHistoryEvent;

class ezPropertyAnimationTrack : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimationTrack, ezReflectedClass);

public:
  ezString m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  ezString m_sComponentType;        ///< Empty to reference the game object properties (position etc.)
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
  ezUInt64 m_uiCurveDuration = 480;
  ezEnum<ezPropertyAnimMode> m_Mode;
  ezDynamicArray<ezPropertyAnimationTrack*> m_Tracks;
  ezEventTrackData m_EventTrack;
};

struct ezPropertyAnimAssetDocumentEvent
{
  enum class Type
  {
    AnimationLengthChanged,
    ScrubberPositionChanged,
    PlaybackChanged,
  };

  const ezPropertyAnimAssetDocument* m_pDocument;
  Type m_Type;
};

class ezPropertyAnimAssetDocument : public ezSimpleAssetDocument<ezPropertyAnimationTrackGroup, ezGameObjectContextDocument>
{
  using BaseClass = ezSimpleAssetDocument<ezPropertyAnimationTrackGroup, ezGameObjectContextDocument>;
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimAssetDocument, BaseClass);

public:
  ezPropertyAnimAssetDocument(const char* szDocumentPath);
  ~ezPropertyAnimAssetDocument();

  void SetAnimationDurationTicks(ezUInt64 uiNumTicks);
  ezUInt64 GetAnimationDurationTicks() const;
  ezTime GetAnimationDurationTime() const;
  void AdjustDuration();

  bool SetScrubberPosition(ezUInt64 uiTick);
  ezUInt64 GetScrubberPosition() const { return m_uiScrubberTickPos; }

  ezEvent<const ezPropertyAnimAssetDocumentEvent&> m_PropertyAnimEvents;

  void SetPlayAnimation(bool bPlay);
  bool GetPlayAnimation() const { return m_bPlayAnimation; }
  void SetRepeatAnimation(bool bRepeat);
  bool GetRepeatAnimation() const { return m_bRepeatAnimation; }
  void ExecuteAnimationPlaybackStep();

  const ezPropertyAnimationTrack* GetTrack(const ezUuid& trackGuid) const;
  ezPropertyAnimationTrack* GetTrack(const ezUuid& trackGuid);

  ezStatus CanAnimate(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target) const;

  ezUuid FindTrack(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target) const;
  ezUuid CreateTrack(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target);

  ezUuid FindCurveCp(const ezUuid& trackGuid, ezInt64 iTickX);
  ezUuid InsertCurveCpAt(const ezUuid& trackGuid, ezInt64 iTickX, double fNewPosY);

  ezUuid FindGradientColorCp(const ezUuid& trackGuid, ezInt64 iTickX);
  ezUuid InsertGradientColorCpAt(const ezUuid& trackGuid, ezInt64 iTickX, const ezColorGammaUB& color);

  ezUuid FindGradientAlphaCp(const ezUuid& trackGuid, ezInt64 iTickX);
  ezUuid InsertGradientAlphaCpAt(const ezUuid& trackGuid, ezInt64 iTickX, ezUInt8 uiAlpha);

  ezUuid FindGradientIntensityCp(const ezUuid& trackGuid, ezInt64 iTickX);
  ezUuid InsertGradientIntensityCpAt(const ezUuid& trackGuid, ezInt64 iTickX, float fIntensity);

  ezUuid InsertEventTrackCpAt(ezInt64 iTickX, const char* szValue);

  virtual ezManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return ezManipulatorSearchStrategy::ChildrenOfSelectedObject;
  }

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

private:
  void GameObjectContextEventHandler(const ezGameObjectContextEvent& e);
  void TreeStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  struct PropertyValue
  {
    ezVariant m_InitialValue;
    ezHybridArray<ezUuid, 3> m_Tracks;
  };
  struct PropertyKeyHash
  {
    EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezPropertyReference& key)
    {
      return ezHashingUtils::xxHash32(&key.m_Object, sizeof(ezUuid)) + ezHashingUtils::xxHash32(&key.m_pProperty, sizeof(const ezAbstractProperty*)) +
             (ezUInt32)key.m_Index.ComputeHash();
    }

    EZ_ALWAYS_INLINE static bool Equal(const ezPropertyReference& a, const ezPropertyReference& b)
    {
      return a.m_Object == b.m_Object && a.m_pProperty == b.m_pProperty && a.m_Index == b.m_Index;
    }
  };

  void RebuildMapping();
  void RemoveTrack(const ezUuid& track);
  void AddTrack(const ezUuid& track);
  ezStatus FindTrackKeys(
    const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath, ezHybridArray<ezPropertyReference, 1>& keys) const;
  void GenerateTrackInfo(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezStringBuilder& sObjectSearchSequence,
    ezStringBuilder& sComponentType, ezStringBuilder& sPropertyPath) const;
  void ApplyAnimation();
  void ApplyAnimation(const ezPropertyReference& key, const PropertyValue& value);

  ezHashTable<ezPropertyReference, PropertyValue, PropertyKeyHash> m_PropertyTable;
  ezHashTable<ezUuid, ezHybridArray<ezPropertyReference, 1>> m_TrackTable;

  bool m_bPlayAnimation = false;
  bool m_bRepeatAnimation = false;
  ezTime m_LastFrameTime;
  ezUInt64 m_uiScrubberTickPos = 0;
};
