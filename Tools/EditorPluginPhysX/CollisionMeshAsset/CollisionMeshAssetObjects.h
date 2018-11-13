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

    Default = TriangleMesh
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezCollisionMeshType);

struct ezConvexCollisionMeshType
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    ConvexHull,
    Cylinder,

    Default = ConvexHull
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezConvexCollisionMeshType);

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
  bool m_bIsConvexMesh = false;
  ezEnum<ezConvexCollisionMeshType> m_ConvexMeshType;

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
