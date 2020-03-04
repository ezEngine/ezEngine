#pragma once

#include <Core/World/World.h>
#include <Core/WorldSerializer/ResourceHandleStreamOperations.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/TagSet.h>

/// \brief Stores an entire ezWorld in a stream.
///
/// Used for exporting a world in binary form either as a level or as a prefab (though there is no
/// difference).
/// Can be used for saving a game, if the exact state of the world shall be stored (e.g. like in an FPS).
class EZ_CORE_DLL ezWorldWriter
{
public:
  /// \brief Writes all content in \a world to \a stream.
  ///
  /// All game objects with tags that overlap with \a pExclude will be ignored.
  void WriteWorld(ezStreamWriter& stream, ezWorld& world, const ezTagSet* pExclude = nullptr);

  /// \brief Only writes the given root objects and all their children to the stream.
  void WriteObjects(ezStreamWriter& stream, const ezDeque<const ezGameObject*>& rootObjects);

  /// \brief Only writes the given root objects and all their children to the stream.
  void WriteObjects(ezStreamWriter& stream, ezArrayPtr<const ezGameObject*> rootObjects);

  /// \brief Writes the given game object handle to the stream.
  ///
  /// \note If the handle belongs to an object that is not part of the serialized scene, e.g. an object
  /// that was excluded by a tag, this function will assert.
  void WriteGameObjectHandle(const ezGameObjectHandle& hObject);

  /// \brief Writes the given component handle to the stream.
  ///
  /// \note If the handle belongs to a component that is not part of the serialized scene, e.g. an object
  /// that was excluded by a tag, this function will assert.
  void WriteComponentHandle(const ezComponentHandle& hComponent);

  /// \brief Accesses the stream to which data is written. Use this in component serialization functions
  /// to write data to the stream.
  ezStreamWriter& GetStream() const { return *m_pStream; }

  /// \brief Returns an array containing all game object pointers that were written to the stream as root objects
  const ezDeque<const ezGameObject*>& GetAllWrittenRootObjects() const { return m_AllRootObjects; }

  /// \brief Returns an array containing all game object pointers that were written to the stream as child objects
  const ezDeque<const ezGameObject*>& GetAllWrittenChildObjects() const { return m_AllChildObjects; }

private:
  void Clear();
  ezResult WriteToStream();
  void AssignGameObjectIndices();
  void AssignComponentHandleIndices();
  void IncludeAllComponentBaseTypes();
  void IncludeAllComponentBaseTypes(const ezRTTI* pRtti);
  void Traverse(ezGameObject* pObject);

  ezVisitorExecution::Enum ObjectTraverser(ezGameObject* pObject);
  void WriteGameObject(const ezGameObject* pObject);
  void WriteComponentTypeInfo(const ezRTTI* pRtti);
  void WriteComponentCreationData(const ezDeque<const ezComponent*>& components);
  void WriteComponentSerializationData(const ezDeque<const ezComponent*>& components);

  ezStreamWriter* m_pStream = nullptr;
  const ezTagSet* m_pExclude = nullptr;

  ezDeque<const ezGameObject*> m_AllRootObjects;
  ezDeque<const ezGameObject*> m_AllChildObjects;
  ezMap<ezGameObjectHandle, ezUInt32> m_WrittenGameObjectHandles;

  struct Components
  {
    ezUInt16 m_uiSerializedTypeIndex = 0;
    ezDeque<const ezComponent*> m_Components;
    ezMap<ezComponentHandle, ezUInt32> m_HandleToIndex;
  };

  ezHashTable<const ezRTTI*, Components> m_AllComponents;
};
