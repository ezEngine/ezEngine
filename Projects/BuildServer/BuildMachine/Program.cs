using System;
using System.Collections.Generic;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using BuildShared;

namespace BuildMachine
{
  class Program
  {
    static void Main(string[] args)
    {
      if (args.Length < 1)
      {
        Console.WriteLine("No argument given. The build machine need the absolute path to the CMake workspace as the first argument to run.");
        return;
      }

      ezBuildMachine buildMachine = new ezBuildMachine(args[0]);
      if (!buildMachine.Init())
        return;

      buildMachine.Run();
    }
  }
}
