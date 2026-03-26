#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Declarations.h>

EZ_DECLARE_FLAGS(ezUInt16, ezParticleStreamMask, Position, Velocity, Color, Size, LifeTime, Rotation);

/// Pre-sampled curve data for fast lookups via sampleCurve() in expressions.
struct EZ_PARTICLEPLUGIN_DLL ezParticleExpressionCurveSamples : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleExpressionCurveSamples, ezReflectedClass);

  ezDynamicArray<float> m_Samples; ///< Uniformly spaced samples covering [m_fMinX, m_fMaxX].
  float m_fMinX = 0.0f;            ///< X position of the first sample.
  float m_fMaxX = 1.0f;            ///< X position of the last sample.
};

/// One input slot for the expression behavior, holding either an inline or shared curve.
struct EZ_PARTICLEPLUGIN_DLL ezParticleExpressionInput
{
  ezEnum<ezCurveSource> m_CurveSource;
  ezSingleCurveData m_Curve;        ///< Edit-time curve data (available in editor). Converted to m_RuntimeCurve on save.
  ezCurve1DResourceHandle m_hSharedCurve;
  mutable ezCurve1D m_RuntimeCurve; ///< Populated from m_Curve on save or directly from stream on load.
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleExpressionInput);

/// Evaluates a math expression per-particle, either to modify an attribute or to discard particles.
///
/// Particle streams are accessible via inPos, inVel, inColor, inSize, inRotSpeed, inLifeTime
/// and outPos, outVel, outColor, outSize, outRotSpeed, outLifeTime, outDiscard.
/// The global variable timeDiff provides the frame delta time.
///
/// Curve inputs defined on the factory can be sampled via sampleCurve(index, at),
/// where index is the 0-based position in the Inputs array and at is the lookup position.
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
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  ezString m_sExpression;
  ezHybridArray<ezParticleExpressionInput, 2> m_Inputs;

  /// Null if the expression is empty or failed to compile.
  const ezExpressionByteCode* GetByteCode() const { return m_bBytecodeValid ? &m_ByteCode : nullptr; }

private:
  /// Parses and compiles m_sExpression into bytecode. Derives m_InputStreams, m_OutputStreams, and parameter name lists from which stream names appear in the expression text. No-ops if the expression has not changed since the last call.
  void CompileExpression(const ezParticleEffectDescriptor& ownerEffectDescriptor);

  /// Evaluates all input curves (inline or shared) and stores fixed-size sample tables in m_CurveSamples for use by sampleCurve() at runtime.
  void BuildCurveSamples();

  ezExpressionByteCode m_ByteCode;
  ezString m_sCompiledExpression;
  bool m_bBytecodeValid = false;
  bool m_bUsesDiscard = false;
  ezBitflags<ezParticleStreamMask> m_InputStreams;
  ezBitflags<ezParticleStreamMask> m_OutputStreams;
  ezDynamicArray<ezParticleExpressionCurveSamples> m_CurveSamples;
  ezDynamicArray<ezHashedString> m_FloatParamNames;
  ezDynamicArray<ezHashedString> m_ColorParamNames;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Expression final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Expression, ezParticleBehavior);

public:
  ezParticleBehavior_Expression();

  bool m_bUsesDiscard = false;
  ezBitflags<ezParticleStreamMask> m_InputStreams;
  ezBitflags<ezParticleStreamMask> m_OutputStreams;

  /// Points to the byte code owned by the factory. Null if the expression is invalid.
  const ezExpressionByteCode* m_pByteCode = nullptr;

  /// Points to the pre-sampled curve data owned by the factory. Null if there are no curve inputs.
  const ezDynamicArray<ezParticleExpressionCurveSamples>* m_pCurveSamples = nullptr;

  /// Points to the effect parameter name lists owned by the factory. Null if no parameters are used.
  const ezDynamicArray<ezHashedString>* m_pFloatParamNames = nullptr;
  const ezDynamicArray<ezHashedString>* m_pColorParamNames = nullptr;

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
