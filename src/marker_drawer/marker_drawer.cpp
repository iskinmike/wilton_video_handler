#include "marker_drawer.hpp"

marker_drawer::marker_drawer()
    : window(nullptr),
      test_shader(nullptr),
      bg_texture(nullptr),
      buffer_reverser(nullptr),
      image_ptr(nullptr),
      destination_ptr(nullptr)
{
    stop_drawing.exchange(false);
    drawing_needed.exchange(false);
}

marker_drawer::~marker_drawer(){
    stop_drawing.exchange(true);
    if (drawer_thread.joinable()){
        drawer_thread.join();
    }
    if (window) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

void marker_drawer::run_draw_thread(int width, int height){
    this->width = width;
    this->height = height;
    drawer_thread = std::thread([this] {return this->start_draw();});
}

void marker_drawer::start_draw(){
    /// Нам нужно сообщить OpenGL размер отрисовываемого окна, чтобы OpenGL знал,
    /// как мы хотим отображать данные и координаты относительно окна
    glfwInit();
    //Настройка GLFW
    //Задается минимальная требуемая версия OpenGL.
    //Мажорная
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //Минорная
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //Установка профайла для которого создается контекст
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //Выключение возможности изменения размера окна
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // создаем окно
    window = glfwCreateWindow(640, 480, "LearnOpenGL", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
//    glfwHideWindow(window);
    glfwSetWindowPos(window, 700, 100);
    glfwMakeContextCurrent(window);

    // Инициализация opengl контекста через glew
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return;
    }

    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    GLint nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum nr of vertex attributes supported: " << nrAttributes << std::endl;
//    shader_class test_shader/*(vertex_shader_source, fragment_shader_source)*/;
    test_shader = std::make_shared<shader_class> ();

    test_shader->load_vertex_shader_source("dist/resources/test_vertex_shader.glsl");
    test_shader->load_fragment_shader_source("dist/resources/test_fragment_shader.glsl");

    test_shader->compile_shaders();
    test_shader->create_program();

    // Для каждой отрисовки необходимо настраивать программу и данные VBO
    // Для упрощения можно сделать VAO - массив объектов вершинного буфера
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

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

    // Создаем объект EBO -elementary buffer object там храняться индексы вершин из VBO
    glGenBuffers(1, &EBO);
    // Нужно добавить EBO в VAO и указать какие индексы мы хотим использовать
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Нужно сообщить OpenGL как он должен интерпретировать вершинные данные.
    // Которые у нас в VBO храняться
    // Делается это с помощью функции glVertexAttribPointer
    // Атрибут с координатами
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    //4. Отвязываем VAO
    glBindVertexArray(0);

    // Нужно создать так же обработчик бэкграунда
    bg_texture = std::make_shared<background_texture>();
    buffer_reverser = std::make_shared<reverse_buffer>();

    glClearColor(0.2f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    while (!stop_drawing) {

        {
            std::lock_guard<std::mutex> lock(mtx);
            // check query to draw
            // do draw
            if (drawing_needed) {
                drawing_needed.exchange(false);
                if (draw()){
                    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, *destination_ptr);
                    std::cout << "[*****] 1 frame ptr: " << std::hex <<  *destination_ptr << " " << destination_ptr  << std::endl;
                    buffer_reverser->load_texture_rgba(width, height, static_cast<uint8_t*>(*destination_ptr));
                    buffer_reverser->draw_background();
                    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, *destination_ptr);
//                    glClearColor(0.2f, 0.0f, 0.5f, 1.0f);
                    destination_ptr = nullptr;
                }
                for (auto el : sync_array) {
                    el->flag.exchange(true);
                    el->cond.notify_one();
                }
                sync_array.clear();
            }
        }

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    window = nullptr;
}

bool marker_drawer::draw(){
    if (!projection_vector.size() ||
        !image_ptr || !destination_ptr) {
        return false;
    }
    GLfloat view_projection_matrix[16];
    int counter = 0;
    for (auto& el: projection_vector) {
        view_projection_matrix[counter++] = el;
    }

    std::cout << "draw_bg" << std::endl;
    bg_texture->load_texture_rgba(width, height, image_ptr);
    bg_texture->draw_background(); // Рисуем бэкграунд - наш кадр

    GLint vertexModelProjectionMatrix = glGetUniformLocation(test_shader->get_program(), "model_projection_matrix");
    GLint vertexModelViewMatrix = glGetUniformLocation(test_shader->get_program(), "view_matrix");
    GLint vertexModelViewProjectionMatrix = glGetUniformLocation(test_shader->get_program(), "model_view_projection_matrix");
    GLint vertexModelTranslateMatrix = glGetUniformLocation(test_shader->get_program(), "model_translate_matrix");
    GLint vertexModelScaleMatrix = glGetUniformLocation(test_shader->get_program(), "model_scale_matrix");

    test_shader->use_program();

    glUniformMatrix4fv(vertexModelProjectionMatrix, 1, GL_FALSE, projection_matrix);
    glUniformMatrix4fv(vertexModelViewMatrix, 1, GL_FALSE, view_matrix);
    glUniformMatrix4fv(vertexModelViewProjectionMatrix, 1, GL_FALSE, view_projection_matrix);
    glUniformMatrix4fv(vertexModelTranslateMatrix, 1, GL_FALSE, translate_matrix);
    glUniformMatrix4fv(vertexModelScaleMatrix, 1, GL_FALSE, scale_matrix);

    glBindVertexArray(VAO); // Привязываем Масиив Вершинных объектов
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // рисуем положение маркера
    glBindVertexArray(0); // Отвязываем Масиив Вершинных объектов

    projection_vector.clear();
    image_ptr = nullptr;
    return true;
}

void marker_drawer::ask_to_draw(const std::vector<float> &prj, uint8_t *image, unsigned char **dest){
    sync_waiter waiter;
    waiter.flag.exchange(false);
    {
        std::lock_guard<std::mutex> lock(mtx);
        projection_vector = prj;
        image_ptr = image;
        destination_ptr = reinterpret_cast<void**> (dest);
        drawing_needed.exchange(true);
        sync_array.push_back(&waiter);
    }

    std::unique_lock<std::mutex> lck(cond_mtx);
    while (!waiter.flag) {
       std::cv_status status = waiter.cond.wait_for(lck, std::chrono::seconds(1));
       if (std::cv_status::timeout == status) {
           break;
       }
    }
}


GLfloat marker_drawer::vertices[12] = {
    // Позиции           // Цвета            // Текстурные координаты
    0.5f,  0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
   -0.5f, -0.5f, 0.0f,
   -0.5f,  0.5f, 0.0f,
};

GLuint marker_drawer::indices[6] = {  // Помните, что мы начинаем с 0!
    0, 1, 3,   // Первый треугольник
    1, 2, 3    // Второй треугольник
};

GLfloat marker_drawer::projection_matrix[16] = {  // Помните, что мы начинаем с 0!
     1.813,  0.000,  0.000,  0.0,
     0.000,  2.419,  0.000,  0.0,
     -0.002, 0.002, -1.002, -1.0,
     0.000,  0.000, -20.000,  0.0
};


GLfloat marker_drawer::view_matrix[16] = {  // Помните, что мы начинаем с 0!
    1.0f, 0.0f, 0.0f, 0.0f, // Column 0,
    0.0f, 1.0f, 0.0f, 0.0f, // Column 1,
    0.0f, 0.0f, 1.0f, 0.0f, // Column 2,
    0.0f, 0.0f, 0.0f, 1.0f  // Column 3;
};

GLfloat marker_drawer::translate_matrix[16] = {  // Помните, что мы начинаем с 0!
//    1.0f, 0.0f, 0.0f, 0.0f, // Column 0,
//    0.0f, 1.0f, 0.0f, 0.0f, // Column 1,
//    0.0f, 0.0f, 1.0f, 0.0f, // Column 2,
//    0.0f, 0.0f, 0.0f, 1.0f  // Column 3;
     1.0f, 0.0f, 0.0f, 0.0f, // Column 0,
     0.0f, 1.0f, 0.0f, 0.0f, // Column 1,
     0.0f, 0.0f, 1.0f, 0.0f, // Column 2,
     0.0f, 0.0f, 0.0f, 1.0f  // Column 3;
};

GLfloat marker_drawer::scale_matrix[16] = {  // Помните, что мы начинаем с 0!
    40.0f, 0.0f, 0.0f, 0.0f, // Column 0,
    0.0f, 40.0f, 0.0f, 0.0f, // Column 1,
    0.0f, 0.0f, 40.0f, 0.0f, // Column 2,
    0.0f, 0.0f,  0.0f, 1.0f  // Column 3;
};
