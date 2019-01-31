#ifndef FRAME_TO_BG_HPP
#define FRAME_TO_BG_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <iostream>

#include "shader_class.hpp"

class background_texture
{
    GLuint texture;
    GLuint program;
    shader_class shader;
    static std::string fragment_shader_text;
    static std::string vertex_shader_text;
    static GLfloat vertices[20];
    static GLuint indices[6];
    GLint vertex_texture_location;
    GLuint VBO;
    GLuint EBO;
    GLuint VAO;

    void setup_shader_program();

    void bind_EBO();

    void bind_VBO();

    void bind_VAO(){
        // Для каждой отрисовки необходимо настраивать программу и данные VBO
        // Для упрощения можно сделать VAO - массив объектов вершинного буфера
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        // Привязываем к VAO: VBO и EBO
        bind_VBO();

        // Отвязываем VAO и вместе с ним отвязываются все остальные
        glBindVertexArray(0);
    }

public:
    background_texture();
    ~background_texture(){}

    void load_texture_rgba(int size_x, int size_y, uint8_t* data);
    
    void draw_background();

    GLuint get_texture() const;
    GLuint get_VAO() const;
};



#endif // FRAME_TO_BG_HPP


