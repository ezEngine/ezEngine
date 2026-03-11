#pragma once

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Containers/Deque.h>
#include <Utilities/UtilitiesDLL.h>

/// \brief Stores a list of game objects as a 'selection' for editor tools and gameplay systems.
///
/// This class is commonly used in editor applications to track which objects are currently selected
/// for operations like deletion, transformation, or property editing. It can also be used in gameplay
/// systems that need to maintain groups of objects, such as unit selection in strategy games,
/// inventory systems, or quest target tracking.
///
/// The selection automatically handles validation of object handles and provides utilities for
/// common selection operations like adding, removing, and toggling objects.
class EZ_UTILITIES_DLL ezObjectSelection
{
public:
  ezObjectSelection();

  /// \brief Sets the world context for this selection.
  ///
  /// All objects in the selection must belong to the same world. This is used for validation
  /// when adding objects and for cleaning up destroyed objects.
  void SetWorld(ezWorld* pWorld);

  /// \brief Returns the world in which the selected objects exist.
  const ezWorld* GetWorld() const { return m_pWorld; }

  /// \brief Clears the selection.
  void Clear() { m_Objects.Clear(); }

  /// \brief Removes objects that have been destroyed from the selection.
  ///
  /// Game objects can be destroyed at any time, leaving invalid handles in the selection.
  /// Call this periodically (e.g., each frame) to keep the selection clean, or before
  /// performing operations on the selected objects.
  void RemoveDeadObjects();

  /// \brief Adds the given object to the selection, unless it is not valid anymore. Objects can be added multiple times.
  void AddObject(ezGameObjectHandle hObject, bool bDontAddTwice = true);

  /// \brief Removes the first occurrence of the given object from the selection. Returns false if the object did not exist in the
  /// selection.
  bool RemoveObject(ezGameObjectHandle hObject);

  /// \brief Removes the object from the selection if it exists already, otherwise adds it.
  void ToggleSelection(ezGameObjectHandle hObject);

  /// \brief Returns the number of objects currently in the selection.
  ezUInt32 GetCount() const { return m_Objects.GetCount(); }

  /// \brief Returns the n-th object in the selection.
  ezGameObjectHandle GetObject(ezUInt32 uiIndex) const { return m_Objects[uiIndex]; }

private:
  ezWorld* m_pWorld = nullptr;
  ezDeque<ezGameObjectHandle> m_Objects;
};
