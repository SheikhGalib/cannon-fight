#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out vec3 vertColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 lightDir;

void main()
{
   gl_Position = proj * view * model * vec4(aPos, 1.0);

   // Phase 8: Gouraud shading - compute the per-vertex diffuse term
   // here (in the vertex shader) instead of per-fragment.  The lit
   // colour is interpolated across the triangle, so each pixel gets
   // a smooth blend of the vertex colours rather than a per-pixel
   // dot product.
   vec3 normal = normalize(mat3(model) * aNormal);
   float diffuse = max(dot(normal, -lightDir), 0.0);
   float brightness = 0.35 + 0.65 * diffuse;
   vertColor = aColor * brightness;
}