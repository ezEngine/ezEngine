#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

//struct ezResourceSlot
//{
//  ezString m_sLabel;
//  ezString m_sResource;
//};

//EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezResourceSlot);

struct ezCollisionMeshType
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    ConvexHull,
    TriangleMesh,

    Default = ConvexHull
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezCollisionMeshType);

class ezCollisionMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollisionMeshAssetProperties, ezReflectedClass);

public:
  ezCollisionMeshAssetProperties();

  ezString m_sMeshFile;
  float m_fUniformScaling;
  ezVec3 m_vNonUniformScaling;

  ezEnum<ezBasisAxis> m_ForwardDir;
  ezEnum<ezBasisAxis> m_RightDir;
  ezEnum<ezBasisAxis> m_UpDir;
  ezEnum<ezCollisionMeshType> m_MeshType;

  //ezHybridArray<ezResourceSlot, 8> m_Slots;

  //const ezString& GetResourceSlotProperty(ezUInt32 uiSlot) const;

  ezUInt32 m_uiVertices;
  ezUInt32 m_uiTriangles;

private:

};
