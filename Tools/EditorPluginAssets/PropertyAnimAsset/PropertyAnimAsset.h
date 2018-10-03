#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <Foundation/Communication/Event.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>

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
  typedef ezSimpleAssetDocument<ezPropertyAnimationTrackGroup, ezGameObjectContextDocument> BaseClass;
  EZ_ADD_DYNAMIC_REFLECTION(ezPropertyAnimAssetDocument, BaseClass);

public:
  ezPropertyAnimAssetDocument(const char* szDocumentPath);
  ~ezPropertyAnimAssetDocument();

  virtual const char* QueryAssetType() const override { return "PropertyAnim"; }
  virtual ezObjectAccessorBase* GetObjectAccessor() const override;

  void SetAnimationDurationTicks(ezUInt64 uiNumTicks);
  ezUInt64 GetAnimationDurationTicks() const;
  ezTime GetAnimationDurationTime() const;
  void AdjustDuration();

  bool SetScrubberPosition(ezUInt64 uiTick);
  ezUInt64 GetScrubberPosition() const { return m_uiScrubberTickPos; }

  ezEvent<const ezPropertyAnimAssetDocumentEvent&> m_PropertyAnimEvents;

  void SetPlayAnimation(bool play);
  bool GetPlayAnimation() const { return m_bPlayAnimation; }
  void SetRepeatAnimation(bool repeat);
  bool GetRepeatAnimation() const { return m_bRepeatAnimation; }
  void ExecuteAnimationPlaybackStep();

  const ezPropertyAnimationTrack* GetTrack(const ezUuid& trackGuid) const;
  ezPropertyAnimationTrack* GetTrack(const ezUuid& trackGuid);

  ezStatus CanAnimate(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target) const;

  ezUuid FindTrack(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target) const;
  ezUuid CreateTrack(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target);

  ezUuid FindCurveCp(const ezUuid& trackGuid, ezInt64 tickX);
  ezUuid InsertCurveCpAt(const ezUuid& trackGuid, ezInt64 tickX, double newPosY);

  ezUuid FindGradientColorCp(const ezUuid& trackGuid, ezInt64 tickX);
  ezUuid InsertGradientColorCpAt(const ezUuid& trackGuid, ezInt64 tickX, const ezColorGammaUB& color);

  ezUuid FindGradientAlphaCp(const ezUuid& trackGuid, ezInt64 tickX);
  ezUuid InsertGradientAlphaCpAt(const ezUuid& trackGuid, ezInt64 tickX, ezUInt8 alpha);

  ezUuid FindGradientIntensityCp(const ezUuid& trackGuid, ezInt64 tickX);
  ezUuid InsertGradientIntensityCpAt(const ezUuid& trackGuid, ezInt64 tickX, float intensity);

  ezUuid InsertEventTrackCpAt(ezInt64 tickX, const char* szValue);

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezAssetPlatformConfig* pPlatformConfig, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual void InitializeAfterLoading() override;

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
      return ezHashing::xxHash32(&key.m_Object, sizeof(ezUuid))
        + ezHashing::xxHash32(&key.m_pProperty, sizeof(const ezAbstractProperty*))
        + (ezUInt32)key.m_Index.ComputeHash();
    }

    EZ_ALWAYS_INLINE static bool Equal(const ezPropertyReference& a, const ezPropertyReference& b)
    {
      return a.m_Object == b.m_Object && a.m_pProperty == b.m_pProperty && a.m_Index == b.m_Index;
    }
  };

  void RebuildMapping();
  void RemoveTrack(const ezUuid& track);
  void AddTrack(const ezUuid& track);
  void FindTrackKeys(const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath, ezHybridArray<ezPropertyReference, 1>& keys) const;
  void GenerateTrackInfo(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index,
    ezStringBuilder& sObjectSearchSequence, ezStringBuilder& sComponentType, ezStringBuilder& sPropertyPath) const;
  void ApplyAnimation();
  void ApplyAnimation(const ezPropertyReference& key, const PropertyValue& value);

  ezHashTable<ezPropertyReference, PropertyValue, PropertyKeyHash> m_PropertyTable;
  ezHashTable<ezUuid, ezHybridArray<ezPropertyReference, 1>> m_TrackTable;

  ezUniquePtr<ezPropertyAnimObjectAccessor> m_pAccessor;

  bool m_bPlayAnimation = false;
  bool m_bRepeatAnimation = false;
  ezTime m_LastFrameTime;
  ezUInt64 m_uiScrubberTickPos = 0;
};
