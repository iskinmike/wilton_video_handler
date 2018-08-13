
#include "display.hpp"
#include "frame_keeper.hpp"

#include <iostream>
#ifdef WIN32
#include <windows.h>
#include <cstdint>
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#define MAX_PROPERTY_VALUE_LEN 4096
#endif
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

#else
static char *get_property (Display *disp, Window win,
        Atom xa_prop_type, const char *prop_name, unsigned long *size) {
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop;
    char *ret;

    xa_prop_name = XInternAtom(disp, prop_name, False);

    /* MAX_PROPERTY_VALUE_LEN / 4 explanation (XGetWindowProperty manpage):
     *
     * long_length = Specifies the length in 32-bit multiples of the
     *               data to be retrieved.
     */
    XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
            xa_prop_type, &xa_ret_type, &ret_format,
            &ret_nitems, &ret_bytes_after, &ret_prop);

    /* null terminate the result to make string handling easier */
    tmp_size = (ret_format / (32 / sizeof(long))) * ret_nitems;
    ret = (char*) malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    if (size) {
        *size = tmp_size;
    }

    XFree(ret_prop);
    return ret;
}


static Window *get_client_list (Display *disp, unsigned long *size) {
    Window *client_list;

    if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp),
                    XA_WINDOW, "_NET_CLIENT_LIST", size)) == NULL) {
        if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp),
                        XA_CARDINAL, "_WIN_CLIENT_LIST", size)) == NULL) {
            return NULL;
        }
    }

    return client_list;
}
#endif
} // namespace


void display::run_display()
{
    frame_keeper& fk = frame_keeper::instance();
    while (!stop_flag) {
        AVFrame *tmp_frame = fk.get_current_frame();
        display_frame(tmp_frame);
        av_frame_free(&tmp_frame);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if( event.type == SDL_QUIT ) break;
        }
    }
    stop_flag.exchange(false);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    SDL_Quit();
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

display::display(const std::string& title)
    : renderer(NULL), screen(NULL), texture(NULL), title(title),
      init_result("can't init"), initialized(false) 
{            
    stop_flag.exchange(false);
}
display::~display() {
    stop_display();
}

std::string display::init(int pos_x, int pos_y, int width, int height)
{
    this->width = width;
    this->height = height;

    const int default_screen_pos = 100;
    int screen_pos_x = (-1 != pos_x) ? pos_x : default_screen_pos ;
    int screen_pos_y = (-1 != pos_y) ? pos_y : default_screen_pos ;

    if(SDL_Init(SDL_INIT_VIDEO)) {
      return std::string("Could not initialize SDL - ") + std::string(SDL_GetError());
    }

    screen = SDL_CreateWindow(title.c_str(), screen_pos_x, screen_pos_y, width, height,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    if(!screen) {
        SDL_Quit();
        return std::string("SDL: could not set video mode - exiting");
    }

    renderer = SDL_CreateRenderer(screen, -1, 0);
    if (!renderer) {
        SDL_DestroyWindow(screen);
        SDL_Quit();
        return std::string("SDL: could not create renderer - exiting");
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                            SDL_TEXTUREACCESS_STATIC, width, height);
    if (!texture) {
        SDL_DestroyWindow(screen);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return std::string("SDL: could not create texture - exiting");
    }

#ifdef WIN32
	param_info info;
	info.title = title;
	info.window = 0;

	LONG_PTR title_cb = reinterpret_cast<std::uintptr_t>(&info);
    // EnumWindows calls EnumWindowsProc for each avaliable window handler until returns true and stops if false returned.
    // EnumWindows returns false if EnumWindowsProc return false and true if looks throught all windows.
	if (!EnumWindows(&EnumWindowsProc, title_cb)) {
    	if (0 != info.window) {
            // Set Window topmost, 0 - zeores are ignored by flags SWP_NOMOVE | SWP_NOSIZE
            SetWindowPos(info.window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    	}
    }
#else
	// оpen default display
	Display* disp = nullptr;
	disp = XOpenDisplay(NULL);

	Window *client_list;
	unsigned long client_list_size;

	client_list = get_client_list(disp, &client_list_size);

	for (unsigned long i = 0; i < client_list_size; i++) {
		Window *win = &client_list[i];
		if (nullptr == win) {
			continue;
		}
		std::string tmp(get_property(disp, client_list[i], XA_STRING, "WM_NAME", NULL));
		std::cout << tmp << std::endl;
		if (title.compare(tmp)) {
			XRaiseWindow(disp, *win);
			break;
		}
	}
#endif

    initialized = true;
    return std::string{};
}

std::string display::start_display(int pos_x, int pos_y, int width, int height)
{
    display_thread = std::thread([this, pos_x, pos_y, width, height] {
        send_result(init(pos_x, pos_y, width, height));
        this->run_display();
    });
    return wait_result();
}

void display::stop_display()
{
    stop_flag.exchange(true);
    if (display_thread.joinable()) display_thread.join();
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
