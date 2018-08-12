#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

struct ezPropertyMetaStateEvent;

struct ezSurfaceResourceSlot
{
  ezString m_sLabel;
  ezString m_sResource;
};

struct ezCollisionMeshType
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    ConvexHull,
    TriangleMesh,
    Cylinder,
    //Pyramid,
    //Tetraeder, // 3 sided pyramid
    //Oktaeder, // Double 4 sided pyramid

    Default = TriangleMesh
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezCollisionMeshType);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezSurfaceResourceSlot);

class ezCollisionMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollisionMeshAssetProperties, ezReflectedClass);

public:
  ezCollisionMeshAssetProperties();

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezString m_sMeshFile;
  ezString m_sSubMeshName;
  float m_fUniformScaling;
  ezVec3 m_vNonUniformScaling;

  ezEnum<ezBasisAxis> m_ForwardDir;
  ezEnum<ezBasisAxis> m_RightDir;
  ezEnum<ezBasisAxis> m_UpDir;
  ezEnum<ezCollisionMeshType> m_MeshType;

  // Cylinder
  float m_fRadius;
  float m_fRadius2;
  float m_fHeight;
  ezUInt8 m_uiDetail;

  ezHybridArray<ezSurfaceResourceSlot, 8> m_Slots;

  ezUInt32 m_uiVertices;
  ezUInt32 m_uiTriangles;

private:

};
