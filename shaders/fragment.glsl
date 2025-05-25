#version 410 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 VertexColor;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 lightDir = normalize(vec3(1.0, -2.0, 1.0));
uniform vec3 lightColor = vec3(1.0);
uniform vec3 ambientColor = vec3(1.0);

void main()
{
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 ambient = ambientColor;

    vec3 lighting = ambient + diffuse;
    vec4 texColor = texture(texture1, TexCoord);

    FragColor = vec4(texColor.rgb * lighting, texColor.a) * VertexColor;
}
