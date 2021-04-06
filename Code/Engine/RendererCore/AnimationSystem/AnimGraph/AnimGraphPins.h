#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>

class ezAnimGraph;
class ezStreamWriter;
class ezStreamReader;
struct ezAnimGraphBoneWeights;
struct ezAnimGraphLocalTransforms;
struct ezAnimGraphModelTransforms;

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

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

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
  bool IsTriggered(ezAnimGraph& controller) const;
  bool AreAllTriggered(ezAnimGraph& controller) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphTriggerOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphTriggerOutputPin, ezAnimGraphOutputPin);

public:
  /// \brief Sets this output pin to the triggered or untriggered state for this frame.
  ///
  /// All pin states are reset before every graph update, so this only needs to be called
  /// when a pin should be set to the triggered state, but then it must be called every frame.
  void SetTriggered(ezAnimGraph& controller, bool triggered);
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphNumberInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphNumberInputPin, ezAnimGraphInputPin);

public:
  double GetNumber(ezAnimGraph& controller) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphNumberOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphNumberOutputPin, ezAnimGraphOutputPin);

public:
  void SetNumber(ezAnimGraph& controller, double value);
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphBoneWeightsInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoneWeightsInputPin, ezAnimGraphInputPin);

public:
  ezAnimGraphBoneWeights* GetWeights(ezAnimGraph& controller) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphBoneWeightsOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphBoneWeightsOutputPin, ezAnimGraphOutputPin);

public:
  void SetWeights(ezAnimGraph& controller, ezAnimGraphBoneWeights* pWeights);
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseInputPin, ezAnimGraphInputPin);

public:
  ezAnimGraphLocalTransforms* GetPose(ezAnimGraph& controller) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseMultiInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseMultiInputPin, ezAnimGraphInputPin);

public:
  void GetPoses(ezAnimGraph& controller, ezDynamicArray<ezAnimGraphLocalTransforms*>& out_Poses) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphLocalPoseOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphLocalPoseOutputPin, ezAnimGraphOutputPin);

public:
  void SetPose(ezAnimGraph& controller, ezAnimGraphLocalTransforms* pPose);
};

//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezAnimGraphModelPoseInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphModelPoseInputPin, ezAnimGraphInputPin);

public:
  ezAnimGraphModelTransforms* GetPose(ezAnimGraph& controller) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphModelPoseOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphModelPoseOutputPin, ezAnimGraphOutputPin);

public:
  void SetPose(ezAnimGraph& controller, ezAnimGraphModelTransforms* pPose);
};
