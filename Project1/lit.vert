#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out vec3 fragPos;
out vec3 fragNormal;
out vec3 fragColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
   vec4 worldPos = model * vec4(aPos, 1.0);
   fragPos = worldPos.xyz;
   gl_Position = proj * view * worldPos;

   fragNormal = mat3(model) * aNormal;
   fragColor = aColor;
}

