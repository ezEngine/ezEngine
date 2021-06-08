#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

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
    ConvexDecomposition,

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
  ~ezCollisionMeshAssetProperties();

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezString m_sMeshFile;
  float m_fUniformScaling = 1.0f;

  ezEnum<ezBasisAxis> m_RightDir = ezBasisAxis::PositiveX;
  ezEnum<ezBasisAxis> m_UpDir = ezBasisAxis::PositiveY;
  bool m_bFlipForwardDir = false;
  bool m_bIsConvexMesh = false;
  ezEnum<ezConvexCollisionMeshType> m_ConvexMeshType;
  ezUInt16 m_uiMaxConvexPieces = 2;

  // Cylinder
  float m_fRadius = 0.5f;
  float m_fRadius2 = 0.5f;
  float m_fHeight = 1.0f;
  ezUInt8 m_uiDetail = 1;

  ezHybridArray<ezSurfaceResourceSlot, 8> m_Slots;

  ezUInt32 m_uiVertices = 0;
  ezUInt32 m_uiTriangles = 0;
};
