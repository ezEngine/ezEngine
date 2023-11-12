#pragma once

#include <Foundation/Threading/TaskSystem.h>

/// \brief A simple task implementation that calls a delegate function.
template <typename T>
class ezDelegateTask final : public ezTask
{
public:
  using FunctionType = ezDelegate<void(const T&)>;

  ezDelegateTask(const char* szTaskName, ezTaskNesting taskNesting, FunctionType func, const T& param)
  {
    m_Func = func;
    m_param = param;
    ConfigureTask(szTaskName, taskNesting);
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
  using FunctionType = ezDelegate<void()>;

  ezDelegateTask(const char* szTaskName, ezTaskNesting taskNesting, FunctionType func)
  {
    m_Func = func;
    ConfigureTask(szTaskName, taskNesting);
  }

private:
  virtual void Execute() override { m_Func(); }

  FunctionType m_Func;
};
