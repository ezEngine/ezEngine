#pragma once

#include <Foundation/Utilities/Progress.h>

class ezWorld;

class ezBakingInterface
{
public:
  /// \brief Renders a debug view of the baking scene
  virtual ezResult RenderDebugView(const ezWorld& world, const ezMat4& InverseViewProjection, ezUInt32 uiWidth, ezUInt32 uiHeight, ezDynamicArray<ezColorGammaUB>& out_Pixels, ezProgress& progress) const = 0;
};
