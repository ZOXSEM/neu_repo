glsl
#version 460 core

// Uniforms for camera and time
uniform vec2 iResolution;
uniform float iTime;
uniform vec3 iCameraPosition;

// Noise function for procedural textures
float noise(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 45.164))) * 43758.5453);
}

// Fractal Brownian Motion for cloud-like patterns
float fbm(vec3 p) {
    float f = 0.0;
    f += 0.5000 * noise(p); p *= 2.0; 
    f += 0.2500 * noise(p); p *= 2.0;
    f += 0.1250 * noise(p); p *= 2.0;
    f += 0.0625 * noise(p);
    return f;
}

// Sky color based on position and time
vec3 getSkyColor(vec3 rayDir) {
    // Base sky color (dark blue at horizon, deeper blue at zenith)
    vec3 skyColor = mix(
        vec3(0.1, 0.1, 0.2),  // Horizon
        vec3(0.05, 0.05, 0.15), // Zenith
        rayDir.y * 0.5 + 0.5
    );
    
    // Add some subtle color variation
    float timeFactor = sin(iTime * 0.1) * 0.1 + 0.9;
    skyColor *= timeFactor;
    
    return skyColor;
}

// Moon shader with phase and movement
vec3 getMoon(vec3 rayDir, vec2 uv) {
    // Moon position (moving across the sky)
    float moonPhase = (sin(iTime * 0.2) + 1.0) * 0.5;
    vec3 moonPos = vec3(
        cos(iTime * 0.1) * 0.8,
        sin(iTime * 0.05) * 0.6,
        sin(iTime * 0.1) * 0.8
    );
    
    // Calculate distance to moon
    float dist = length(rayDir - moonPos);
    
    if (dist < 0.1) {
        // Moon surface with craters
        vec3 moonColor = vec3(0.8, 0.8, 0.85);
        
        // Add crater pattern
        vec3 moonCoord = normalize(moonPos);
        float craterNoise = fbm(moonCoord * 5.0) * 0.3;
        moonColor -= craterNoise;
        
        // Phase effect
        float phaseEffect = 1.0 - moonPhase * 0.7;
        moonColor *= phaseEffect;
        
        return moonColor;
    }
    
    return vec3(0.0);
}

// Star field with twinkling effect
vec3 getStars(vec3 rayDir) {
    vec3 starColor = vec3(0.0);
    
    // Generate star positions in a spherical distribution
    for (int i = 0; i < 100; i++) {
        // Use a simple hash to generate star positions
        float seed = float(i) * 12.9898 + iTime;
        vec3 starPos = vec3(
            sin(seed * 0.1) * 100.0,
            cos(seed * 0.13) * 100.0,
            sin(seed * 0.17) * 100.0
        );
        
        // Distance to star
        float dist = length(rayDir - starPos);
        
        // Only draw stars that are close enough
        if (dist < 0.05) {
            // Twinkling effect
            float twinkle = sin(iTime * 2.0 + seed * 0.5) * 0.5 + 0.5;
            float intensity = 0.8 + twinkle * 0.2;
            
            // Star color (white with slight blue tint)
            vec3 color = vec3(1.0, 0.95, 0.9) * intensity;
            
            starColor += color;
        }
    }
    
    return starColor;
}

// Main raymarching function
vec3 render(vec3 rayDir) {
    vec3 color = getSkyColor(rayDir);
    
    // Add moon
    vec3 moonColor = getMoon(rayDir, vec2(0.0));
    color = mix(color, moonColor, moonColor.r > 0.0 ? 1.0 : 0.0);
    
    // Add stars
    vec3 starColor = getStars(rayDir);
    color += starColor * 0.5;
    
    return color;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Normalize coordinates
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / min(iResolution.y, iResolution.x);
    
    // Camera setup
    vec3 rayDir = normalize(vec3(uv, 1.0));
    
    // Render the scene
    vec3 color = render(rayDir);
    
    // Apply gamma correction
    color = pow(color, vec3(0.4545));
    
    fragColor = vec4(color, 1.0);
}