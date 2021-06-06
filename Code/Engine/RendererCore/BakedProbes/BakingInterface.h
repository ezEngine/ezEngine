#pragma once

#include <Foundation/Basics.h>

class ezBakingInterface
{
public:
  /// \brief Renders a debug view of the baking scene
  virtual ezResult RenderDebugView(const ezMat4& InverseViewProjection, ezUInt32 uiWidth, ezUInt32 uiHeight, ezDynamicArray<ezColorGammaUB>& out_Pixels,
    ezProgress& progress) const = 0;
};
