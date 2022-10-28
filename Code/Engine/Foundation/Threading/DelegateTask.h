#pragma once

#include <Foundation/Threading/TaskSystem.h>

/// \brief A simple task implementation that calls a delegate function.
template <typename T>
class ezDelegateTask final : public ezTask
{
public:
  typedef ezDelegate<void(const T&)> FunctionType;

  ezDelegateTask(const char* szTaskName, FunctionType func, const T& param)
  {
    m_Func = func;
    m_param = param;
    ConfigureTask(szTaskName, ezTaskNesting::Never);
  }

private:
  virtual void Execute() override { m_Func(m_param); }

  FunctionType m_Func;
  T m_param;
};

template <>
class ezDelegateTask<void> final : public ezTask
{
public:
  typedef ezDelegate<void()> FunctionType;

  ezDelegateTask(const char* szTaskName, FunctionType func)
  {
    m_Func = func;
    ConfigureTask(szTaskName, ezTaskNesting::Never);
  }

private:
  virtual void Execute() override { m_Func(); }

  FunctionType m_Func;
};
