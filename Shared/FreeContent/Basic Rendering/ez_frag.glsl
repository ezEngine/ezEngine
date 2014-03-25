#version 430

layout(location = 0) in vec3 inNorm;
layout(location = 1) in vec2 inTex0;


layout(location = 0, index = 0) out vec4 outFragColor;

void main()
{
  outFragColor = vec4(inTex0, 0.0, 1.0);
}
