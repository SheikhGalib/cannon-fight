#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out vec3 vertColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform bool isSun;

void main()
{
   vec4 worldPos = model * vec4(aPos, 1.0);
   gl_Position = proj * view * worldPos;

   if (isSun) {
       vertColor = aColor;
       return;
   }

   vec3 normal = normalize(mat3(model) * aNormal);
   vec3 lDir = normalize(-lightDir);

   float amb = (ambientStrength > 0.0) ? ambientStrength : 0.35;
   float diff = max(dot(normal, lDir), 0.0);

   vec3 viewDir = normalize(viewPos - worldPos.xyz);
   vec3 reflectDir = reflect(-lDir, normal);
   float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);

   vec3 ambient = amb * aColor;
   vec3 diffuse = (1.0 - amb) * diff * aColor;
   vec3 specular = 0.20 * spec * vec3(1.0);

   vertColor = ambient + diffuse + specular;
}