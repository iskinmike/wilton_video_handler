#ifndef SHADER_CLASS_HPP
#define SHADER_CLASS_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <iostream>

class shader_class
{
    std::string fragment_shader_text;
    std::string vertex_shader_text;

    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint shader_program;

    GLint load_shader(GLint shader_handler, const std::basic_string<GLchar>& source);

public:
    shader_class(const std::string& vertex, const std::string& fragment);
    shader_class();

    GLint create_program();
    ~shader_class();

    GLint compile_shaders();
    GLint get_program();
    void drop_program();
    void drop_vertex_shader();
    void drop_fragment_shader();

    GLint use_program();

    std::string get_fragment_shader_text() const;
    void set_fragment_shader_text(const std::string &value);
    std::string get_vertex_shader_text() const;
    void set_vertex_shader_text(const std::string &value);

    // support functions
    void load_vertex_shader_source(const std::string& source_path);
    void load_fragment_shader_source(const std::string& source_path);
    // class support functions
    static void load_shader_source(const std::string& source_path, std::string& dest);
};

#endif // SHADER_CLASS_HPP
