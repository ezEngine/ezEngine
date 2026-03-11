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

/// Base class for all nodes in an ezAnimGraph
///
/// Animation graph nodes implement different operations in the animation system such as sampling clips,
/// blending poses, logic operations, or even outputting debug information.
///
/// Nodes define:
/// - Input and output pins (typed connections to other nodes)
/// - Behavior in the Step() method (called each frame during graph evaluation)
/// - Optional per-instance state data (playback time, blend weights, etc.)
///
/// Nodes that need to store state across frames (like playback time or transition progress) should use
/// the instance data pattern. This allows one graph definition (ezAnimGraph) to be shared by many
/// runtime instances (ezAnimGraphInstance), where each instance has its own state data.
///
/// The same ezAnimGraph can be used by hundreds of characters, each with their own
/// ezAnimGraphInstance and instance data. The graph definition (nodes, connections, pins) is shared
/// to minimize memory overhead.
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
