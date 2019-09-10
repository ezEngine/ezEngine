#pragma once

#include <Core/World/World.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

class EZ_PROCGENPLUGIN_DLL ezVolumeCollection
{
  struct Sphere
  {
    ezSimdMat4f m_GlobalToLocalTransform;
    ezEnum<ezProcGenBlendMode> m_BlendMode;
  };

  float EvaluateAtGlobalPosition(const ezVec3& vPosition, float fInitialValue = 0.0f) const;


  static void FindObjectsInBox(const ezWorld& world, const ezBoundingBox& box, ezSpatialData::Category spatialCategory,
    const ezTagSet& includeTags, ezVolumeCollection& out_Collection);
};

struct EZ_PROCGENPLUGIN_DLL ezMsgExtractVolumes : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgExtractVolumes, ezMessage);

  ezVolumeCollection* m_pCollection = nullptr;
};
