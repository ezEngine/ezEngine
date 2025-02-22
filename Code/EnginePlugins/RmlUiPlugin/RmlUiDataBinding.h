#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

class EZ_RMLUIPLUGIN_DLL ezRmlUiDataBinding
{
public:
  virtual ~ezRmlUiDataBinding() = default;

  virtual ezResult Initialize(Rml::Context& ref_context) = 0;
  virtual void Deinitialize(Rml::Context& ref_context) = 0;

  /// \brief Returns true if anything was updated
  virtual bool Update() = 0;
};
