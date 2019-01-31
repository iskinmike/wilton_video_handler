#include "reverse_buffer.hpp"



void reverse_buffer::setup_shader_program(){
    shader = shader_class{reverse_buffer::vertex_shader_text, reverse_buffer::fragment_shader_text};
    shader.compile_shaders();
    program = shader.create_program();
    vertex_texture_location = glGetUniformLocation(program, "loaded_texture");
}

void reverse_buffer::bind_EBO(){
    glGenBuffers(1, &EBO);
    // Нужно добавить EBO в VAO и указать какие индексы мы хотим использовать
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void reverse_buffer::bind_VBO(){
    // Создаем объеки вершинного буфера. Тип VBO — GL_ARRAY_BUFFER.
    glGenBuffers(1, &VBO);
    // OpenGL позволяет привязывать множество буферов, если у них разные типы.
    // Мы можем привязать GL_ARRAY_BUFFER к нашему буферу с помощью glBindBuffer:
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // С этого момента любой вызов, использующий буфер, будет работать с VBO.
    // Теперь мы можем вызвать glBufferData для копирования вершинных данных в этот буфер.
    /// Существует 3 режима:
    /// GL_STATIC_DRAW: данные либо никогда не будут изменяться, либо будут изменяться очень редко;
    /// GL_DYNAMIC_DRAW: данные будут меняться довольно часто;
    /// GL_STREAM_DRAW: данные будут меняться при каждой отрисовке.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Создаем объект EBO - elementary buffer object там храняться индексы вершин из VBO
    bind_EBO();

    // Нужно сообщить OpenGL как он должен интерпретировать вершинные данные.
    // Которые у нас в VBO храняться
    // Делается это с помощью функции glVertexAttribPointer
    // Атрибут с координатами
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Атрибут с координатами текстуры
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3* sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
}

reverse_buffer::reverse_buffer(){
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); // All upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // Set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT (usually basic wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    setup_shader_program();
    bind_VAO();
}

void reverse_buffer::load_texture_rgba(int size_x, int size_y, uint8_t *data){
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size_x, size_y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    //        glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    //        glBindTexture(GL_TEXTURE_2D, texture);
    //        glUniform1i(vertex_texture_location, 0); // Устанавливаем значение текстуры в положение 0 т.е. выбрана первая загруженная текстура кажется это так рабоотает
}

void reverse_buffer::draw_background(){
    shader.use_program();
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(vertex_texture_location, 0);
    glBindVertexArray(VAO); // Привязываем Масиив Вершинных объектов
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0); // Отвязываем Масиив Вершинных объектов
    glBindTexture(GL_TEXTURE_2D, 0);
}


std::string reverse_buffer::fragment_shader_text =
"#version 330 core\n"
"in vec2 tex_coords;"
"out vec4 color;"
"uniform sampler2D loaded_texture;\n"
"void main()\n"
"{\n"
"   color = texture(loaded_texture, tex_coords);\n"
"}\n";
std::string reverse_buffer::vertex_shader_text =
"#version 330 core\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec2 texture_coords;\n"
"out vec2 tex_coords;\n"
"void main()\n"
"{\n"
// "    tex_coords = texture_coords;\n"
"    tex_coords = vec2(texture_coords.x, 1.0 - texture_coords.y);\n"
"    gl_Position = vec4(position, 1);\n"
"}\n";

GLfloat reverse_buffer::vertices[20] = {
     // Позиции          // Текстурные координаты (меняются от 0.0f до 1.0f)
    1.0f,  1.0f, 0.0f,   1.0f, 1.0f,   // Верхний правый
     1.0f, -1.0f, 0.0f,  1.0f, 0.0f,   // Нижний правый
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,   // Нижний левый
    -1.0f,  1.0f, 0.0f,  0.0f, 1.0f    // Верхний левый
//     1.0f,  1.0f, 0.0f, 1.0f, 0.0f,   // Верхний правый
//     1.0f, -1.0f, 0.0f, 1.0f, 1.0f,   // Нижний правый
//    -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,   // Нижний левый
//    -1.0f,  1.0f, 0.0f, 0.0f, 0.0f     // Верхний левый
};

GLuint reverse_buffer::indices[6] = {  // Помните, что мы начинаем с 0!
    0, 1, 3,   // Первый треугольник
    1, 2, 3    // Второй треугольник
};

GLuint reverse_buffer::get_VAO() const
{
    return VAO;
}

GLuint reverse_buffer::get_texture() const
{
    return texture;
}
