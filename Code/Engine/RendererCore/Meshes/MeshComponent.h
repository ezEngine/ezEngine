#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

struct ezMsgExtractGeometry;
using ezMeshComponentManager = ezComponentManager<class ezMeshComponent, ezBlockStorageType::Compact>;

class EZ_RENDERERCORE_DLL ezMeshComponent : public ezMeshComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezMeshComponent, ezMeshComponentBase, ezMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezMeshComponent

public:
  ezMeshComponent();
  ~ezMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const; // [ msg handler ]
};
