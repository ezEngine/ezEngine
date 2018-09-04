#pragma once

#include <RendererCore/Meshes/RenderMeshComponent.h>

struct ezMsgExtractGeometry;
typedef ezComponentManager<class ezMeshComponent, ezBlockStorageType::Compact> ezMeshComponentManager;

class EZ_RENDERERCORE_DLL ezMeshComponent : public ezRenderMeshComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezMeshComponent, ezRenderMeshComponent, ezMeshComponentManager);

public:
  ezMeshComponent();
  ~ezMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnExtractGeometry(ezMsgExtractGeometry& msg);
};
