#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

class ezAnimGraphInstance;
class ezAnimGraphNode;

//////////////////////////////////////////////////////////////////////////

using ezAnimGraphResourceHandle = ezTypedResourceHandle<class ezAnimGraphResource>;

/// Maps a logical animation clip name to an actual animation clip resource.
///
/// Animation graphs reference clips by name (e.g., "Walk", "Jump") rather than directly referencing
/// resources. This allows the same graph to be used with different animation sets by remapping the clip
/// names to different resources.
///
/// This indirection allows:
/// - Same graph with different animation sets (e.g., male/female characters)
/// - Runtime clip swapping for character customization
/// - Graph reuse across different character types
struct EZ_RENDERERCORE_DLL ezAnimationClipMapping : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipMapping, ezReflectedClass);

  ezHashedString m_sClipName;
  ezAnimationClipResourceHandle m_hClip;

  const char* GetClipName() const { return m_sClipName.GetData(); }
  void SetClipName(const char* szName) { m_sClipName.Assign(szName); }
};

/// Resource containing an animation graph definition (ezAnimGraph) and its animation clip mappings (ezAnimationClipMapping).
///
/// ## Content
///
/// - **Animation Graph**: The node graph structure (ezAnimGraph)
/// - **Clip Mappings**: Maps logical names like "Walk" to actual animation clip resources
/// - **Include Graphs**: References to other graph resources to compose larger graphs
///
/// ## Usage Pattern
///
/// Resources are loaded via the resource manager and shared across multiple characters:
///
/// ```cpp
/// ezAnimGraphResourceHandle hGraph = ezResourceManager::LoadResource<ezAnimGraphResource>("Character.ezAnimGraph");
/// controller.AddAnimGraph(hGraph);
/// ```
class EZ_RENDERERCORE_DLL ezAnimGraphResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimGraphResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezAnimGraphResource);

public:
  ezAnimGraphResource();
  ~ezAnimGraphResource();

  const ezAnimGraph& GetAnimationGraph() const { return m_AnimGraph; }

  /// References to other animation graph resources that should be included.
  ///
  /// Used to compose complex graphs from smaller reusable pieces.
  ezArrayPtr<const ezString> GetIncludeGraphs() const { return m_IncludeGraphs; }

  /// Default mappings from clip names to animation resources.
  ///
  /// These can be overridden at runtime via ezAnimController::SetAnimationClipInfo().
  const ezDynamicArray<ezAnimationClipMapping>& GetAnimationClipMapping() const { return m_AnimationClipMapping; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezDynamicArray<ezString> m_IncludeGraphs;
  ezDynamicArray<ezAnimationClipMapping> m_AnimationClipMapping;
  ezAnimGraph m_AnimGraph;
};
