#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

#include <Foundation/Types/RefCounted.h>
#include <ozz/base/maths/soa_transform.h>

class ezAnimGraphInstance;
class ezAnimController;
class ezStreamWriter;
class ezStreamReader;
struct ezAnimGraphPinDataBoneWeights;
struct ezAnimGraphPinDataLocalTransforms;
struct ezAnimGraphPinDataModelTransforms;

/// Shared bone weight mask used to control which bones are affected by animations.
///
/// Bone weights are globally shared across all characters to save memory. Created via
/// ezAnimController::CreateBoneWeights() with a unique name for caching.
struct ezAnimGraphSharedBoneWeights : public ezRefCounted
{
  ezDynamicArray<ozz::math::SimdFloat4, ezAlignedAllocatorWrapper> m_Weights;
};

using ezAnimPoseGeneratorLocalPoseID = ezUInt32;
using ezAnimPoseGeneratorModelPoseID = ezUInt32;
using ezAnimPoseGeneratorCommandID = ezUInt32;

/// Base class for all animation graph pins, representing typed connections between nodes.
///
/// Pins are the connection points on nodes that allow data to flow through the graph. Each pin has
/// a type (trigger, number, bool, pose, etc.) and can be either an input or output. Pins are connected
/// in the graph editor to define data flow.
///
/// ## Pin Indices
///
/// During graph preparation, each pin is assigned an index (m_iPinIndex) that identifies its storage
/// location in the instance data arrays. Unconnected pins have index -1 and return default values.
///
/// ## Data Flow
///
/// Output pins write values to the instance's pin state arrays, input pins read from those arrays.
/// The mapping from output pin indices to input pin indices is pre-computed during PrepareForUse().
///
/// ## Pin Types
///
/// - **Trigger**: One-shot events (animation finished, state entered)
/// - **Number**: Floating-point values for blend weights, speeds, parameters
/// - **Bool**: Boolean flags and conditions
/// - **BoneWeights**: Masks controlling which bones are affected by animations
/// - **LocalPose**: Bone transforms in local space (parent-relative)
/// - **ModelPose**: Bone transforms in model space (skeleton root-relative)
class EZ_RENDERERCORE_DLL ezAnimGraphPin : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphPin, ezReflectedClass);

public:
  enum Type : ezUInt8
  {
    Invalid,
    Trigger,     ///< One-shot events that occur for a single frame
    Number,      ///< Double-precision floating-point values
    Bool,        ///< Boolean true/false values
    BoneWeights, ///< Masks controlling bone influence
    LocalPose,   ///< Bone transforms in local space
    ModelPose,   ///< Bone transforms in model space
    // EXTEND THIS if a new type is introduced

    ENUM_COUNT
  };

  /// Returns whether this pin is connected to another pin.
  ///
  /// Unconnected pins have index -1 and return default values when queried.
  bool IsConnected() const
  {
    return m_iPinIndex != -1;
  }

  virtual ezAnimGraphPin::Type GetPinType() const = 0;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

protected:
  friend class ezAnimGraph;

  ezInt16 m_iPinIndex = -1;       ///< Index into the instance's pin state array, -1 if unconnected
  ezUInt8 m_uiNumConnections = 0; ///< Number of connections to this pin
};

/// Base class for input pins that receive data from connected output pins.
class EZ_RENDERERCORE_DLL ezAnimGraphInputPin : public ezAnimGraphPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphInputPin, ezAnimGraphPin);

public:
};

/// Base class for output pins that send data to connected input pins.
class EZ_RENDERERCORE_DLL ezAnimGraphOutputPin : public ezAnimGraphPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphOutputPin, ezAnimGraphPin);

public:
};

//////////////////////////////////////////////////////////////////////////

/// Input pin for receiving trigger events.
///
/// Triggers are one-shot events that are active for a single frame. Common uses include signaling
/// animation completion, state transitions, or specific animation events.
class EZ_RENDERERCORE_DLL ezAnimGraphTriggerInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphTriggerInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Trigger; }

  /// Returns whether at least one connected output pin triggered this frame.
  bool IsTriggered(ezAnimGraphInstance& ref_graph) const;

  /// Returns whether all connected output pins triggered this frame.
  ///
  /// Useful for nodes that need all inputs to be ready before proceeding.
  bool AreAllTriggered(ezAnimGraphInstance& ref_graph) const;
};

/// Output pin for sending trigger events.
///
/// Trigger pins send one-shot events that last for a single frame.
class EZ_RENDERERCORE_DLL ezAnimGraphTriggerOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphTriggerOutputPin, ezAnimGraphOutputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Trigger; }

  /// \brief Sets this output pin to the triggered state for this frame.
  ///
  /// All pin states are reset before every graph update, so this only needs to be called
  /// when a pin should be set to the triggered state, but then it must be called every frame.
  void SetTriggered(ezAnimGraphInstance& ref_graph) const;
};

//////////////////////////////////////////////////////////////////////////

/// Input pin for receiving number values.
///
/// Number pins carry double-precision floating-point values used for blend weights, animation speeds,
/// parameters, and other numerical data.
class EZ_RENDERERCORE_DLL ezAnimGraphNumberInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphNumberInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Number; }

  /// Retrieves the number value from connected output pins.
  ///
  /// Returns fFallback if the pin is not connected.
  double GetNumber(ezAnimGraphInstance& ref_graph, double fFallback = 0.0) const;
};

/// Output pin for sending number values.
class EZ_RENDERERCORE_DLL ezAnimGraphNumberOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphNumberOutputPin, ezAnimGraphOutputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Number; }

  void SetNumber(ezAnimGraphInstance& ref_graph, double value) const;
};

//////////////////////////////////////////////////////////////////////////

/// Input pin for receiving boolean values.
///
/// Bool pins carry true/false values used for conditions, flags, and logical operations.
class EZ_RENDERERCORE_DLL ezAnimGraphBoolInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoolInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Bool; }

  /// Retrieves the boolean value from connected output pins.
  ///
  /// Returns bFallback if the pin is not connected.
  bool GetBool(ezAnimGraphInstance& ref_graph, bool bFallback = false) const;
};

/// Output pin for sending boolean values.
class EZ_RENDERERCORE_DLL ezAnimGraphBoolOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoolOutputPin, ezAnimGraphOutputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Bool; }

  void SetBool(ezAnimGraphInstance& ref_graph, bool bValue) const;
};

//////////////////////////////////////////////////////////////////////////

/// Input pin for receiving bone weight masks.
///
/// Bone weight pins carry masks that control which bones are affected by animations, enabling
/// partial skeleton animations (e.g., upper body only, lower body only).
class EZ_RENDERERCORE_DLL ezAnimGraphBoneWeightsInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoneWeightsInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::BoneWeights; }

  ezAnimGraphPinDataBoneWeights* GetWeights(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph) const;
};

/// Output pin for sending bone weight masks.
class EZ_RENDERERCORE_DLL ezAnimGraphBoneWeightsOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoneWeightsOutputPin, ezAnimGraphOutputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::BoneWeights; }

  void SetWeights(ezAnimGraphInstance& ref_graph, ezAnimGraphPinDataBoneWeights* pWeights) const;
};

//////////////////////////////////////////////////////////////////////////

/// Input pin for receiving a single local pose.
///
/// Local pose pins carry bone transforms in local space (relative to parent bone). This is the
/// output of animation sampling and blending before forward kinematics is applied.
class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::LocalPose; }

  ezAnimGraphPinDataLocalTransforms* GetPose(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph) const;
};

/// Output pin for sending local poses.
class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseOutputPin, ezAnimGraphOutputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::LocalPose; }

  void SetPose(ezAnimGraphInstance& ref_graph, ezAnimGraphPinDataLocalTransforms* pPose) const;
};
