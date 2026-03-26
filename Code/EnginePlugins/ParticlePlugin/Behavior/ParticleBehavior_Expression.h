#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleAttributes.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Evaluates a math expression per-particle, either to modify an attribute or to discard particles.
///
/// Variables a, b, c, d are mapped to particle streams. In ModifyAttribute mode the result is
/// written to the chosen output attribute. In Discard mode the particle is killed when the result
/// satisfies the comparison against the threshold.
///
/// Uses the ezExpressionVM for SIMD-batched evaluation.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Expression final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Expression, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Expression();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;
  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  ezEnum<ezParticleExpressionMode> m_Mode;
  ezString m_sExpression;
  ezEnum<ezParticleAttributeRead> m_InputA;
  ezEnum<ezParticleAttributeRead> m_InputB;
  ezEnum<ezParticleAttributeRead> m_InputC;
  ezEnum<ezParticleAttributeRead> m_InputD;
  ezEnum<ezParticleAttributeWrite> m_Output;
  ezEnum<ezComparisonOperator> m_Comparison;
  float m_fThreshold = 0.0f;
  ezSingleCurveData m_Curve;
  ezCurve1DResourceHandle m_hSharedCurve;
  mutable ezCurve1D m_RuntimeCurve;

  /// Null if the expression is empty or failed to compile.
  const ezExpressionByteCode* GetByteCode() const { return m_bBytecodeValid ? &m_ByteCode : nullptr; }

  /// Recompiles the expression if it has changed since the last compile.
  void EnsureExpressionCompiled() const;

private:
  mutable ezExpressionByteCode m_ByteCode;
  mutable ezString m_sCompiledExpression;
  mutable bool m_bBytecodeValid = false;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Expression final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Expression, ezParticleBehavior);

public:
  ezParticleBehavior_Expression();

  ezEnum<ezParticleExpressionMode> m_Mode;
  ezEnum<ezParticleAttributeRead> m_InputA;
  ezEnum<ezParticleAttributeRead> m_InputB;
  ezEnum<ezParticleAttributeRead> m_InputC;
  ezEnum<ezParticleAttributeRead> m_InputD;
  ezEnum<ezParticleAttributeWrite> m_Output;
  ezEnum<ezComparisonOperator> m_Comparison;
  float m_fThreshold = 0.0f;

  /// Points to the byte code owned by the factory. Null if the expression is invalid.
  const ezExpressionByteCode* m_pByteCode = nullptr;

  /// Points to the runtime curve owned by the factory. Null in non-curve modes.
  const ezCurve1D* m_pCurve = nullptr;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;

  void UpdateElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements);

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamRotationSpeed = nullptr;

  ezExpressionVM m_VM;
};
