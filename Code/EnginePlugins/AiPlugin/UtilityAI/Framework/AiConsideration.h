#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiContext.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/GameEngineDLL.h>

class ezAiPerception;
class ezAiPerceptionManager;
struct ezInstanceDataDesc;

/// Maps the normalized input of an ezAiConsideration (0 to 1) to its result (0 to 1).
struct ezAiResponseCurve
{
  using StorageType = ezUInt8;

  enum Enum : StorageType
  {
    Linear,       ///< x
    Quadratic,    ///< x^2: stays low for a long time, rises late.
    InvQuadratic, ///< 1 - (1 - x)^2: rises early, flattens towards 1.
    SmoothStep,   ///< 3x^2 - 2x^3: flat at both ends.
    Logistic,     ///< S-curve that is steeper than SmoothStep, mostly 0 or 1 except around the middle.
    Step,         ///< 0 below the middle of the input range, 1 from the middle on.

    Default = Linear
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiResponseCurve);

/// Rates one aspect of a situation from 0 to 1, e.g. 'how close is the target' or 'how much health is left'.
///
/// Connected to a behavior through its 'Considerations' pin. The behavior multiplies its score with the results of all its
/// considerations (see ezAiBehavior::MakeScore()), so a consideration that returns 0 prevents the behavior from running.
/// When a behavior has several candidate perceptions (e.g. several targets), the considerations are evaluated per candidate
/// and the best candidate wins. Considerations that don't depend on the candidate (e.g. own health) ignore it.
///
/// Derived types only compute a raw value (GetInputValue()). Evaluate() maps it from the input range to 0 to 1
/// and then through the response curve. The input range and curve are reflected with EZ_AI_CONSIDERATION_INPUT_PROPERTIES.
///
/// A consideration object is shared by all agents of the same type and must not be modified after creation.
/// Per-agent state belongs into the instance data, see GetInstanceDataDesc(). The results are cached per decision update,
/// so one consideration can be connected to several behaviors without being computed several times (see ezAiConsiderationManager).
class EZ_AIPLUGIN_DLL ezAiConsideration : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiConsideration, ezReflectedClass);

public:
  ezAiConsideration() = default;
  ~ezAiConsideration() = default;

  /// Shown in the debug output and as the node subtitle. If empty, the type name is used.
  ezHashedString m_sName; // [ property ]

  /// The raw value that maps to 0. May be larger than m_fInputMax, which reverses the direction.
  float m_fInputMin = 0.0f; // [ property ]
  /// The raw value that maps to 1.
  float m_fInputMax = 1.0f;          // [ property ]
  ezEnum<ezAiResponseCurve> m_Curve; // [ property ]
  /// Returns 1 minus the curve result, e.g. to prefer closer targets with a distance consideration.
  bool m_bInvert = false; // [ property ]

  // accessors for EZ_AI_CONSIDERATION_INPUT_PROPERTIES, member properties can't be declared on derived types
  float GetInputMin() const { return m_fInputMin; }
  void SetInputMin(float f) { m_fInputMin = f; }
  float GetInputMax() const { return m_fInputMax; }
  void SetInputMax(float f) { m_fInputMax = f; }
  ezEnum<ezAiResponseCurve> GetCurve() const { return m_Curve; }
  void SetCurve(ezEnum<ezAiResponseCurve> curve) { m_Curve = curve; }
  bool GetInvert() const { return m_bInvert; }
  void SetInvert(bool b) { m_bInvert = b; }

  /// Calls ezAiPerceptionManager::FlagAsNeeded() for every input pin that GetInputValue() reads.
  ///
  /// Called for the considerations of every behavior that is available in the decision update.
  /// The candidates are flagged by the behavior itself.
  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const {}

  /// Whether the result depends on the candidate perception. If not, it is computed only once per decision update.
  virtual bool UsesCandidate() const { return true; }

  /// Returns the raw value, which Evaluate() then maps through the input range and the curve.
  ///
  /// pCandidate may be null or of an unexpected type, if the behavior has no candidates or the graph connects
  /// the consideration to a behavior with other perception types.
  virtual float GetInputValue(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pCandidate) const = 0;

  /// The result from 0 to 1.
  float Evaluate(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pCandidate) const;

  /// Maps a raw value through the input range, the curve and the invert option.
  float ApplyResponse(float fValue) const;

  /// m_sName, or the type name without the 'ezAiConsideration' prefix.
  ezStringView GetDisplayName() const;

  /// Returns false, if the consideration doesn't need instance data. Then pInstanceData is null in all calls.
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const { return false; }
};

/// Reflects the input range, curve and invert option of ezAiConsideration with the defaults of one concrete consideration type.
///
/// Put this into the property list of every concrete consideration. The defaults only take effect through reflection
/// (in the editor, or with ezReflectionUtils::SetAllMemberPropertiesToDefault()), the constructors don't set them.
#define EZ_AI_CONSIDERATION_INPUT_PROPERTIES(fDefaultMin, fDefaultMax, DefaultCurve, bDefaultInvert)                                     \
  EZ_ACCESSOR_PROPERTY("InputMin", GetInputMin, SetInputMin)->AddAttributes(new ezDefaultValueAttribute(fDefaultMin)),                   \
    EZ_ACCESSOR_PROPERTY("InputMax", GetInputMax, SetInputMax)->AddAttributes(new ezDefaultValueAttribute(fDefaultMax)),                 \
    EZ_ENUM_ACCESSOR_PROPERTY("Curve", ezAiResponseCurve, GetCurve, SetCurve)->AddAttributes(new ezDefaultValueAttribute(DefaultCurve)), \
    EZ_ACCESSOR_PROPERTY("Invert", GetInvert, SetInvert)->AddAttributes(new ezDefaultValueAttribute(bDefaultInvert))
