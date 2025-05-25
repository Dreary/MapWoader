#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec4 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec4 VertexColor;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    TexCoord = aTexCoord;
    Normal = mat3(transpose(inverse(model))) * aNormal;

    VertexColor = aColor / 255.0; // Convert from u8vec4-style to normalized vec4

    gl_Position = projection * view * worldPos;
}
