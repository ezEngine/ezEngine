#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
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
    ezFloat16 m_fValue;
    ezUInt32 m_uiSortingKey;
    float m_fFadeOutScale;
    float m_fFadeOutBias;

    EZ_ALWAYS_INLINE bool operator<(const Sphere& other) const { return m_uiSortingKey < other.m_uiSortingKey; }
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

  void AddSphere(const ezSimdTransform& transform, float fRadius, ezEnum<ezProcGenBlendMode> blendMode, float fSortOrder,
    float fValue, float fFadeOutStart);

private:
  friend class ezVolumeCollection;

  ezVolumeCollection* m_pCollection = nullptr;
};
