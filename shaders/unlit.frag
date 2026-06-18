#version 330 core
out vec4 FragColor;

in vec3 WorldNormal;

uniform vec3 lightColor;
uniform vec3 lightDir;

void main() {
    float mask = 1.0; 
    
    if (length(lightDir) > 0.001) {
        vec3 N = normalize(WorldNormal);
        vec3 D = normalize(lightDir);
        
        // Calculate alignment factor
        float factor = dot(N, D);
        
        // Crisp transition: front hemisphere glows, back housing gets a faint dark ambient base
        mask = factor > 0.0 ? 1.0 : 0.15; 
    }
    
    FragColor = vec4(lightColor * mask, 1.0);
}