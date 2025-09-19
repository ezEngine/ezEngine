#pragma once

#include <Foundation/Threading/TaskSystem.h>

/// \brief Convenience task wrapper that executes delegate functions with parameters
///
/// Provides an easy way to wrap function calls (delegates) as tasks for the task system.
/// The template parameter T specifies the type of data passed to the delegate function.
/// Use the void specialization for parameterless functions. This eliminates the need
/// to manually derive from ezTask for simple function execution scenarios.
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
