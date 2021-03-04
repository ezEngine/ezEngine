#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/RendererCoreDLL.h>

class ezAnimGraph;
class ezStreamWriter;
class ezStreamReader;
struct ezAnimGraphBlendWeights;



class EZ_RENDERERCORE_DLL ezAnimGraphPin : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphPin, ezReflectedClass);

  enum Type : ezUInt8
  {
    Invalid,
    Trigger,
    Number,
    SkeletonWeights,
    // EXTEND THIS if a new type is introduced

    ENUM_COUNT
  };

  bool IsConnected() const
  {
    return m_iPinIndex != -1;
  }

public:
  ezInt16 m_iPinIndex = -1;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
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
};

class EZ_RENDERERCORE_DLL ezAnimGraphTriggerOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphTriggerOutputPin, ezAnimGraphOutputPin);

public:
  void SetTriggered(ezAnimGraph& controller, bool triggered);

private:
  bool m_bTriggered = false;
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

class EZ_RENDERERCORE_DLL ezAnimGraphSkeletonWeightsInputPin : public ezAnimGraphInputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphSkeletonWeightsInputPin, ezAnimGraphInputPin);

public:
  ezAnimGraphBlendWeights* GetWeights(ezAnimGraph& controller) const;
};

class EZ_RENDERERCORE_DLL ezAnimGraphSkeletonWeightsOutputPin : public ezAnimGraphOutputPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphSkeletonWeightsOutputPin, ezAnimGraphOutputPin);

public:
  void SetWeights(ezAnimGraph& controller, ezAnimGraphBlendWeights* pWeights);
};
