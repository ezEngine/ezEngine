#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <EditorPluginAssets/ModelImporter/Handles.h>

namespace ezModelImporter
{
  /// An object in the hierarchy of an imported scene.
  class HierarchyObject
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


    /// Pointer to parent of this object.
    /// If it is invalid, this is a root object.
    ObjectHandle GetParent() const { return m_Parent; }
    void SetParent(ObjectHandle newParent);


  protected:
    HierarchyObject(ObjectHandle::Type type) : m_Type(type) {}

    ObjectHandle m_Parent;
  };


  template<typename T>
  inline const T* HierarchyObject::Cast() const
  {
    switch (m_Type)
    {
    case ObjectHandle::MESH:
      if (!std::is_same<T, class Mesh>::value) return nullptr;
      break;
    case ObjectHandle::NODE:
      if (!std::is_same<T, Node>::value) return nullptr;
      break;
    default:
      return nullptr;
    }
    return static_cast<const T*>(this);
  }
}
