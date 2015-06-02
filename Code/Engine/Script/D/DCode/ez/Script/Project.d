module ez.Script.Project;
import ez.Script.Reflection.Reflection;
import ez.Foundation.Logging.Log;
import std.process;

static import std.file;

class Project
{
private:
  static struct Module
  {
    string relativePath;
    IReflectedModule reflection;
  }

  Module[] m_modules;
  string m_rootPath;
  string m_ezLibPath;
  string m_ezCodePath;
  string m_compilerPath;
public:

  this(string rootPath, string ezLibPath, string ezCodePath)
  {
    m_rootPath = rootPath;
    m_ezLibPath = ezLibPath;
    m_ezCodePath = ezCodePath;

    string dmdPath = environment.get("EZ_DMD_PATH", null);
    if(dmdPath.length == 0)
    {
      throw new Exception("EZ_DMD_PATH is not set");
    }

    m_compilerPath = dmdPath ~ "\\windows\\bin\\dmd.exe";
    if(!std.file.exists(m_compilerPath))
    {
      throw new Exception("The D-Compiler could not be found in " ~ m_compilerPath);
    }
  }

  void loadModule(string relativePath)
  {
    // %EZ_DMD_PATH%\windows\bin\dmd.exe test.d dllmain.d -debug -shared -gc -m64 -of"test.dll" -defaultlib="druntime64s.lib" phobos64ds.lib -L/NODEFAULTLIB:libcmt -Lmsvcrt.lib -I..\..\..\Code\Engine\Script\D\Dcode ..\..\..\Output\Lib\WinVs2013Debug64\ezDScript.lib -L/ignore:4217
    auto command = [m_compilerPath, relativePath, "\"" ~ m_ezLibPath ~ "\\ezScriptHook.lib\"", "-debug", "-shared", "-gc", "-m64", "-of\"bin/" ~ relativePath ~ "\"", 
    "-defaultlib=\"druntime64ds.lib\"", "phobos64ds.lib", "-L/NODEFAULTLIB:libcmt", "-Lmsvcrt.lib", "-L/ignore:4217",
    "-I\"" ~ m_ezCodePath ~ "\\Engine\\Script\\D\\Dcode\"", "\"" ~ m_ezLibPath ~ "\\ezDScript.lib\""];
    auto result = execute(command, null, Config.none, size_t.max, m_rootPath);
    if(result.status != 0)
    {
      ezLog.Error("Compiling module '%.*s' failed with:\n.*%s", relativePath.length, relativePath.ptr, result.output.length, result.output.ptr);
    }
  }
}