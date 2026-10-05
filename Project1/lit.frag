#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragColor;

out vec4 FragColor;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform bool isSun;

// Ray tracing uniforms
uniform bool useRayTracing;
uniform int numBoxes;
uniform vec3 boxMin[36];
uniform vec3 boxMax[36];

// Ray-AABB slab intersection test
bool hitBox(vec3 ro, vec3 rd, vec3 bMin, vec3 bMax, out float tHit) {
    vec3 invD = 1.0 / rd;
    vec3 t0 = (bMin - ro) * invD;
    vec3 t1 = (bMax - ro) * invD;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar  = min(min(tmax.x, tmax.y), tmax.z);
    if (tNear <= tFar && tFar > 0.0) {
        tHit = tNear > 0.0 ? tNear : tFar;
        return true;
    }
    return false;
}

float calcShadow(vec3 ro, vec3 rd, float maxDist) {
    if (!useRayTracing) return 1.0;
    for (int i = 0; i < numBoxes; i++) {
        float tHit;
        if (hitBox(ro, rd, boxMin[i], boxMax[i], tHit)) {
            if (tHit > 0.15 && tHit < maxDist) {
                return 0.0; // In shadow!
            }
        }
    }
    return 1.0;
}

void main()
{
   if (isSun) {
       FragColor = vec4(fragColor, 1.0);
       return;
   }

   vec3 normal = normalize(fragNormal);
   vec3 lDir = normalize(-lightDir);

   // Ambient
   float amb = (ambientStrength > 0.0) ? ambientStrength : 0.35;
   vec3 ambient = amb * fragColor;

   // Diffuse (Lambertian)
   float diff = max(dot(normal, lDir), 0.0);
   vec3 diffuse = (1.0 - amb) * diff * fragColor;

   // Specular (Phong)
   vec3 viewDir = normalize(viewPos - fragPos);
   vec3 reflectDir = reflect(-lDir, normal);
   float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
   vec3 specular = 0.20 * spec * vec3(1.0);

   // Ray-traced shadow ray towards light
   float shadow = 1.0;
   if (diff > 0.0 && useRayTracing) {
       vec3 ro = fragPos + normal * 0.05;
       shadow = calcShadow(ro, lDir, 160.0);
   }

   vec3 result = ambient + shadow * (diffuse + specular);
   FragColor = vec4(result, 1.0);
}
