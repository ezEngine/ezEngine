using System;
using System.Diagnostics;
using CommandLine;

namespace ezUwpTestHarness
{

  class Program
  {
    public class Options
    {
      [Option('w', "workspace", Required = true, Default = "", HelpText = "")]
      public string ezWorkspace { get; set; }
      [Option('o', "output", Required = true, Default = "", HelpText = "")]
      public string testOutputPath { get; set; }
      [Option('p', "project", Required = false, Default = "FoundationTest", HelpText = "")]
      public string project { get; set; }

      [Option('c', "configuration", Required = false, Default = "Dev", HelpText = "")]
      public string configuration { get; set; }
      [Option("platform", Required = false, Default = "x64", HelpText = "")]
      public string platform { get; set; }
    }

    static void Main(string[] args)
    {
      if (!(Microsoft.Diagnostics.Tracing.Session.TraceEventSession.IsElevated() ?? false))
      {
        Console.Out.WriteLine("To turn on ETW events you need to be Administrator, please run from an Admin process.");
        Debugger.Break();
      }

      AppDomain.CurrentDomain.UnhandledException += (object sender, UnhandledExceptionEventArgs eventArgs) =>
      {
        Environment.ExitCode = 1;
        Console.WriteLine(string.Format("Failed to run test. Exception: '{0}'", eventArgs.ExceptionObject.ToString()));
        Console.Out.Flush();
        try
        {
          string args2 = string.Format("-accepteula -ma {0}", Process.GetCurrentProcess().Id);
          string absBinDir = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location);
          var bla = ezProcessHelper.RunExternalExe("procdump", args2, absBinDir);
        }
        catch (Exception e)
        {
          Console.WriteLine(string.Format("Failed to run procdump: '{0}'", e.ToString()));
        }
      };

      Parser.Default.ParseArguments<Options>(args)
        .WithParsed<Options>(o =>
        {
          ezTestUWP test = new ezTestUWP(o.ezWorkspace, o.testOutputPath, o.configuration, o.platform, o.project);
          test.ExecuteTest();
        });
    }

  }
}
