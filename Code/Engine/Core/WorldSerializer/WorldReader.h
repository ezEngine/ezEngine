#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>

class ezStringDeduplicationReadContext;
class ezProgress;
class ezProgressRange;

struct ezPrefabInstantiationOptions
{
  ezGameObjectHandle m_hParent;

  ezDynamicArray<ezGameObject*>* m_pCreatedRootObjectsOut = nullptr;
  ezDynamicArray<ezGameObject*>* m_pCreatedChildObjectsOut = nullptr;
  const ezUInt16* m_pOverrideTeamID = nullptr;

  bool m_bForceDynamic = false;

  /// \brief If the prefab has a single root node with this non-empty name, rather than creating a new object, instead the m_hParent object is used.
  ezTempHashedString m_ReplaceNamedRootWithParent;

  enum class RandomSeedMode
  {
    DeterministicFromParent, ///< ezWorld::CreateObject() will either derive a deterministic value from the parent object, or assign a random value, if no parent exists
    CompletelyRandom,        ///< ezWorld::CreateObject() will assign a random value to this object
    FixedFromSerialization,  ///< Keep deserialized random seed value
    CustomRootValue,         ///< Use the given seed root value to assign a deterministic (but different) value to each game object.
  };

  RandomSeedMode m_RandomSeedMode = RandomSeedMode::DeterministicFromParent;
  ezUInt32 m_uiCustomRandomSeedRootValue = 0;

  ezTime m_MaxStepTime = ezTime::MakeZero();

  ezProgress* m_pProgress = nullptr;
};

/// \brief Reads a world description from a stream. Allows to instantiate that world multiple times
///        in different locations and different ezWorld's.
///
/// The reader will ignore unknown component types and skip them during instantiation.
class EZ_CORE_DLL ezWorldReader
{
public:
  /// \brief A context object is returned from InstantiateWorld or InstantiatePrefab if a maxStepTime greater than zero is specified.
  ///
  /// Call the Step() function periodically to complete the instantiation.
  /// Each step will try to spend not more than the given maxStepTime.
  /// E.g. this is useful if the instantiation cost of large prefabs needs to be distributed over multiple frames.
  class InstantiationContextBase
  {
  public:
    enum class StepResult
    {
      Continue,          ///< The available time slice is used up. Call Step() again to continue the process.
      ContinueNextFrame, ///< The process has reached a point where you need to call ezWorld::Update(). Otherwise no further progress can be made.
      Finished,          ///< The instantiation is finished and you can delete the context. Don't call 'Step()' on it again.
    };

    virtual ~InstantiationContextBase() = default;

    /// \Brief Advance the instantiation by one step
    /// \return Whether the operation is finished or needs to be repeated.
    virtual StepResult Step() = 0;

    /// \Brief Cancel the instantiation. This might lead to inconsistent states and must be used with care.
    virtual void Cancel() = 0;
  };

  ezWorldReader();
  ~ezWorldReader();

  /// \brief Reads all information about the world from the given stream.
  ///
  /// Call this once to populate ezWorldReader with information how to instantiate the world.
  /// Afterwards \a stream can be deleted.
  /// Call InstantiateWorld() or InstantiatePrefab() afterwards as often as you like
  /// to actually get an objects into an ezWorld.
  /// By default, the method will warn if it skips bytes in the stream that are of unknown
  /// types. The warnings can be suppressed by setting warningOnUnkownSkip to false.
  ezResult ReadWorldDescription(ezStreamReader& inout_stream, bool bWarningOnUnkownSkip = true);

  /// \brief Creates one instance of the world that was previously read by ReadWorldDescription().
  ///
  /// This is identical to calling InstantiatePrefab() with identity values, however, it is a bit
  /// more efficient, as unnecessary computations are skipped.
  ///
  /// If pOverrideTeamID is not null, every instantiated game object will get it passed in as its new value.
  /// This can be used to identify that the object belongs to a specific player or team.
  ///
  /// If maxStepTime is not zero the function will return a valid ptr to an InstantiationContextBase.
  /// This context will only spend the given amount of time in its Step() function.
  /// The function has to be periodically called until it returns true to complete the instantiation.
  ///
  /// If pProgress is a valid pointer it is used to track the progress of the instantiation. The ezProgress object
  /// has to be valid as long as the instantiation is in progress.
  ezUniquePtr<InstantiationContextBase> InstantiateWorld(ezWorld& ref_world, const ezUInt16* pOverrideTeamID = nullptr, ezTime maxStepTime = ezTime::MakeZero(), ezProgress* pProgress = nullptr);

  /// \brief Creates one instance of the world that was previously read by ReadWorldDescription().
  ///
  /// \param rootTransform is an additional transform that is applied to all root objects.
  /// \param hParent allows to attach the newly created objects immediately to a parent
  /// \param out_CreatedRootObjects If this is valid, all pointers the to created root objects are stored in this array
  ///
  /// If pOverrideTeamID is not null, every instantiated game object will get it passed in as its new value.
  /// This can be used to identify that the object belongs to a specific player or team.
  ///
  /// If maxStepTime is not zero the function will return a valid ptr to an InstantiationContextBase.
  /// This context will only spend the given amount of time in its Step() function.
  /// The function has to be periodically called until it returns true to complete the instantiation.
  ///
  /// If pProgress is a valid pointer it is used to track the progress of the instantiation. The ezProgress object
  /// has to be valid as long as the instantiation is in progress.
  ezUniquePtr<InstantiationContextBase> InstantiatePrefab(ezWorld& ref_world, const ezTransform& rootTransform, const ezPrefabInstantiationOptions& options);

  /// \brief Gives access to the stream of data. Use this inside component deserialization functions to read data.
  ezStreamReader& GetStream() const { return *m_pStream; }

  /// \brief Used during component deserialization to read a handle to a game object.
  ezGameObjectHandle ReadGameObjectHandle();

  /// \brief Used during component deserialization to read a handle to a component.
  void ReadComponentHandle(ezComponentHandle& out_hComponent);

  /// \brief Used during component deserialization to query the actual version number with which the
  /// given component type was written. The version number is given through the EZ_BEGIN_COMPONENT_TYPE
  /// macro. Whenever the serialization of a component changes, that number should be increased.
  ezUInt32 GetComponentTypeVersion(const ezRTTI* pRtti) const;

  /// \brief Returns whether world contains a component of given type.
  bool HasComponentOfType(const ezRTTI* pRtti) const;

  /// \brief Clears all data.
  void ClearAndCompact();

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const;

  using FindComponentTypeCallback = ezDelegate<const ezRTTI*(ezStringView sTypeName)>;

  /// \brief An optional callback to redirect the lookup of a component type name to an ezRTTI type.
  ///
  /// If specified, this is used by ALL world readers. The intention is to use this either for logging purposes,
  /// or to implement a whitelist or blacklist for specific component types.
  /// E.g. if the callback returns nullptr, the component type is 'unknown' and skipped by the world reader.
  /// Thus one can remove unwanted component types.
  /// Theoretically one could also redirect an old (or renamed) component type to a new one,
  /// given that their deserialization code is compatible.
  static FindComponentTypeCallback s_FindComponentTypeCallback;

  ezUInt32 GetRootObjectCount() const;
  ezUInt32 GetChildObjectCount() const;

  static void SetMaxStepTime(InstantiationContextBase* pContext, ezTime maxStepTime);
  static ezTime GetMaxStepTime(InstantiationContextBase* pContext);

private:
  struct GameObjectToCreate
  {
    ezGameObjectDesc m_Desc;
    ezString m_sGlobalKey;
    ezUInt32 m_uiParentHandleIdx;
  };

  void ReadGameObjectDesc(GameObjectToCreate& godesc);
  void ReadComponentTypeInfo(ezUInt32 uiComponentTypeIdx);
  void ReadComponentDataToMemStream(bool warningOnUnknownSkip = true);
  void ClearHandles();
  ezUniquePtr<InstantiationContextBase> Instantiate(ezWorld& world, bool bUseTransform, const ezTransform& rootTransform, const ezPrefabInstantiationOptions& options);

  ezStreamReader* m_pStream = nullptr;
  ezWorld* m_pWorld = nullptr;

  ezUInt8 m_uiVersion = 0;
  ezDynamicArray<ezGameObjectHandle> m_IndexToGameObjectHandle;

  ezDynamicArray<GameObjectToCreate> m_RootObjectsToCreate;
  ezDynamicArray<GameObjectToCreate> m_ChildObjectsToCreate;

  struct ComponentTypeInfo
  {
    const ezRTTI* m_pRtti = nullptr;
    ezDynamicArray<ezComponentHandle> m_ComponentIndexToHandle;
    ezUInt32 m_uiNumComponents = 0;
  };

  ezDynamicArray<ComponentTypeInfo> m_ComponentTypes;
  ezHashTable<const ezRTTI*, ezUInt32> m_ComponentTypeVersions;
  ezDefaultMemoryStreamStorage m_ComponentCreationStream;
  ezDefaultMemoryStreamStorage m_ComponentDataStream;
  ezUInt64 m_uiTotalNumComponents = 0;

  ezUniquePtr<ezStringDeduplicationReadContext> m_pStringDedupReadContext;

  class InstantiationContext : public InstantiationContextBase
  {
  public:
    InstantiationContext(ezWorldReader& ref_worldReader, bool bUseTransform, const ezTransform& rootTransform, const ezPrefabInstantiationOptions& options);
    ~InstantiationContext();

    virtual StepResult Step() override;
    virtual void Cancel() override;

    template <bool UseTransform>
    bool CreateGameObjects(const ezDynamicArray<GameObjectToCreate>& objects, ezGameObjectHandle hParent, ezDynamicArray<ezGameObject*>* out_pCreatedObjects, ezTime endTime);

    bool CreateComponents(ezTime endTime);
    bool DeserializeComponents(ezTime endTime);
    bool AddComponentsToBatch(ezTime endTime);

    void SetMaxStepTime(ezTime stepTime);
    ezTime GetMaxStepTime() const;

  private:
    void BeginNextProgressStep(ezStringView sName);
    void SetSubProgressCompletion(double fCompletion);

    friend class ezWorldReader;
    ezWorldReader& m_WorldReader;

    bool m_bUseTransform = false;
    ezTransform m_RootTransform;

    ezPrefabInstantiationOptions m_Options;

    ezComponentInitBatchHandle m_hComponentInitBatch;

    // Current state
    struct Phase
    {
      enum Enum
      {
        Invalid = -1,
        CreateRootObjects,
        CreateChildObjects,
        CreateComponents,
        DeserializeComponents,
        AddComponentsToBatch,
        InitComponents,

        Count
      };
    };

    Phase::Enum m_Phase = Phase::Invalid;
    ezUInt32 m_uiCurrentIndex = 0; // object or component
    ezUInt32 m_uiCurrentComponentTypeIndex = 0;
    ezUInt64 m_uiCurrentNumComponentsProcessed = 0;
    ezMemoryStreamReader m_CurrentReader;

    ezUniquePtr<ezProgressRange> m_pOverallProgressRange;
    ezUniquePtr<ezProgressRange> m_pSubProgressRange;
  };
};
