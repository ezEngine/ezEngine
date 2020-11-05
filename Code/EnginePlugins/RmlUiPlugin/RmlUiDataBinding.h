#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

class EZ_RMLUIPLUGIN_DLL ezRmlUiDataBinding
{
public:
  virtual ~ezRmlUiDataBinding() {}

  virtual ezResult Setup(Rml::Context& context) = 0;

  virtual void Update() = 0;
};
