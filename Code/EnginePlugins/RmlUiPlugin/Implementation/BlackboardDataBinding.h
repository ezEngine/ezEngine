#pragma once

#include <RmlUiPlugin/RmlUiDataBinding.h>

class ezBlackboard;

namespace ezRmlUiInternal
{
  class BlackboardDataBinding final : public ezRmlUiDataBinding
  {
  public:
    BlackboardDataBinding(ezBlackboard& blackboard, const char* szModelName);
    ~BlackboardDataBinding();

    virtual ezResult Setup(Rml::Context& context) override;
    virtual void Update() override;

  private:
    ezBlackboard& m_Blackboard;
  };
} // namespace ezRmlUiInternal
