#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

#include <Foundation/Types/RefCounted.h>
#include <ozz/base/maths/soa_transform.h>

class ezAnimGraphInstance;
class ezStreamWriter;
class ezStreamReader;
struct ezAnimGraphPinDataBoneWeights;
struct ezAnimGraphPinDataLocalTransforms;
struct ezAnimGraphPinDataModelTransforms;

struct ezAnimGraphSharedBoneWeights : public ezRefCounted
{
  ezDynamicArray<ozz::math::SimdFloat4, ezAlignedAllocatorWrapper> m_Weights;
};

using ezAnimPoseGeneratorLocalPoseID = ezUInt32;
using ezAnimPoseGeneratorModelPoseID = ezUInt32;
using ezAnimPoseGeneratorCommandID = ezUInt32;

class EZ_RENDERERCORE_DLL ezAnimGraphPin : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphPin, ezReflectedClass);

public:
  enum Type : ezUInt8
  {
    Invalid,
    Trigger,
    Number,
    Bool,
    BoneWeights,
    LocalPose,
    ModelPose,
    // EXTEND THIS if a new type is introduced

    ENUM_COUNT
  };

  bool IsConnected() const
  {
    return m_iPinIndex != -1;
  }

  virtual ezAnimGraphPin::Type GetPinType() const = 0;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

protected:
  friend class ezAnimGraph;

  ezInt16 m_iPinIndex = -1;
  ezUInt8 m_uiNumConnections = 0;
};

class EZ_RENDERERCORE_DLL ezAnimGraphInputPin : public ezAnimGraphPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphInputPin, ezAnimGraphPin);

public:
};

class EZ_RENDERERCORE_DLL ezAnimGraphOutputPin : public ezAnimGraphPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphOutputPin, ezAnimGraphPin);

public:
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphTriggerInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphTriggerInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Trigger; }

  bool IsTriggered(ezAnimGraphInstance& ref_graph) const;
  bool AreAllTriggered(ezAnimGraphInstance& ref_graph) const;
};

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

class EZ_RENDERERCORE_DLL ezAnimGraphNumberInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphNumberInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Number; }

  double GetNumber(ezAnimGraphInstance& ref_graph, double fFallback = 0.0) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphNumberOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphNumberOutputPin, ezAnimGraphOutputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Number; }

  void SetNumber(ezAnimGraphInstance& ref_graph, double value) const;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphBoolInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoolInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Bool; }

  bool GetBool(ezAnimGraphInstance& ref_graph, bool bFallback = false) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphBoolOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoolOutputPin, ezAnimGraphOutputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::Bool; }

  void SetBool(ezAnimGraphInstance& ref_graph, bool bValue) const;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphBoneWeightsInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoneWeightsInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::BoneWeights; }

  ezAnimGraphPinDataBoneWeights* GetWeights(ezAnimGraphInstance& ref_graph) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphBoneWeightsOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoneWeightsOutputPin, ezAnimGraphOutputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::BoneWeights; }

  void SetWeights(ezAnimGraphInstance& ref_graph, ezAnimGraphPinDataBoneWeights* pWeights) const;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::LocalPose; }

  ezAnimGraphPinDataLocalTransforms* GetPose(ezAnimGraphInstance& ref_graph) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseMultiInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseMultiInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::LocalPose; }

  void GetPoses(ezAnimGraphInstance& ref_graph, ezDynamicArray<ezAnimGraphPinDataLocalTransforms*>& out_poses) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseOutputPin, ezAnimGraphOutputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::LocalPose; }

  void SetPose(ezAnimGraphInstance& ref_graph, ezAnimGraphPinDataLocalTransforms* pPose) const;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphModelPoseInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphModelPoseInputPin, ezAnimGraphInputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::ModelPose; }

  ezAnimGraphPinDataModelTransforms* GetPose(ezAnimGraphInstance& ref_graph) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphModelPoseOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphModelPoseOutputPin, ezAnimGraphOutputPin);

public:
  virtual ezAnimGraphPin::Type GetPinType() const override { return ezAnimGraphPin::ModelPose; }

  void SetPose(ezAnimGraphInstance& ref_graph, ezAnimGraphPinDataModelTransforms* pPose) const;
};
