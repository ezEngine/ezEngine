#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

/// How Move behavior changes movement speed
struct EZ_PARTICLEPLUGIN_DLL ezMovementMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Constant,    ///< Constant movement speed
    CustomCurve, ///< Use embedded curve data
    SharedCurve, ///< Reference shared curve resource

    Default = Constant
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezMovementMode);

/// Behavior that moves particles along world axes
///
/// Moves particles at constant speed along the X, Y, or Z axes.
/// Movement direction is in world space.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Move final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Move, ezParticleBehavior);

public:
  ezParticleBehavior_Move();
  ~ezParticleBehavior_Move();


  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  ezEnum<ezMovementMode> m_MoveX_Mode;
  float m_fMoveX_Speed = 0.0f;
  ezSingleCurveData m_MoveX_Curve;
  ezCurve1DResourceHandle m_hMoveX_SharedCurve;
  float m_fMoveX_CurveOffset = 0.0f;
  float m_fMoveX_CurveScale = 1.0f;
  mutable ezCurve1D m_RuntimeMoveX_Curve;

  ezEnum<ezMovementMode> m_MoveY_Mode;
  float m_fMoveY_Speed = 0.0f;
  ezSingleCurveData m_MoveY_Curve;
  ezCurve1DResourceHandle m_hMoveY_SharedCurve;
  float m_fMoveY_CurveOffset = 0.0f;
  float m_fMoveY_CurveScale = 1.0f;
  mutable ezCurve1D m_RuntimeMoveY_Curve;

  ezEnum<ezMovementMode> m_MoveZ_Mode;
  float m_fMoveZ_Speed = 0.0f;
  ezSingleCurveData m_MoveZ_Curve;
  ezCurve1DResourceHandle m_hMoveZ_SharedCurve;
  float m_fMoveZ_CurveOffset = 0.0f;
  float m_fMoveZ_CurveScale = 1.0f;
  mutable ezCurve1D m_RuntimeMoveZ_Curve;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamPosition;
    ezProcessingStream* m_pStreamLifeTime;
  };
};
