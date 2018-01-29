#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>

/// \brief Reads a world description from a stream. Allows to instantiate that world multiple times
///        in different locations and different ezWorld's.
///
/// The reader will ignore unknown component types and skip them during instantiation.
class EZ_CORE_DLL ezWorldReader
{
public:
  ezWorldReader();

  /// \brief Reads all information about the world from the given stream.
  ///
  /// Call this once to populate ezWorldReader with information how to instantiate the world.
  /// Afterwards \a stream can be deleted.
  /// Call InstantiateWorld() or InstantiatePrefab() afterwards as often as you like
  /// to actually get an objects into an ezWorld.
  void ReadWorldDescription(ezStreamReader& stream);

  /// \brief Creates one instance of the world that was previously read by ReadWorldDescription().
  ///
  /// This is identical to calling InstantiatePrefab() with identity values, however, it is a bit
  /// more efficient, as unnecessary computations are skipped.
  ///
  /// If pOverrideTeamID is not null, every instantiated game object will get it passed in as its new value.
  /// This can be used to identify that the object belongs to a specific player or team.
  void InstantiateWorld(ezWorld& world, const ezUInt16* pOverrideTeamID = nullptr);

  /// \brief Creates one instance of the world that was previously read by ReadWorldDescription().
  ///
  /// \param rootTransform is an additional transform that is applied to all root objects.
  /// \param hParent allows to attach the newly created objects immediately to a parent
  /// \param out_CreatedRootObjects If this is valid, all pointers the to created root objects are stored in this array
  ///
  /// If pOverrideTeamID is not null, every instantiated game object will get it passed in as its new value.
  /// This can be used to identify that the object belongs to a specific player or team.
  void InstantiatePrefab(ezWorld& world, const ezTransform& rootTransform, ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, ezHybridArray<ezGameObject*, 8>* out_CreatedChildObjects, const ezUInt16* pOverrideTeamID);

  /// \brief Gives access to the stream of data. Use this inside component deserialization functions to read data.
  ezStreamReader& GetStream() const { return *m_pStream; }

  /// \brief Used during component deserialization to read a handle to a game object.
  ezGameObjectHandle ReadGameObjectHandle();

  /// \brief Used during component deserialization to read a handle to a component.
  ///
  /// The handle might not have been created at this point. Therefore all such reads are queued
  /// and fulfilled at the very end of the reading process. Therefore the ezComponentHandle pointer
  /// must stay valid throughout the deserialization process.
  void ReadComponentHandle(ezComponentHandle* out_hComponent);

  /// \brief Used during component deserialization to query the actual version number with which the
  /// given component type was written. The version number is given through the EZ_BEGIN_COMPONENT_TYPE
  /// macro. Whenever the serialization of a component changes, that number should be increased.
  ezUInt32 GetComponentTypeVersion(const ezRTTI* pRtti) const;

  /// \brief Clears all data.
  void ClearAndCompact();

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const;

private:

  struct GameObjectToCreate
  {
    ezGameObjectDesc m_Desc;
    ezString m_sGlobalKey;
    ezUInt32 m_uiParentHandleIdx;
  };

  void ReadGameObjectDesc(GameObjectToCreate& godesc);
  void ReadComponentInfo(ezUInt32 uiComponentTypeIdx);
  void ReadComponentsOfType(ezUInt32 uiComponentTypeIdx);
  void FulfillComponentHandleRequets();
  void Instantiate(ezWorld& world, bool bUseTransform, const ezTransform& rootTransform, ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, ezHybridArray<ezGameObject*, 8>* out_CreatedChildObjects, const ezUInt16* pOverrideTeamID);

  void CreateGameObjects(const ezDynamicArray<GameObjectToCreate>& objects, ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, const ezUInt16* pOverrideTeamID);
  void CreateGameObjects(const ezDynamicArray<GameObjectToCreate>& objects, const ezTransform& rootTransform, ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, const ezUInt16* pOverrideTeamID);

  ezStreamReader* m_pStream;
  ezWorld* m_pWorld;

  ezUInt8 m_uiVersion;
  ezUInt32 m_uiMaxComponents;
  ezDynamicArray<ezGameObjectHandle> m_IndexToGameObjectHandle;
  ezDynamicArray<ezComponentHandle> m_IndexToComponentHandle;

  struct CompRequest
  {
    EZ_DECLARE_POD_TYPE();

    ezComponentHandle* m_pWriteToComponent;
    ezUInt32 m_uiComponentIndex;
  };


  ezDynamicArray<GameObjectToCreate> m_RootObjectsToCreate;
  ezDynamicArray<GameObjectToCreate> m_ChildObjectsToCreate;

  ezHybridArray<CompRequest, 64> m_ComponentHandleRequests;
  ezDynamicArray<const ezRTTI*> m_ComponentTypes;
  ezHashTable<const ezRTTI*, ezUInt32> m_ComponentTypeVersions;
  ezMemoryStreamStorage m_ComponentStream;
  ezResourceHandleReadContext m_HandleReadContext;
};


