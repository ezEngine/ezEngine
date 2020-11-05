#include <RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>

namespace ezRmlUiInternal
{
  BlackboardDataBinding::BlackboardDataBinding(ezBlackboard& blackboard, const char* szModelName)
    : m_Blackboard(blackboard)
  {
  }

  BlackboardDataBinding::~BlackboardDataBinding() = default;

  ezResult BlackboardDataBinding::Setup(Rml::Context& context)
  {
    return EZ_SUCCESS;
  }

  void BlackboardDataBinding::Update()
  {
  }

} // namespace ezRmlUiInternal
