#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out vec3 fragNormal;
out vec3 fragColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
   gl_Position = proj * view * model * vec4(aPos, 1.0);

   // Move the normal into world space too, so lighting (done in
   // lit.frag) compares it against a world-space light direction.
   // mat3(model) - just the rotation/scale part of the model matrix -
   // is only correct because nothing here uses non-uniform scaling
   // (every Transform's `scale` stays 1,1,1); a true "normal matrix"
   // (transpose(inverse(mat3(model)))) would be needed the day that changes.
   fragNormal = mat3(model) * aNormal;
   fragColor = aColor;
}
