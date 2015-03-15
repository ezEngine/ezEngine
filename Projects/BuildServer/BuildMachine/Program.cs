using System;
using System.Collections.Generic;
using System.Net;
using System.Text;
using System.Linq;
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

      if (args.Any(x => x == "-test"))
      {
        BuildProcess process = new BuildProcess(args[args.Length - 1]);
        if (!process.Init())
          return;

        bool bRes = process.Run(-1, false);
        string sResults = process.GetJSON();
        System.IO.File.WriteAllText("test.json", sResults, Encoding.UTF8);
        return;
      }

      ezBuildMachine buildMachine = new ezBuildMachine(args[args.Length - 1]);
      if (!buildMachine.Init())
        return;

      buildMachine.Run();
    }
  }
}
