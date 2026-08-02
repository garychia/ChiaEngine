#version 450

layout (std140, set = 0, binding = 0) uniform MatrixBlock
{
    mat4 world;
    mat4 view;
    mat4 projection;
    float useTexture;
};

layout (set = 0, binding = 1) uniform sampler2D uTexture;

layout (location = 0) in vec4 vColor;
layout (location = 1) in vec2 vTexCoord;
layout (location = 2) flat in uint vCMode;

layout (location = 0) out vec4 FragColor;

void main()
{
    vec4 base = vColor;
    if (useTexture > 0.5)
    {
        vec4 texel = texture(uTexture, vTexCoord);
        // vCMode & 1 → modulate(與 OpenGL 版一致)
        if ((vCMode & 1u) != 0u)
            base *= texel;
        else
            base = texel;
    }
    FragColor = base;
}
