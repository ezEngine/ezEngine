#pragma once

#include <Foundation/Types/Id.h>

namespace ezModelImporter
{
  typedef ezGenericId<24, 8> ObjectId;

  /// A handle to a hierarchy object.
  /// \see ezModelImporter::Scene
  class ObjectHandle
  {
  public:
    EZ_DECLARE_POD_TYPE();

    // Cannot use EZ_DECLARE_HANDLE_TYPE since type is an integral part.

    ObjectHandle() : m_Type(NULLREF), m_Id() {}

    void operator = (const ObjectHandle& rhs);
    bool operator == (const ObjectHandle& rhs) const;

    bool IsValid() const { return m_Type != NULLREF && m_Id != ObjectId(); }

    enum Type
    {
      NODE,
      MESH,
      //LIGHT,
      //CAMERA,

      NULLREF = -1  // null reference.
    };

    Type GetType() const { return m_Type; }

  private:
    // Importer impls are allowed to create these handles ...
    friend class ImporterImplementation;
    // ... and the scene consumses them.
    friend class Scene;


    ObjectHandle(Type type, ObjectId id) : m_Type(type), m_Id(id) {}


    Type m_Type;
    ObjectId m_Id;
  };


  typedef ezGenericId<22, 10> MaterialId;

  /// A handle to a material.
  /// \see ezModelImporter::Scene
  class MaterialHandle
  {
    EZ_DECLARE_HANDLE_TYPE(MaterialHandle, MaterialId);
  };
}
