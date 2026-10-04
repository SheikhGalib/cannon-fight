#version 330 core

// A deliberately UNLIT fragment shader, used only by RenderDocShots.cpp to
// produce the "before lighting" picture in docs/code-walkthrough.md. It shows
// what every shape looks like when the surface normal is ignored: a flat
// silhouette with no sense of depth. lit.frag is the real one.

in vec3 fragNormal;
in vec3 fragColor;

out vec4 FragColor;

uniform vec3 lightDir; // accepted but ignored, so the same C++ code can drive both

void main()
{
   FragColor = vec4(fragColor, 1.0);
}
