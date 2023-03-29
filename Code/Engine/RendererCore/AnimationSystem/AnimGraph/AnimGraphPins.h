#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

#include <Foundation/Types/RefCounted.h>
#include <ozz/base/maths/soa_transform.h>

class ezAnimGraph;
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

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

protected:
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
  bool IsTriggered(ezAnimGraph& ref_graph) const;
  bool AreAllTriggered(ezAnimGraph& ref_graph) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphTriggerOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphTriggerOutputPin, ezAnimGraphOutputPin);

public:
  /// \brief Sets this output pin to the triggered or untriggered state for this frame.
  ///
  /// All pin states are reset before every graph update, so this only needs to be called
  /// when a pin should be set to the triggered state, but then it must be called every frame.
  void SetTriggered(ezAnimGraph& ref_graph, bool bTriggered);
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphNumberInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphNumberInputPin, ezAnimGraphInputPin);

public:
  double GetNumber(ezAnimGraph& ref_graph, double fFallback = 0.0) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphNumberOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphNumberOutputPin, ezAnimGraphOutputPin);

public:
  void SetNumber(ezAnimGraph& ref_graph, double value);
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphBoneWeightsInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoneWeightsInputPin, ezAnimGraphInputPin);

public:
  ezAnimGraphPinDataBoneWeights* GetWeights(ezAnimGraph& ref_graph) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphBoneWeightsOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoneWeightsOutputPin, ezAnimGraphOutputPin);

public:
  void SetWeights(ezAnimGraph& ref_graph, ezAnimGraphPinDataBoneWeights* pWeights);
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseInputPin, ezAnimGraphInputPin);

public:
  ezAnimGraphPinDataLocalTransforms* GetPose(ezAnimGraph& ref_graph) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseMultiInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseMultiInputPin, ezAnimGraphInputPin);

public:
  void GetPoses(ezAnimGraph& ref_graph, ezDynamicArray<ezAnimGraphPinDataLocalTransforms*>& out_poses) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseOutputPin, ezAnimGraphOutputPin);

public:
  void SetPose(ezAnimGraph& ref_graph, ezAnimGraphPinDataLocalTransforms* pPose);
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphModelPoseInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphModelPoseInputPin, ezAnimGraphInputPin);

public:
  ezAnimGraphPinDataModelTransforms* GetPose(ezAnimGraph& ref_graph) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphModelPoseOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphModelPoseOutputPin, ezAnimGraphOutputPin);

public:
  void SetPose(ezAnimGraph& ref_graph, ezAnimGraphPinDataModelTransforms* pPose);
};
