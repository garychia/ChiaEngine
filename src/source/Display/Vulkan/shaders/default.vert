#version 450

layout (std140, set = 0, binding = 0) uniform MatrixBlock
{
    mat4 world;
    mat4 view;
    mat4 projection;
    float useTexture;
};

layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in uint aCMode;
layout (location = 4) in uint aGUI;

layout (location = 0) out vec4 vColor;
layout (location = 1) out vec2 vTexCoord;
layout (location = 2) flat out uint vCMode;

void main()
{
    gl_Position = projection * view * world * vec4(aPos.xyz, 1.0);
    vColor = aColor;
    vTexCoord = aTexCoord;
    vCMode = aCMode;
}
