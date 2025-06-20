#pragma once

#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>
#include <RendererCore/Declarations.h>

struct ezMsgExtractRenderData;

struct EZ_GAMECOMPONENTS_DLL ezMeshDecalDescription
{
  ezUInt16 m_uiIndex = 0;
  ezTexture2DResourceHandle m_hBaseColorTexture;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMECOMPONENTS_DLL, ezMeshDecalDescription);

////////////////////////////////////////////////////////////////////////////

using ezMeshDecalComponentManager = ezComponentManager<class ezMeshDecalComponent, ezBlockStorageType::Compact>;

/// \brief A component that takes a couple of decal textures, picks a random one for each slot,
/// adds them to runtime decal atlas and sends a custom data message with the corresponding decal indices.
///
/// The decals in this case are not regular projected decals, but rather mesh decals aka floaters.
/// This requires a special shader to be used which uses the custom data in the instance data to map the UV coordinates to the decal atlas.
/// See "Data/Samples/Testing Chambers/Materials/MeshDecalMaterial.ezShader" for an example shader.
class EZ_GAMECOMPONENTS_DLL ezMeshDecalComponent final : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezMeshDecalComponent, ezComponent, ezMeshDecalComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

private:
  ezUInt32 Decals_GetCount() const;
  const ezMeshDecalDescription& Decals_Get(ezUInt32 uiIndex) const;
  void Decals_Set(ezUInt32 uiIndex, const ezMeshDecalDescription& desc);
  void Decals_Insert(ezUInt32 uiIndex, const ezMeshDecalDescription& desc);
  void Decals_Remove(ezUInt32 uiIndex);

  void UpdateDecals();
  void DeleteDecals();

  ezSmallArray<ezMeshDecalDescription, 2> m_DecalDescs;
  ezSmallArray<ezDecalId, 2> m_DecalIds;
};
