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
    m_func = func;
    m_param = param;
    ConfigureTask(szTaskName, ezTaskNesting::Never);
  }

private:
  virtual void Execute() override { m_func(m_param); }

  FunctionType m_func;
  T m_param;
};

template <>
class ezDelegateTask<void> final : public ezTask
{
public:
  typedef ezDelegate<void()> FunctionType;

  ezDelegateTask(const char* szTaskName, FunctionType func)
  {
    m_func = func;
    ConfigureTask(szTaskName, ezTaskNesting::Never);
  }

private:
  virtual void Execute() override { m_func(); }

  FunctionType m_func;
};
