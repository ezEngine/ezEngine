#pragma once

#include <Core/World/World.h>
#include <Core/WorldSerializer/ResourceHandleStreamOperations.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/UniquePtr.h>

class ezStringDeduplicationReadContext;
class ezProgress;
class ezProgressRange;

/// \brief Reads a world description from a stream. Allows to instantiate that world multiple times
///        in different locations and different ezWorld's.
///
/// The reader will ignore unknown component types and skip them during instantiation.
class EZ_CORE_DLL ezWorldReader
{
public:
  /// \brief A context object is returned from InstantiateWorld or InstantiatePrefab if a maxStepTime greater than zero is specified.
  ///
  /// Call the Step function periodically until it returns true to complete the instantiation.
  /// Each step will try to spend not more than the given maxStepTime.
  /// E.g. this is useful if the instantiation cost of large prefabs needs to be distributed over multiple frames.
  class InstantiationContextBase
  {
  public:
    virtual ~InstantiationContextBase() {}
    virtual bool Step() = 0; // returns true if instantiation is done
  };

  ezWorldReader();
  ~ezWorldReader();

  /// \brief Reads all information about the world from the given stream.
  ///
  /// Call this once to populate ezWorldReader with information how to instantiate the world.
  /// Afterwards \a stream can be deleted.
  /// Call InstantiateWorld() or InstantiatePrefab() afterwards as often as you like
  /// to actually get an objects into an ezWorld.
  ezResult ReadWorldDescription(ezStreamReader& stream);

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
  ezUniquePtr<InstantiationContextBase> InstantiateWorld(ezWorld& world, const ezUInt16* pOverrideTeamID = nullptr,
    ezTime maxStepTime = ezTime::Zero(), ezProgress* pProgress = nullptr);

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
  ezUniquePtr<InstantiationContextBase> InstantiatePrefab(ezWorld& world, const ezTransform& rootTransform, ezGameObjectHandle hParent,
    ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, ezHybridArray<ezGameObject*, 8>* out_CreatedChildObjects,
    const ezUInt16* pOverrideTeamID, bool bForceDynamic, ezTime maxStepTime = ezTime::Zero(), ezProgress* pProgress = nullptr);

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

  /// \brief Clears all data.
  void ClearAndCompact();

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const;

  using FindComponentTypeCallback = ezDelegate<const ezRTTI*(const char* szTypeName)>;

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

private:
  struct GameObjectToCreate
  {
    ezGameObjectDesc m_Desc;
    ezString m_sGlobalKey;
    ezUInt32 m_uiParentHandleIdx;
  };

  void ReadGameObjectDesc(GameObjectToCreate& godesc);
  void ReadComponentTypeInfo(ezUInt32 uiComponentTypeIdx);
  void ReadComponentDataToMemStream();
  void ClearHandles();
  ezUniquePtr<InstantiationContextBase> Instantiate(ezWorld& world, bool bUseTransform, const ezTransform& rootTransform,
    ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, ezHybridArray<ezGameObject*, 8>* out_CreatedChildObjects,
    const ezUInt16* pOverrideTeamID, bool bForceDynamic, ezTime maxStepTime, ezProgress* pProgress);

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
  ezMemoryStreamStorage m_ComponentCreationStream;
  ezMemoryStreamStorage m_ComponentDataStream;
  ezUInt64 m_uiTotalNumComponents = 0;

  ezUniquePtr<ezStringDeduplicationReadContext> m_pStringDedupReadContext;

  class InstantiationContext : public InstantiationContextBase
  {
  public:
    InstantiationContext(ezWorldReader& worldReader, bool bUseTransform, const ezTransform& rootTransform,
      ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, ezHybridArray<ezGameObject*, 8>* out_CreatedChildObjects,
      const ezUInt16* pOverrideTeamID, bool bForceDynamic, ezTime maxStepTime, ezProgress* pProgress);
    ~InstantiationContext();

    virtual bool Step();

    template <bool UseTransform>
    bool CreateGameObjects(const ezDynamicArray<GameObjectToCreate>& objects, ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedObjects, ezTime endTime);

    bool CreateComponents(ezTime endTime);
    bool DeserializeComponents(ezTime endTime);
    bool AddComponentsToBatch(ezTime endTime);

  private:
    void BeginNextProgressStep(const char* szName);
    void SetSubProgressCompletion(double fCompletion);

    friend class ezWorldReader;
    ezWorldReader& m_WorldReader;

    bool m_bUseTransform = false;
    bool m_bForceDynamic = false;
    ezTransform m_RootTransform;
    ezGameObjectHandle m_hParent;
    ezHybridArray<ezGameObject*, 8>* m_pCreatedRootObjects;
    ezHybridArray<ezGameObject*, 8>* m_pCreatedChildObjects;
    const ezUInt16* m_pOverrideTeamID = nullptr;
    ezTime m_MaxStepTime;
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
