using System;
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
            Parser.Default.ParseArguments<Options>(args)
                   .WithParsed<Options>(o =>
                   {
                       try
                       {
                           ezTestUWP test = new ezTestUWP(o.ezWorkspace, o.testOutputPath, o.configuration, o.platform, o.project);
                           test.ExecuteTest();
                       }
                       catch (Exception e)
                       {
                           Environment.ExitCode = 1;
                           Console.WriteLine(string.Format("Failed to run test. Exception: '{0}'", e.ToString()));
                       }
                       Console.Out.Flush();
                   });
        }
    }
}
