#pragma once

#include <Core/World/World.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

class EZ_PROCGENPLUGIN_DLL ezVolumeCollection : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVolumeCollection, ezReflectedClass);

public:
  struct Sphere
  {
    ezSimdMat4f m_GlobalToLocalTransform;
    ezEnum<ezProcGenBlendMode> m_BlendMode;
    float m_fValue;
    float m_fFadeOutScale;
    float m_fFadeOutBias;
  };

  ezDynamicArray<Sphere, ezAlignedAllocatorWrapper> m_Spheres;

  bool IsEmpty() { return m_Spheres.IsEmpty(); }

  float EvaluateAtGlobalPosition(const ezVec3& vPosition, float fInitialValue = 0.0f) const;

  static void ExtractVolumesInBox(const ezWorld& world, const ezBoundingBox& box, ezSpatialData::Category spatialCategory,
    const ezTagSet& includeTags, ezVolumeCollection& out_Collection, const ezRTTI* pComponentBaseType = nullptr);
};

struct EZ_PROCGENPLUGIN_DLL ezMsgExtractVolumes : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgExtractVolumes, ezMessage);

  void AddSphere(const ezSimdTransform& transform, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fValue, float fFadeOutStart);

private:
  friend class ezVolumeCollection;

  ezVolumeCollection* m_pCollection = nullptr;
};
