#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct ezPropertyMetaStateEvent;

struct ezMeshPrimitive
{
  using StorageType = ezInt8;

  enum Enum
  {
    File,
    Box,
    Rect,
    Cylinder,
    Cone,
    Pyramid,
    Sphere,
    HalfSphere,
    GeodesicSphere,
    Capsule,
    Torus,

    Default = File
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezMeshPrimitive);

class ezMeshAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshAssetProperties, ezReflectedClass);

public:
  ezMeshAssetProperties();
  ~ezMeshAssetProperties();

  static void PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

  ezString m_sMeshFile;
  ezString m_sMeshIncludeTags;
  ezString m_sMeshExcludeTags;
  float m_fUniformScaling = 1.0f;

  float m_fRadius = 0.5f;
  float m_fRadius2 = 0.5f;
  float m_fHeight = 1.0f;
  ezAngle m_Angle = ezAngle::MakeFromDegree(360.0f);
  ezUInt16 m_uiDetail = 0;
  ezUInt16 m_uiDetail2 = 0;
  bool m_bCap = true;
  bool m_bCap2 = true;

  ezEnum<ezMeshImportTransform> m_ImportTransform;
  ezEnum<ezBasisAxis> m_RightDir = ezBasisAxis::NegativeX;
  ezEnum<ezBasisAxis> m_UpDir = ezBasisAxis::PositiveY;
  bool m_bFlipForwardDir = false;

  ezMeshPrimitive::Enum m_PrimitiveType = ezMeshPrimitive::Default;

  bool m_bRecalculateNormals = false;
  bool m_bRecalculateTangents = true;
  bool m_bImportMaterials = true;

  bool m_bHighPrecision = false;
  ezEnum<ezMeshVertexColorConversion> m_VertexColorConversion;

  ezHybridArray<ezMaterialResourceSlot, 8> m_Slots;

  ezUInt32 m_uiVertices = 0;
  ezUInt32 m_uiTriangles = 0;

  bool m_bSimplifyMesh = false;
  bool m_bAggressiveSimplification = false;
  ezUInt8 m_uiMeshSimplification = 50;
  ezUInt8 m_uiMaxSimplificationError = 5;
};
