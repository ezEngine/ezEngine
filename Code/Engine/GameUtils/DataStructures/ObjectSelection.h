#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>

/// \brief Stores a list of game objects as a 'selection'. Provides some common convenience functions for working with selections.
class EZ_GAMEUTILS_DLL ezObjectSelection
{
public:
  ezObjectSelection();

  /// \brief The ezWorld in which the game objects are stored.
  void SetWorld(ezWorld* pWorld);

  /// \brief Returns the ezWorld in which the game objects live.
  const ezWorld* GetWorld() const { return m_pWorld; }

  /// \brief Clears the selection.
  void Clear() { m_Objects.Clear(); }

  /// \brief Iterates over all objects and removes the ones that have been destroyed from the selection.
  void RemoveDeadObjects();

  /// \brief Adds the given object to the selection, unless it is not valid anymore. Objects can be added multiple times.
  void AddObject(ezGameObjectHandle hObject, bool bDontAddTwice = true);

  /// \brief Removes the first occurrence of the given object from the selection. Returns false if the object did not exist in the selection.
  bool RemoveObject(ezGameObjectHandle hObject);

  /// \brief Removes the object from the selection if it exists already, otherwise adds it.
  void ToggleSelection(ezGameObjectHandle hObject);

  /// \brief Returns the number of objects in the selection.
  ezUInt32 GetCount() const { return m_Objects.GetCount(); }

  /// \brief Returns the n-th object in the selection.
  ezGameObjectHandle GetObject(ezUInt32 index) const { return m_Objects[index]; }

private:
  ezWorld* m_pWorld;
  ezDeque<ezGameObjectHandle> m_Objects;
};

