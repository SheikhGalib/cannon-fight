#version 330 core

in vec3 fragNormal;
in vec3 fragColor;

out vec4 FragColor;

// Direction the light travels (points AWAY from the light, e.g. a sun
// shining down-and-across), set once in Main.cpp. Not the full ambient +
// diffuse + specular Phong model yet (that, plus contrasting wood/metal
// materials, is Phase 2 - see docs/phase-2-plan.md Step 4) - this is just
// enough shading (ambient floor + one diffuse term) for shapes to read as
// solid 3D forms instead of flat colored silhouettes.
uniform vec3 lightDir;

void main()
{
   vec3 normal = normalize(fragNormal);

   // How directly this fragment's surface faces the light: 1.0 = facing it
   // head-on, 0.0 = facing sideways or away. This is the standard Lambertian
   // diffuse term (dot product of the normal and the light direction).
   float diffuse = max(dot(normal, -lightDir), 0.0);

   // Keep faces that are facing away from the light dim but still visible
   // (0.35) instead of pure black - a cheap stand-in for bounced/sky light,
   // until a real ambient term arrives in Phase 2.
   float brightness = 0.35 + 0.65 * diffuse;

   FragColor = vec4(fragColor * brightness, 1.0);
};
