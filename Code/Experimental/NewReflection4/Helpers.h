#pragma once

class ezExecuteAtStartup
{
public:
  typedef void (*Function)();

  ezExecuteAtStartup(Function f)
  {
    f();
  }
};