#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_PROCEDURALPLACEMENTPLUGIN_LIB
    #define EZ_PROCEDURALPLACEMENTPLUGIN_DLL __declspec(dllexport)
  #else
    #define EZ_PROCEDURALPLACEMENTPLUGIN_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_PROCEDURALPLACEMENTPLUGIN_DLL
#endif

#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>

namespace ezPPInternal
{
  class ByteCode;
  class ActiveTile;

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

  struct Layer
  {
    Layer()
    {
      m_pPattern = nullptr;
      m_fFootprint = 1.0f;
      m_vMinOffset.SetZero();
      m_vMaxOffset.SetZero();
      m_fAlignToNormal = 1.0f;
      m_vMinScale.Set(1.0f);
      m_vMaxScale.Set(1.0f);
      m_fCullDistance = 100.0f;

      //m_ByteCode = nullptr;
    }

    float GetTileSize() const
    {
      return m_pPattern->m_fSize * m_fFootprint;
    }

    ezHashedString m_sName;

    ezHybridArray<ezHashedString, 4> m_ObjectsToPlace;

    Pattern* m_pPattern;
    float m_fFootprint;

    ezVec3 m_vMinOffset;
    ezVec3 m_vMaxOffset;

    float m_fAlignToNormal;

    ezVec3 m_vMinScale;
    ezVec3 m_vMaxScale;

    float m_fCullDistance;

    //ColorGradient m_ColorGradient;

    //ByteCode* m_ByteCode;
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

  struct TileDesc
  {
    ezUInt32 m_uiResourceIdHash;
    ezUInt32 m_uiLayerIndex;
    ezInt32 m_iPosX;
    ezInt32 m_iPosY;
    float m_fMinZ;
    float m_fMaxZ;

    ezHybridArray<ezSimdTransform, 8, ezAlignedAllocatorWrapper> m_LocalBoundingBoxes;
  };
}
