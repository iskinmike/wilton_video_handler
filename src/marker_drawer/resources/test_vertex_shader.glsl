#version 330 core
layout (location = 0) in vec3 position; // Устанавливаем позицию атрибута в 0
layout (location = 1) in vec3 color;    // Устанавливаем позицию атрибута в 1
layout (location = 2) in vec2 texture_coords; // Устанавливаем позицию атрибута в 1
out vec4 vertexColor; // Передаем цвет во фрагментный шейдер
out vec2 tex_coords;
out vec2 pos;
uniform float distortion;
uniform float cos_time;
uniform float sin_time;
uniform mat4 model_projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_view_projection_matrix;
uniform mat4 model_translate_matrix;
uniform mat4 model_scale_matrix;
uniform mat4 mtx_calc_proj;
void main()
{
	tex_coords = vec2(1.0 - texture_coords.x, 1.0 - texture_coords.y);
	// gl_Position = vec4(position.x + sin_time, position.y + cos_time, 0.5, 1.5 + sin_time);
	mat4 result_mtx = model_projection_matrix * view_matrix;
	result_mtx = result_mtx * model_view_projection_matrix;
	result_mtx = result_mtx * model_scale_matrix;
	result_mtx = result_mtx * model_translate_matrix;
	gl_Position = vec4(position.x,
						position.y,
						position.z, 
						1);
	gl_Position = result_mtx * gl_Position;
	// mat4 rotate = mat4( vec4(-1.0, 0.0, 0.0, 0.0), 
	// 				    vec4(0.0, -1.0, 0.0, 0.0), 
	// 				    vec4(0.0, 0.0, 1.0, 0.0), 
	// 				    vec4(0.0, 0.0, 0.0, 1.0));
	// gl_Position = gl_Position * rotate;

	// gl_Position = vec4(position, 1);
	vertexColor = vec4(color + distortion, 1.0f);
}