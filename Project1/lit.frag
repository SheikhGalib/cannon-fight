#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragColor;
in vec4 fragPosLightSpace;

out vec4 FragColor;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform bool isSun;
uniform bool isNight;

// Shadow Map & PCF
uniform sampler2D shadowMap;
uniform bool useShadowMap;

// Multi-Light System: Point Lights (torches, braziers, muzzle flashes)
uniform int numPointLights;
uniform vec3 pointLightPos[8];
uniform vec3 pointLightColor[8];

// Ray-tracing slab intersection fallback
uniform bool useRayTracing;
uniform int numBoxes;
uniform vec3 boxMin[36];
uniform vec3 boxMax[36];

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

float calcRayShadow(vec3 ro, vec3 rd, float maxDist) {
    for (int i = 0; i < numBoxes; i++) {
        float tHit;
        if (hitBox(ro, rd, boxMin[i], boxMax[i], tHit)) {
            if (tHit > 0.15 && tHit < maxDist) {
                return 0.0;
            }
        }
    }
    return 1.0;
}

// 3x3 Percentage-Closer Filtering (PCF) Soft Shadow
float calcShadowMapPCF(vec4 fragPosLS, vec3 normal, vec3 lDir) {
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Out of depth map bounds
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }

    // Normal-offset slope-scaled depth bias to eliminate shadow acne
    float bias = max(0.0035 * (1.0 - dot(normal, lDir)), 0.0006);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 0.0 : 1.0;
        }
    }
    return shadow / 9.0;
}

void main()
{
    // Sun and Moon are self-illuminated emissive spheres
    if (isSun) {
        FragColor = vec4(fragColor, 1.0);
        return;
    }

    vec3 normal = normalize(fragNormal);
    vec3 lDir = normalize(-lightDir);
    vec3 vDir = normalize(viewPos - fragPos);
    vec3 hDir = normalize(lDir + vDir);

    // =========================================================================
    // 1. Procedural Surface Materials & Details
    // =========================================================================
    vec3 baseColor = fragColor;

    // Stone Masonry (Castle walls, towers, pillars, bridge)
    bool isStone = (distance(fragColor, vec3(0.62, 0.60, 0.55)) < 0.22) ||
                   (distance(fragColor, vec3(0.45, 0.43, 0.38)) < 0.18) ||
                   (distance(fragColor, vec3(0.18, 0.16, 0.14)) < 0.12);
    if (isStone && fragPos.y > 0.1) {
        float row = floor(fragPos.y / 0.40);
        float hCoord = (abs(normal.x) > 0.5 ? fragPos.z : fragPos.x) + mod(row, 2.0) * 0.40;
        float mortarY = abs(fract(fragPos.y / 0.40) - 0.5) * 2.0;
        float mortarH = abs(fract(hCoord / 0.80) - 0.5) * 2.0;
        float mortar = smoothstep(0.86, 0.98, max(mortarY, mortarH));
        float brickNoise = sin(row * 12.3 + floor(hCoord / 0.80) * 7.7) * 0.05;
        baseColor = mix(baseColor * (1.0 + brickNoise), baseColor * 0.42, mortar);
    }

    // Wood Grain & Planks (Doors, carriages, bridge planks, tent bases)
    bool isWood = (distance(fragColor, vec3(0.42, 0.24, 0.11)) < 0.22) ||
                  (distance(fragColor, vec3(0.60, 0.38, 0.17)) < 0.22);
    if (isWood) {
        float grain = sin(fragPos.y * 40.0 + fragPos.x * 25.0 + fragPos.z * 25.0) * 0.05;
        baseColor *= (1.0 + grain);
    }

    // Ground Grass / Trampled Dirt Road near drawbridge
    bool isGrass = (fragColor.g > fragColor.r * 1.05 && fragPos.y < 0.2);
    if (isGrass) {
        if (abs(fragPos.z) < 2.3 && fragPos.x > -22.0 && fragPos.x < 4.0) {
            // Dirt road leading directly to the drawbridge
            baseColor = mix(baseColor, vec3(0.35, 0.26, 0.16), 0.70);
        } else {
            float patch = sin(fragPos.x * 0.4 + fragPos.z * 0.4) * 0.05;
            baseColor *= (1.0 + patch);
        }
    }

    // Metallic response (Iron cannon, swords, armor, helmets)
    bool isMetal = (distance(fragColor, vec3(0.17, 0.17, 0.19)) < 0.12) ||
                   (distance(fragColor, vec3(0.08, 0.08, 0.09)) < 0.08) ||
                   (distance(fragColor, vec3(0.72, 0.55, 0.15)) < 0.15) ||
                   (distance(fragColor, vec3(0.75, 0.78, 0.82)) < 0.20);

    // =========================================================================
    // 2. Hemispheric Ambient Light (Sky / Ground bounce)
    // =========================================================================
    float ambStrength = (ambientStrength > 0.0) ? ambientStrength : (isNight ? 0.12 : 0.22);
    vec3 skyColor    = isNight ? vec3(0.05, 0.07, 0.14) : vec3(0.40, 0.52, 0.68);
    vec3 groundColor = isNight ? vec3(0.02, 0.02, 0.04) : vec3(0.24, 0.20, 0.15);
    float hemiFactor = normal.y * 0.5 + 0.5;
    vec3 ambient = mix(groundColor, skyColor, hemiFactor) * ambStrength * baseColor;

    // =========================================================================
    // 3. Directional Light: Diffuse + Blinn-Phong Specular
    // =========================================================================
    float diff = max(dot(normal, lDir), 0.0);
    vec3 dirLightColor = isNight ? vec3(0.45, 0.55, 0.75) : vec3(1.00, 0.96, 0.88);
    vec3 diffuse = diff * dirLightColor * baseColor;

    // Material Shininess & Specular response
    float shininess = isMetal ? 96.0 : (isStone ? 20.0 : 6.0);
    float specStrength = isMetal ? 0.65 : (isStone ? 0.12 : 0.04);
    float spec = pow(max(dot(normal, hDir), 0.0), shininess);
    vec3 specular = specStrength * spec * dirLightColor;

    // Metallic Fresnel rim highlight
    if (isMetal) {
        float rim = 1.0 - max(dot(normal, vDir), 0.0);
        specular += pow(rim, 3.5) * 0.25 * vec3(0.85, 0.90, 1.0);
    }

    // =========================================================================
    // 4. Soft Shadow Mapping (PCF) with Ray-Tracing Fallback
    // =========================================================================
    float shadow = 1.0;
    if (diff > 0.0) {
        if (useShadowMap) {
            shadow = calcShadowMapPCF(fragPosLightSpace, normal, lDir);
        } else if (useRayTracing) {
            vec3 ro = fragPos + normal * 0.05;
            shadow = calcRayShadow(ro, lDir, 160.0);
        }
    }

    vec3 directLit = shadow * (diffuse + specular);

    // =========================================================================
    // 5. Point Lights (Braziers, Torches, Muzzle Flash) with Quadratic Attenuation
    // =========================================================================
    vec3 pointLightsTotal = vec3(0.0);
    for (int i = 0; i < numPointLights && i < 8; i++) {
        vec3 pPos = pointLightPos[i];
        vec3 pColor = pointLightColor[i];
        vec3 pToLight = pPos - fragPos;
        float d = length(pToLight);
        if (d > 0.001) {
            vec3 pDir = pToLight / d;
            float pAttn = 1.0 / (1.0 + 0.12 * d + 0.045 * d * d);
            float pDiff = max(dot(normal, pDir), 0.0);
            vec3 pHal = normalize(pDir + vDir);
            float pSpec = pow(max(dot(normal, pHal), 0.0), shininess) * specStrength;
            pointLightsTotal += pAttn * pColor * (pDiff * baseColor + pSpec);
        }
    }

    // Total illuminated color
    vec3 finalColor = ambient + directLit + pointLightsTotal;

    // =========================================================================
    // 6. Atmospheric Distance Fog (Starts only on distant scenery > 65m)
    // =========================================================================
    float dist = length(viewPos - fragPos);
    float fogDist = max(0.0, dist - 65.0);
    float fogFactor = 1.0 - exp(-pow(fogDist * (isNight ? 0.008 : 0.005), 1.35));
    fogFactor = clamp(fogFactor, 0.0, 0.85);
    vec3 fogColor = isNight ? vec3(0.03, 0.04, 0.10) : vec3(0.64, 0.72, 0.84);
    finalColor = mix(finalColor, fogColor, fogFactor);

    // =========================================================================
    // 7. ACES Filmic Tone Mapping (Punchy contrast & rich blacks) + Gamma 2.2
    // =========================================================================
    // ACES curve: a=2.51, b=0.03, c=2.43, d=0.59, e=0.14
    vec3 x = finalColor * 1.08;
    vec3 mapped = clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
    FragColor = vec4(pow(mapped, vec3(1.0 / 2.2)), 1.0);
}
