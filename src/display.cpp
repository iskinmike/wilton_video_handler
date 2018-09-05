
#include "display.hpp"
#include "frame_keeper.hpp"

#include <iostream>
#include <map>

//#include
namespace {

#ifdef WIN32
struct param_info{
    std::string title;
    HWND window;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	char szTextWin[255];DWORD dwPID = NULL; 
	param_info* tmp_value = reinterpret_cast<param_info*>(lParam);
	std::string title(tmp_value->title);
	if(GetWindowText(hwnd,szTextWin,sizeof(szTextWin)))
	{
		CharToOem(szTextWin,szTextWin);
		std::string window_title(szTextWin);
		if (window_title == title) {
			tmp_value->window = hwnd;
			return false;
		}
	}
	return TRUE;
}

void setup_display_topmost(const std::string& title, HWND& cam_window){
    param_info info;
    info.title = title;
    info.window = 0;

    LONG_PTR title_cb = reinterpret_cast<std::uintptr_t>(&info);
    // EnumWindows calls EnumWindowsProc for each avaliable window handler until returns true and stops if false returned.
    // EnumWindows returns false if EnumWindowsProc return false and true if looks throught all windows.
    if (!EnumWindows(&EnumWindowsProc, title_cb)) {
        if (0 != info.window) {
            // Set Window topmost, 0 - zeores are ignored by flags SWP_NOMOVE | SWP_NOSIZE
            cam_window = info.window;
            SetWindowPos(info.window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
    }
}

#else

std::vector<Window> get_property_as_window(Display *disp, Window win,
        Atom xa_prop_type, const char *prop_name) {
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned char* ret_prop = nullptr;
    unsigned char** ret_prop_ptr = &ret_prop;
    std::vector<Window> ret;

    xa_prop_name = XInternAtom(disp, prop_name, False);

    /* MAX_PROPERTY_VALUE_LEN / 4 explanation (XGcd etWindowProperty manpage):
     *
     * long_length = Specifies the length in 32-bit multiples of the
     *               data to be retrieved.
     */
    XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
            xa_prop_type, &xa_ret_type, &ret_format,
            &ret_nitems, &ret_bytes_after, ret_prop_ptr);

    for (size_t i = 0; i < ret_nitems; ++i) {
        if (nullptr != ret_prop) {
            Window* tmp_ptr = static_cast<Window*>(static_cast<void*>(ret_prop + i*sizeof(unsigned char*)));
            ret.push_back(*tmp_ptr);
        }
    }

    XFree(ret_prop);
    return ret;
}

std::string get_property_as_string(Display *disp, Window win,
        Atom xa_prop_type, const char *prop_name) {
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned char *ret_prop;
    std::string ret;

    xa_prop_name = XInternAtom(disp, prop_name, False);

    /* MAX_PROPERTY_VALUE_LEN / 4 explanation (XGetWindowProperty manpage):
     *
     * long_length = Specifies the length in 32-bit multiples of the
     *               data to be retrieved.
     */
    XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
            xa_prop_type, &xa_ret_type, &ret_format,
            &ret_nitems, &ret_bytes_after, &ret_prop);

    char* ret_ptr = static_cast<char*>(static_cast<void*>(ret_prop));
    ret.assign(ret_ptr, ret_nitems);

    XFree(ret_prop);
    return ret;
}

static std::vector<Window> get_client_array(Display *disp) {
    std::vector<Window> client_list;

    client_list = get_property_as_window(disp, DefaultRootWindow(disp),
                        XA_WINDOW, "_NET_CLIENT_LIST");

    if (!client_list.size()) {
        client_list = get_property_as_window(disp, DefaultRootWindow(disp),
                                XA_CARDINAL, "_WIN_CLIENT_LIST");
    }

    return client_list;
}

static void setup_display_topmost(const std::string& title, Window* cam_window){
    Display* disp = XOpenDisplay(nullptr);
    auto tmp_arr = get_client_array(disp);

    for (size_t i = 0; i < tmp_arr.size(); ++i){
        std::string tmp(get_property_as_string(disp, tmp_arr[i], XA_STRING, "WM_NAME"));
        if (!title.compare(tmp)) {
           (*cam_window) = tmp_arr[i];
           XSetWindowAttributes xswa;
           xswa.override_redirect=True;
           XChangeWindowAttributes(disp, tmp_arr[i], CWOverrideRedirect, &xswa);
           XRaiseWindow(disp, tmp_arr[i]);
           break;
       }
    }

    XCloseDisplay(disp);
}

#endif
} // namespace


void display::run_display()
{
    while (!stop_flag) {
        AVFrame *tmp_frame = get_frame_from_keeper();
        display_frame(tmp_frame);
        av_frame_free(&tmp_frame);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if( event.type == SDL_QUIT ) break;
        }
        set_display_topmost();
    }
    stop_flag.exchange(false);
}

std::string display::wait_result(){
    sync_point.flag.exchange(false);
    std::unique_lock<std::mutex> lck(cond_mtx);
    while (!sync_point.flag) {
        std::cv_status status = sync_point.cond.wait_for(lck, std::chrono::seconds(4));
        if (std::cv_status::timeout == status) {
            break;
        }
    }
    return init_result;
}

void display::send_result(std::string result) {
    init_result = result;
    sync_point.flag.exchange(true);
    sync_point.cond.notify_all();
}

AVFrame *display::get_frame_from_keeper() {
    std::lock_guard<std::mutex> guard(mtx);
    return keeper->get_frame();
}

std::string display::construct_error(std::string what){
    std::string error("{ \"error\": \"");
    error += what;
    error += "\"}";
    return error;
}

display::display(display_settings set)
    : renderer(nullptr), screen(nullptr), texture(nullptr), title(set.title),
      init_result("can't init"), initialized(false), width(set.width),
      height(set.height), pos_x(set.pos_x), pos_y(set.pos_y)
{            
    stop_flag.exchange(false);
#ifdef WIN32
#else
//    cam_window = nullptr;
#endif
}
display::~display() {
    stop_display();
}

std::string display::init()
{
    const int default_screen_pos = 100;
    int screen_pos_x = (-1 != pos_x) ? pos_x : default_screen_pos ;
    int screen_pos_y = (-1 != pos_y) ? pos_y : default_screen_pos ;

    if(SDL_Init(SDL_INIT_VIDEO)) {
        return construct_error("Could not initialize SDL - " + std::string(SDL_GetError()));
    }

    screen = SDL_CreateWindow(title.c_str(), screen_pos_x, screen_pos_y, width, height,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    if(!screen) {
        SDL_Quit();
        return construct_error("SDL: could not set video mode - exiting");
    }

    renderer = SDL_CreateRenderer(screen, -1, 0);
    if (!renderer) {
        SDL_DestroyWindow(screen);
        SDL_Quit();
        return construct_error("SDL: could not create renderer - exiting");
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                            SDL_TEXTUREACCESS_STATIC, width, height);
    if (!texture) {
        SDL_DestroyWindow(screen);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return construct_error("SDL: could not create texture - exiting");
    }

#ifdef WIN32
    setup_display_topmost(title, cam_window);
#else
    setup_display_topmost(title, std::addressof(cam_window));
#endif

    initialized = true;
    return std::string{};
}

std::string display::start_display()
{
    if (keeper){
        display_thread = std::thread([this] {
            send_result(init());
            this->run_display();
        });
        return wait_result();
    }
    return std::string{"{ \"error\": \"Decoder not setted\"}"};
}

void display::stop_display()
{
    stop_flag.exchange(true);
    if (display_thread.joinable()) display_thread.join();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    SDL_Quit();
}

bool display::is_initialized() const {
    return initialized;
}

void display::display_frame(AVFrame *frame)
{
    if (nullptr == frame) {
        return;
    }

    struct SwsContext* sws_ctx;
    AVPixelFormat format = static_cast<AVPixelFormat>(frame->format);
    AVPixelFormat new_format = AV_PIX_FMT_YUV420P;

    sws_ctx = sws_getContext
    (
        frame->width,
        frame->height,
        format,
        this->width,
        this->height,
        new_format,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    AVFrame *frame_rgb = av_frame_alloc();
    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(new_format, this->width,
                    this->height);
    uint8_t* buffer=new uint8_t[numBytes*sizeof(uint8_t)];

    //setup buffer for new frame
    avpicture_fill((AVPicture *)frame_rgb, buffer, new_format,
           this->width, this->height);

    // setup frame sizes
    frame_rgb->width = width;
    frame_rgb->height = height;
    frame_rgb->format = new_format;
    frame_rgb->pts = 0;

    // rescale frame to frameRGB
    sws_scale(sws_ctx,
        ((AVPicture*)frame)->data,
        ((AVPicture*)frame)->linesize,
        0,
        frame->height,
        ((AVPicture *)frame_rgb)->data,
        ((AVPicture *)frame_rgb)->linesize);

    SDL_UpdateYUVTexture(
        texture,
        NULL,
        frame_rgb->data[0], //vp->yPlane,
        frame_rgb->linesize[0],
        frame_rgb->data[1], //vp->yPlane,
        frame_rgb->linesize[1],
        frame_rgb->data[2], //vp->yPlane,
        frame_rgb->linesize[2]
    );

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    delete[] buffer;
    sws_freeContext(sws_ctx);
    av_frame_free(&frame_rgb);
}

void display::set_display_topmost(){
#ifdef WIN32
    SetWindowPos(cam_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#else
    Display* disp = XOpenDisplay(nullptr);
    XRaiseWindow(disp, cam_window);
    XCloseDisplay(disp);
#endif
}

void display::setup_frame_keeper(std::shared_ptr<frame_keeper> keeper){
    std::lock_guard<std::mutex> guard(mtx);
    this->keeper = keeper;
}
