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

  ezUInt64 GetAnimationDurationTicks() const;
  ezTime GetAnimationDurationTime() const;
  void ClearCachedAnimationDuration() { m_uiCachedAnimationDuration = 0; }

  bool SetScrubberPosition(ezUInt64 uiTick);
  ezUInt64 GetScrubberPosition() const { return m_uiScrubberTickPos; }

  ezEvent<const ezPropertyAnimAssetDocumentEvent&> m_PropertyAnimEvents;

  void SetPlayAnimation(bool play);
  bool GetPlayAnimation() const { return m_bPlayAnimation; }
  void SetRepeatAnimation(bool repeat);
  bool GetRepeatAnimation() const { return m_bRepeatAnimation; }
  void ExecuteAnimationPlaybackStep();

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual void InitializeAfterLoading() override;

private:
  void GameObjectContextEventHandler(const ezGameObjectContextEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  void TreeStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  struct PropertyKey
  {
    ezUuid m_Object;
    const ezAbstractProperty* m_pProperty;
    ezVariant m_Index;
  };
  struct PropertyValue
  {
    ezVariant m_InitialValue;
    ezHybridArray<ezUuid, 3> m_Tracks;
  };
  struct PropertyKeyHash
  {
    EZ_ALWAYS_INLINE static ezUInt32 Hash(const PropertyKey& key)
    {
      return ezHashing::MurmurHash(&key.m_Object, sizeof(ezUuid))
        + ezHashing::MurmurHash(&key.m_pProperty, sizeof(const ezAbstractProperty*))
        + (ezUInt32)key.m_Index.ComputeHash();
    }

    EZ_ALWAYS_INLINE static bool Equal(const PropertyKey& a, const PropertyKey& b)
    {
      return a.m_Object == b.m_Object && a.m_pProperty == b.m_pProperty && a.m_Index == b.m_Index;
    }
  };

  void RebuildMapping();
  void RemoveTrack(const ezUuid& track);
  void AddTrack(const ezUuid& track);
  void ApplyAnimation();
  void ApplyAnimation(const PropertyKey& key, const PropertyValue& value);

  ezHashTable<PropertyKey, PropertyValue, PropertyKeyHash> m_PropertyTable;
  ezHashTable<ezUuid, ezHybridArray<PropertyKey, 1>> m_TrackTable;

  ezUniquePtr<ezPropertyAnimObjectAccessor> m_pAccessor;

  bool m_bPlayAnimation = false;
  bool m_bRepeatAnimation = false;
  ezTime m_StartPlaybackTime;
  ezUInt64 m_uiStartPlaybackTick = 0;

  ezUInt64 m_uiScrubberTickPos = 0;
  mutable ezUInt64 m_uiCachedAnimationDuration = 0;
  mutable ezUInt64 m_uiLastAnimationDuration = 0;
};
