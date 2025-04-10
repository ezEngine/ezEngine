#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

class ezSkeletonResource;
class ezGameObject;
class ezAnimGraphInstance;
class ezAnimController;
class ezStreamWriter;
class ezStreamReader;
struct ezAnimGraphPinDataLocalTransforms;
struct ezAnimGraphPinDataBoneWeights;
class ezAnimationClipResource;
struct ezInstanceDataDesc;

using ezAnimationClipResourceHandle = ezTypedResourceHandle<class ezAnimationClipResource>;

namespace ozz
{
  namespace animation
  {
    class Animation;
  }
} // namespace ozz

/// \brief Base class for all nodes in an ezAnimGraphInstance
///
/// These nodes are used to configure which skeletal animations can be played on an object,
/// and how they would be played back exactly.
/// The nodes implement different functionality. For example logic nodes are used to figure out how to play an animation,
/// other nodes then sample and combining animation poses, and yet other nodes can inform the user about events
/// or they write state back to the animation graph's blackboard.
class EZ_RENDERERCORE_DLL ezAnimGraphNode : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphNode, ezReflectedClass);

public:
  ezAnimGraphNode();
  virtual ~ezAnimGraphNode();

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

  const char* GetCustomNodeTitle() const { return m_sCustomNodeTitle.GetString(); }
  void SetCustomNodeTitle(const char* szSz) { m_sCustomNodeTitle.Assign(szSz); }

protected:
  friend class ezAnimGraphInstance;
  friend class ezAnimGraph;
  friend class ezAnimGraphResource;

  ezHashedString m_sCustomNodeTitle;
  ezUInt32 m_uiInstanceDataOffset = ezInvalidIndex;

  virtual ezResult SerializeNode(ezStreamWriter& stream) const = 0;
  virtual ezResult DeserializeNode(ezStreamReader& stream) = 0;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const = 0;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const { return false; }
};

