#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ModelImporter/Handles.h>

namespace ezModelImporter
{
  /// An object in the hierarchy of an imported scene.
  class EZ_MODELIMPORTER_DLL HierarchyObject
  {
  public:
    /// Casts to another hierarchy object type.
    template<typename T>
    const T* Cast() const;
    template<typename T>
    T* Cast() { return const_cast<T*>(static_cast<const HierarchyObject*>(this)->Cast<T>()); }

    /// Type of this object, used for "dynamic" casting.
    const ObjectHandle::Type m_Type;

    /// A optional name for this node.
    ezString m_Name;

  protected:
    HierarchyObject(ObjectHandle::Type type) : m_Type(type) {}
  };


  template<typename T>
  inline const T* HierarchyObject::Cast() const
  {
    if (std::is_same<T, class HierarchyObject>::value)
      return static_cast<const T*>(this);

    switch (m_Type)
    {
    case ObjectHandle::MESH:
      if (!std::is_same<T, class Mesh>::value) return nullptr;
      break;
    case ObjectHandle::NODE:
      if (!std::is_same<T, class Node>::value) return nullptr;
      break;
    default:
      return nullptr;
    }
    return static_cast<const T*>(this);
  }
}
