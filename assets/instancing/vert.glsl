#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec2 v_Position;
layout (location = 1) in vec2 v_TexCoord;

layout (location = 2) in vec4 inModel0; // rotation float, scale float[2]
layout (location = 3) in vec4 inModel1;
layout (location = 4) in vec4 inModel2;
layout (location = 5) in vec3 inColor;
layout (location = 6) in vec2 intexCoord00;
layout (location = 7) in vec2 intexCoordDelta;
layout (location = 8) in int  intexID;

layout (binding = 0) uniform UniformBufferObject {
    mat4 projection_view;
} ubo;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragUV;

void main() {
    mat4 model = mat4 (inModel0, inModel1, inModel2, vec4(0,0,0,1));
    
    gl_Position = ubo.projection_view * transpose(model) * vec4(v_Position, 0, 1.0);

    fragColor = inColor;
    fragUV = vec3(intexCoord00 + v_TexCoord * intexCoordDelta, intexID + 0.1f);
}