#pragma once

#include <KrautGenerator/Description/DescriptionEnums.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct KRAUT_DLL Influence
  {
    Influence();
    virtual ~Influence();

    aeUInt32 m_AffectedBranchTypes = 0;

    virtual const aeVec3 ComputeInfluence(const aeVec3& vPosition) const = 0;
  };

  struct KRAUT_DLL Influence_Position : public Influence
  {
    struct Falloff
    {
      enum Enum
      {
        None,
        Linear,
        Quadratic,
        Hard,

        ENUM_COUNT
      };
    };

    aeVec3 m_vPosition = aeVec3::ZeroVector();
    float m_fStrength = 0;
    float m_fMinRadius = 0;
    float m_fMaxRadius = 0;
    Falloff::Enum m_Falloff = Falloff::None;

    virtual const aeVec3 ComputeInfluence(const aeVec3& vPosition) const override;

    float ComputeInfluenceStrength(const aeVec3& vPosition) const;
  };

  struct KRAUT_DLL Influence_Direction : public Influence_Position
  {
    aeVec3 m_vDirection = aeVec3(1, 0, 0);

    virtual const aeVec3 ComputeInfluence(const aeVec3& vPosition) const override;
  };


} // namespace Kraut
