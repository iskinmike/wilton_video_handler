#version 330 core
in vec4 vertexColor;
in vec2 tex_coords;
out vec4 color;
// in vec4 vertexColor;

uniform sampler2D ourTexture;

void main()
{
	// color = vertexColor;
    color = vec4(1.0f, 0.5f, 0.0f, 1.0f);
	// color = texture(ourTexture, tex_coords) + vertexColor/4;
}