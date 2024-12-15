#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;

uniform vec3 lightDir;     
uniform vec3 spotlightPos; 
uniform vec3 spotlightDir; 
uniform vec3 pointLightPos;
uniform vec3 viewPos;      

uniform vec3 lightColor;
uniform float spotlightCutOff;
uniform float spotlightOuterCutOff;

uniform float constantAttenuation;
uniform float linearAttenuation;
uniform float quadraticAttenuation;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 dirLightDir = normalize(-lightDir);
    float dirDiff = max(dot(norm, dirLightDir), 0.0);
    vec3 dirDiffuse = dirDiff * lightColor;

    vec3 spotLightDir = normalize(spotlightPos - FragPos);
    float spotTheta = dot(spotLightDir, normalize(-spotlightDir));
    float spotEpsilon = spotlightCutOff - spotlightOuterCutOff;
    float spotIntensity = clamp((spotTheta - spotlightOuterCutOff) / spotEpsilon, 0.0, 1.0);

    float spotDiff = max(dot(norm, spotLightDir), 0.0);
    vec3 spotDiffuse = spotIntensity * spotDiff * lightColor;

    vec3 pointLightDir = normalize(pointLightPos - FragPos);
    float distance = length(pointLightPos - FragPos);
    float attenuation = 1.0 / (constantAttenuation + linearAttenuation * distance + quadraticAttenuation * (distance * distance));

    float pointDiff = max(dot(norm, pointLightDir), 0.0);
    vec3 pointDiffuse = attenuation * pointDiff * lightColor;

    vec4 texColor = texture(texture1, TexCoord);

    vec3 result = (dirDiffuse + spotDiffuse + pointDiffuse) * texColor.rgb;
    FragColor = vec4(result, 1.0);
}