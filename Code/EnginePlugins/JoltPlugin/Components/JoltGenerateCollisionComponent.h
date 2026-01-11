#pragma once

#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

#include <Core/World/World.h>

struct ezMsgGenerateSplineMeshCollision;
struct ezMsgComponentInternalTrigger;
class ezAbstractObjectNode;
using ezMeshResourceHandle = ezTypedResourceHandle<class ezMeshResource>;

struct EZ_JOLTPLUGIN_DLL ezJoltMeshMapping
{
  ezMeshResourceHandle m_hRenderMesh;
  ezJoltMeshResourceHandle m_hCollisionMesh;

  bool operator==(const ezJoltMeshMapping& other) const
  {
    return m_hRenderMesh == other.m_hRenderMesh && m_hCollisionMesh == other.m_hCollisionMesh;
  }

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltMeshMapping);

//////////////////////////////////////////////////////////////////////////

using ezJoltGenerateCollisionComponentManager = ezComponentManager<class ezJoltGenerateCollisionComponent, ezBlockStorageType::Compact>;

/// \brief TODO
class EZ_JOLTPLUGIN_DLL ezJoltGenerateCollisionComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltGenerateCollisionComponent, ezComponent, ezJoltGenerateCollisionComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltGenerateCollisionComponent

public:
  ezJoltGenerateCollisionComponent();
  ~ezJoltGenerateCollisionComponent();

  ezArrayPtr<const ezJoltMeshMapping> GetMeshMappings() const { return m_MeshMappings; }                         // [ property ]

private:
  ezUInt32 Reflection_GetMeshMappingCount() const { return m_MeshMappings.GetCount(); }                          // [ property ]
  const ezJoltMeshMapping& Reflection_GetMeshMapping(ezUInt32 uiIndex) const { return m_MeshMappings[uiIndex]; } // [ property ]
  void Reflection_SetMeshMapping(ezUInt32 uiIndex, const ezJoltMeshMapping& mapping);                            // [ property ]
  void Reflection_InsertMeshMapping(ezUInt32 uiIndex, const ezJoltMeshMapping& mapping);                         // [ property ]
  void Reflection_RemoveMeshMapping(ezUInt32 uiIndex);                                                           // [ property ]

  ezCpuMeshResourceHandle GetCollisionCpuMeshForRenderMesh(ezMeshResourceHandle hRenderMesh) const;

  void OnObjectCreated(const ezAbstractObjectNode& node);
  void OnMsgGenerateSplineMeshCollision(ezMsgGenerateSplineMeshCollision& ref_msg); // [ msg handler ]
  void OnMsgComponentInternalTrigger(ezMsgComponentInternalTrigger& ref_msg);       // [ msg handler ]

  void StartGenerateTask(ezSharedPtr<ezTask>&& pTask);

  friend class ezSceneExportModifier_JoltFinalizeGeneratedCollision;
  void FinalizeGeneration();

  ezSmallArray<ezJoltMeshMapping, 1> m_MeshMappings;

  ezUInt64 m_uiStableId = 0;
  ezHashedString m_sCollisionMeshPath;

  ezSharedPtr<ezTask> m_pGenerationTask;
  ezSharedPtr<ezTask> m_pNextGenerationTask;
  ezTaskGroupID m_TaskGroupID;
};
