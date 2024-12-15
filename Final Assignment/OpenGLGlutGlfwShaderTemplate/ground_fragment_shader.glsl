#version 330 core

in vec3 FragPos;    // Fragment position
in vec3 Normal;     // Normal vector

out vec4 FragColor;

uniform vec3 lightPos;    // Position of the light
uniform vec3 viewPos;     // Camera/view position
uniform vec3 lightColor;  // Light color
uniform vec3 groundColor; // Base color of the ground

void main() {
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting
    float specularStrength = 0.3;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = specularStrength * spec * lightColor;

    // Combine lighting with ground color
    vec3 result = (ambient + diffuse + specular) * groundColor;
    FragColor = vec4(result, 1.0);
}