#ifndef MARKER_DRAWER_HPP
#define MARKER_DRAWER_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>

//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>

#include "shader_class.hpp"
#include "frame_to_bg.hpp"
#include "reverse_buffer.hpp"

#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "shader_class.hpp"

////////////////////////// Utils
//enum class pos{
//    p_11 = 0, p_12, p_13, p_14,
//    p_21, p_22, p_23, p_24,
//    p_31, p_32, p_33, p_34,
//    p_41, p_42, p_43, p_44
//};
//void swap_values(float dest[16], pos pos_1_en, pos pos_2_en){
//    int pos_1 = static_cast<int> (pos_1_en);
//    int pos_2 = static_cast<int> (pos_2_en);
//    float tmp = dest[pos_1];
//    dest[pos_1] = dest[pos_2];
//    dest[pos_2] = tmp;
//}
//void transpose_matrix_from_printed(float M[16]){
//    swap_values(M, pos::p_12, pos::p_21);
//    swap_values(M, pos::p_13, pos::p_31);
//    swap_values(M, pos::p_14, pos::p_41);

//    swap_values(M, pos::p_23, pos::p_32);
//    swap_values(M, pos::p_24, pos::p_42);

//    swap_values(M, pos::p_34, pos::p_43);
//}
////////////////////////// Utils end

class marker_drawer
{
    std::mutex cond_mtx;
    struct sync_waiter {
        std::atomic_bool flag;
        std::condition_variable cond;
    };
    std::vector<sync_waiter*> sync_array;
    int width, height;
    std::shared_ptr<shader_class> test_shader;
    std::shared_ptr<background_texture> bg_texture;
    std::shared_ptr<reverse_buffer> buffer_reverser;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    GLFWwindow* window;

    static GLfloat vertices[12];
    static GLuint indices[6];
    static GLfloat projection_matrix[16];
    static GLfloat view_matrix[16];
    static GLfloat translate_matrix[16];
    static GLfloat scale_matrix[16];

    std::thread drawer_thread;
    std::atomic_bool stop_drawing;
    std::mutex mtx;

    std::vector<float> projection_vector;
    uint8_t* image_ptr;
    void** destination_ptr;
    std::atomic_bool drawing_needed;
public:
    marker_drawer();
    ~marker_drawer();

    void run_draw_thread(int width, int height);

    void start_draw();

    bool draw();

    void ask_to_draw(const std::vector<float>& prj, uint8_t* image, unsigned char **dest);
};


#endif // MARKER_DRAWER_HPP


