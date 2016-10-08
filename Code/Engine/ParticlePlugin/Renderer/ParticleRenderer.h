#pragma once

#include <ParticlePlugin/Basics.h>
#include <RendererCore/Meshes/MeshRenderer.h>


/// \brief Implements rendering of particle systems
class EZ_PARTICLEPLUGIN_DLL ezParticleRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezParticleRenderer);

public:
  ezParticleRenderer();
  ~ezParticleRenderer();

protected:

};
