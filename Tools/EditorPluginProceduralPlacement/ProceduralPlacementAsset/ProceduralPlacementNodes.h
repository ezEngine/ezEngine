#pragma once

#include <Foundation/Utilities/Node.h>

class ezProceduralPlacementNodeBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementNodeBase, ezReflectedClass);
};

//////////////////////////////////////////////////////////////////////////

class ezProceduralPlacementLayerOutput : public ezProceduralPlacementNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementLayerOutput, ezProceduralPlacementNodeBase);

public:
  void Save(ezStreamWriter& stream);

  ezString m_sName;
  ezHybridArray<ezString, 4> m_ObjectsToPlace;

  float m_fFootprint;

  ezVec3 m_vMinOffset;
  ezVec3 m_vMaxOffset;

  float m_fAlignToNormal;

  ezVec3 m_vMinScale;
  ezVec3 m_vMaxScale;

  float m_fCullDistance;

  ezInputNodePin m_DensityPin;
  ezInputNodePin m_ScalePin;
  ezInputNodePin m_ColorIndexPin;
  ezInputNodePin m_ObjectIndexPin;
};

//////////////////////////////////////////////////////////////////////////

class ezProceduralPlacementRandom : public ezProceduralPlacementNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementRandom, ezProceduralPlacementNodeBase);

public:
  ezOutputNodePin m_OutputValuePin;
};


