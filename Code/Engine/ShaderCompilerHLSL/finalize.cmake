# Make sure this project is built when the Editor is built
ez_add_as_runtime_dependency(ShaderCompilerHLSL)

ez_add_dependency("ShaderCompiler" "ShaderCompilerHLSL")