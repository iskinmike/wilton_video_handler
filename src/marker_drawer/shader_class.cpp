#include "shader_class.hpp"
#include <fstream>

std::string shader_class::get_fragment_shader_text() const
{
    return fragment_shader_text;
}

void shader_class::set_fragment_shader_text(const std::string &value)
{
    fragment_shader_text = value;
}

std::string shader_class::get_vertex_shader_text() const
{
    return vertex_shader_text;
}

void shader_class::set_vertex_shader_text(const std::string &value)
{
    vertex_shader_text = value;
}

void shader_class::load_vertex_shader_source(const std::string &source_path){
    shader_class::load_shader_source(source_path, vertex_shader_text);
}

void shader_class::load_fragment_shader_source(const std::string &source_path){
    shader_class::load_shader_source(source_path, fragment_shader_text);
}

GLint shader_class::load_shader(GLint shader_handler, const std::basic_string<GLchar> &source){
    std::vector<GLchar> tmp_src(source.size());
    for (size_t i = 0; i < source.size(); ++i) {
        tmp_src[i] = source[i];
    }

    auto gl_pointer = static_cast<const GLchar*> (source.c_str());
    auto gl_const_ptr = const_cast<const GLchar* const*>(&gl_pointer);

    glShaderSource(shader_handler, 1,
                   gl_const_ptr,
                   NULL);
    glCompileShader(shader_handler);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader_handler, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(shader_handler, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return success;
}

void shader_class::load_shader_source(const std::string &source_path, std::string &dest){
    std::ifstream source;
    source.open(source_path.c_str());
    if (!source.is_open()) {
        // handle error
        std::cout << "Error open file [" << source_path << "]" << std::endl;
        return;
    }
    source.seekg(0, std::ios_base::seekdir::_S_end);
    auto size = source.tellg();
    std::vector<char> buffer(size);
    dest.resize(size);
    source.seekg(0, std::ios_base::seekdir::_S_beg);
    source.read(buffer.data(), size);
    dest.assign(buffer.data(), size);
//    std::cout << "readed size: " << dest.size() << std::endl;
//    std::cout << "readed file: [" << dest.c_str() << "]"<< std::endl;
}

shader_class::shader_class(const std::string &vertex, const std::string &fragment) :
    fragment_shader_text(fragment),
    vertex_shader_text(vertex),
    vertex_shader(glCreateShader(GL_VERTEX_SHADER)),
    fragment_shader(glCreateShader(GL_FRAGMENT_SHADER)),
    shader_program(0)
{}

shader_class::shader_class() :
    fragment_shader_text(std::string{}),
    vertex_shader_text(std::string{}),
    vertex_shader(glCreateShader(GL_VERTEX_SHADER)),
    fragment_shader(glCreateShader(GL_FRAGMENT_SHADER)),
    shader_program(0){}

GLint shader_class::create_program(){
    /// Удаллим прогшрамму предыдущую если она у нас была
    drop_program();
    /// Шейдерная программа — это объект, являющийся финальным результатом комбинации нескольких шейдеров.
    /// Для того, чтобы использовать собранные шейдеры их требуется соединить в объект шейдерной программы,
    /// а затем активировать эту программу при отрисовке объектов
    shader_program = glCreateProgram();

    /// Теперь нам надо присоединить наши собранные шейдеры к программе,
    /// а затем связать их с помощью glLinkProgram:
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    // Проверим работу
    GLint success = false;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    GLchar infoLog[512];
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER_PROGRAM::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // После связывания в программу созданные шейдеры больше не нужны
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    vertex_shader = 0;
    fragment_shader = 0;
    return success;
}

GLint shader_class::compile_shaders() {
    std::cout << "vertex_shader\n" << std::endl;
    auto vertex_res = load_shader(vertex_shader, vertex_shader_text);
    std::cout << "fragment_shader\n" << std::endl;
    auto fragment_res = load_shader(fragment_shader, fragment_shader_text);
    return (vertex_res && fragment_res);
}

GLint shader_class::get_program() {
    return shader_program;
}

void shader_class::drop_program(){
    if (shader_program) {
        glDeleteProgram(shader_program);
    }
    shader_program = 0;
}

void shader_class::drop_vertex_shader(){
    if (vertex_shader) {
        glDeleteShader(vertex_shader);
    }
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
}

void shader_class::drop_fragment_shader(){
    if (fragment_shader) {
        glDeleteShader(fragment_shader);
    }
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
}

GLint shader_class::use_program(){
    // Для использования созданной программы надо вызвать glUseProgram:
    if (shader_program) {
        glUseProgram(shader_program);
    }
    return shader_program;
}

shader_class::~shader_class(){
    drop_program();
    drop_vertex_shader();
    drop_fragment_shader();
}
