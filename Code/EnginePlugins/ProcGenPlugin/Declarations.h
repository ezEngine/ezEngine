#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <ProcGenPlugin/ProcGenPluginDLL.h>

class ezExpressionByteCode;
typedef ezTypedResourceHandle<class ezColorGradientResource> ezColorGradientResourceHandle;
typedef ezTypedResourceHandle<class ezPrefabResource> ezPrefabResourceHandle;
typedef ezTypedResourceHandle<class ezSurfaceResource> ezSurfaceResourceHandle;

struct ezProcGenBlendMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Add,
    Subtract,
    Multiply,
    Divide,
    Max,
    Min,
    Set,

    Default = Multiply
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcGenBlendMode);

namespace ezProcGenInternal
{
  class PlacementTile;
  class FindPlacementTilesTask;
  class PreparePlacementTask;
  class PlacementTask;
  class VertexColorTask;

  struct Pattern
  {
    struct Point
    {
      ezVec2 m_Coordinates;
      float m_fThreshold;
    };

    ezArrayPtr<Point> m_Points;
    float m_fSize;
  };

  struct Output : public ezRefCounted
  {
    ezHashedString m_sName;

    ezHybridArray<ezUInt8, 4> m_VolumeIndices;

    ezExpressionByteCode* m_pByteCode = nullptr;
  };

  struct PlacementOutput : public Output
  {
    float GetTileSize() const { return m_pPattern->m_fSize * m_fFootprint; }

    bool IsValid() const
    {
      return !m_ObjectsToPlace.IsEmpty() && m_pPattern != nullptr && m_fFootprint > 0.0f && m_fCullDistance > 0.0f &&
             m_pByteCode != nullptr;
    }    

    ezHybridArray<ezPrefabResourceHandle, 4> m_ObjectsToPlace;

    const Pattern* m_pPattern = nullptr;
    float m_fFootprint = 1.0f;

    ezVec3 m_vMinOffset = ezVec3::ZeroVector();
    ezVec3 m_vMaxOffset = ezVec3::ZeroVector();

    float m_fAlignToNormal = 1.0f;

    ezVec3 m_vMinScale = ezVec3(1.0f);
    ezVec3 m_vMaxScale = ezVec3(1.0f);

    float m_fCullDistance = 30.0f;

    ezUInt32 m_uiCollisionLayer = 0;

    ezColorGradientResourceHandle m_hColorGradient;

    ezSurfaceResourceHandle m_hSurface;
  };

  struct VertexColorOutput : public Output
  {
  };

  struct EZ_PROCGENPLUGIN_DLL ExpressionInputs
  {
    static ezHashedString s_sPositionX;
    static ezHashedString s_sPositionY;
    static ezHashedString s_sPositionZ;
    static ezHashedString s_sNormalX;
    static ezHashedString s_sNormalY;
    static ezHashedString s_sNormalZ;
    static ezHashedString s_sPointIndex;
  };

  struct EZ_PROCGENPLUGIN_DLL ExpressionOutputs
  {
    static ezHashedString s_sDensity;
    static ezHashedString s_sScale;
    static ezHashedString s_sColorIndex;
    static ezHashedString s_sObjectIndex;

    static ezHashedString s_sR;
    static ezHashedString s_sG;
    static ezHashedString s_sB;
    static ezHashedString s_sA;
  };

  struct PlacementPoint
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPosition;
    float m_fScale;
    ezVec3 m_vNormal;
    ezUInt8 m_uiColorIndex;
    ezUInt8 m_uiObjectIndex;
    ezUInt16 m_uiPointIndex;
  };

  struct PlacementTransform
  {
    EZ_DECLARE_POD_TYPE();

    ezSimdTransform m_Transform;
    ezColorGammaUB m_Color;
    ezUInt8 m_uiObjectIndex;
    ezUInt16 m_uiPointIndex;
  };

  struct PlacementTileDesc
  {
    ezComponentHandle m_hComponent;
    ezUInt32 m_uiOutputIndex;
    ezInt32 m_iPosX;
    ezInt32 m_iPosY;
    float m_fMinZ;
    float m_fMaxZ;
    float m_fPatternSize;
    float m_fDistanceToCamera;

    ezHybridArray<ezSimdMat4f, 8, ezAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;
  };
} // namespace ezProcGenInternal
