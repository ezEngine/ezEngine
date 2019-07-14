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

      [Option('c', "configuration", Required = false, Default = "RelWithDebInfo", HelpText = "")]
      public string configuration { get; set; }
      [Option("platform", Required = false, Default = "Win32", HelpText = "")]
      public string platform { get; set; }
    }

    static void Main(string[] args)
    {
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
