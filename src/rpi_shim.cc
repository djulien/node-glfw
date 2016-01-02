#include "common.h"

// Includes
#include <cstdio>
#include <cstdlib>

using namespace v8;
using namespace node;

#include <iostream>
#include <sstream>

using namespace std;

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <bcm_host.h>
#include <time.h>

typedef struct
{
  uint32_t screen_width;
  uint32_t screen_height;

  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
} OGL_STATE;

static OGL_STATE _state;

static void ogl_init(OGL_STATE *state, int width, int height)
{
  // code adapted from: https://github.com/peepo/openGL-RPi-tutorial/blob/master/encode_OGL/src/ogl.c

  //TODO: add support for multiple displays
  //TODO: make sure to pay attention to this: http://directx.com/2014/06/egl-understanding-eglchooseconfig-then-ignoring-it/
  //TODO: error handling

  int32_t success = 0;
  EGLBoolean result;
  EGLint num_config;

  bcm_host_init();

  static EGL_DISPMANX_WINDOW_T nativewindow;

  DISPMANX_ELEMENT_HANDLE_T dispman_element;
  DISPMANX_DISPLAY_HANDLE_T dispman_display;
  DISPMANX_UPDATE_HANDLE_T dispman_update;

  VC_RECT_T dst_rect;
  VC_RECT_T src_rect;

  static const EGLint attribute_list[] = {
    // 32 bit color
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,

    // depth buffer
    EGL_DEPTH_SIZE, 24,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,

    // OpenGL ES 2.x context
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,

    EGL_NONE
  };

  static const EGLint context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  EGLConfig config;

  // get an EGL display connection
  state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  // initialize the EGL display connection
  result = eglInitialize(state->display, NULL, NULL);

  // get an appropriate EGL frame buffer configuration
  result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
  assert(EGL_FALSE != result);

  // get an appropriate EGL frame buffer configuration
  result = eglBindAPI(EGL_OPENGL_ES_API);
  assert(EGL_FALSE != result);


  // create an EGL rendering context
  state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
  assert(state->context != EGL_NO_CONTEXT);

  // create an EGL window surface
  success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
  assert(success >= 0);

  if (width > 0)
    state->screen_width = width;
  else
    width = state->screen_width;

  if (height > 0)
    state->screen_height = height;
  else
    height = state->screen_height;

  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.width = state->screen_width;
  dst_rect.height = state->screen_height;

  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.width = state->screen_width << 16;
  src_rect.height = state->screen_height << 16;

  dispman_display = vc_dispmanx_display_open(0 /*LCD*/);
  dispman_update = vc_dispmanx_update_start(0);

  dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display,
      0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE,
      0/*alpha*/, 0/*clamp*/, DISPMANX_NO_ROTATE/*transform*/);

  nativewindow.element = dispman_element;
  nativewindow.width = state->screen_width;
  nativewindow.height = state->screen_height;
  vc_dispmanx_update_submit_sync(dispman_update);

  state->surface = eglCreateWindowSurface(state->display, config, &nativewindow, NULL);
  assert(state->surface != EGL_NO_SURFACE);


  // connect the context to the surface
  result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
  assert(EGL_FALSE != result);
}

static time_t _startSec;
static long _startNanoSec;

void time_init() {
  _startSec = 0;
  _startNanoSec = 0;
}

namespace glfw {

  /* @Module: EGL initialization, termination and version querying */

  NAN_METHOD(Init) {
    Nan::HandleScope scope;

    ogl_init(&_state, 0, 0);

    info.GetReturnValue().Set(JS_BOOL(true));
  }

  NAN_METHOD(Terminate) {
    Nan::HandleScope scope;
    //
    return;
  }


  NAN_METHOD(GetVersion) {
    Nan::HandleScope scope;
    int major, minor, rev;
    //glfwGetVersion(&major,&minor,&rev);
    major = 0;
    minor = 0;
    rev = 0;
    Local<Array> arr = Nan::New<Array>(3);
    arr->Set(JS_STR("major"), JS_INT(major));
    arr->Set(JS_STR("minor"), JS_INT(minor));
    arr->Set(JS_STR("rev"), JS_INT(rev));
    info.GetReturnValue().Set(arr);
  }

  NAN_METHOD(GetVersionString) {
    Nan::HandleScope scope;
    const char* ver = "TODO"; //glfwGetVersionString();
    info.GetReturnValue().Set(JS_STR(ver));
  }

  /* @Module: Time input */

  NAN_METHOD(GetTime) {
    Nan::HandleScope scope;

    double time;

    timespec rt;
    clock_gettime(CLOCK_REALTIME, &rt);

    if (_startSec == 0) {
      _startSec = rt.tv_sec;
      _startNanoSec = rt.tv_nsec;
      time = 0.0;
    }
    else {
      time = (double)(rt.tv_sec - _startSec);
      time += (double)(rt.tv_nsec - _startNanoSec) / 1000000000.0;
    }

    info.GetReturnValue().Set(JS_NUM(time));
  }

  NAN_METHOD(SetTime) {
    Nan::HandleScope scope;
    //double time = info[0]->NumberValue();
    //glfwSetTime(time);
    return;
  }

  /* @Module: monitor handling */
  // TODO: GetMonitors

  /* @Module Context handling */

  NAN_METHOD(MakeContextCurrent) {
    Nan::HandleScope scope;
    //  uint64_t handle=info[0]->IntegerValue();
    //  if(handle) {
    //    GLFWwindow* window = reinterpret_cast<GLFWwindow*>(handle);
    //    glfwMakeContextCurrent(window);
    //  }
    return;
  }

  // TODO: MakeContextCurrent
  // TODO: GetCurrentContext

  NAN_METHOD(SwapBuffers) {
    Nan::HandleScope scope;
    eglSwapBuffers(_state.display, _state.surface);
    /*
      uint64_t handle=info[0]->IntegerValue();
      if(handle) {
      GLFWwindow* window = reinterpret_cast<GLFWwindow*>(handle);
      glfwSwapBuffers(window);
      }*/
    return;
  }

  NAN_METHOD(SwapInterval) {
    Nan::HandleScope scope;
    //  int interval=info[0]->Int32Value();
    //  glfwSwapInterval(interval);
    return;
  }

  /* Extension support */
  //TODO: ExtensionSupported

  // make sure we close everything when we exit
  void AtExit() {
    //TwTerminate();
    //glfwTerminate();
  }

  NAN_METHOD(glfw_CreateWindow) {
    Nan::HandleScope scope;
    /*
      int width       = info[0]->Uint32Value();
      int height      = info[1]->Uint32Value();
      String::Utf8Value str(info[2]->ToString());
      int monitor_idx = info[3]->Uint32Value();
    */
    //TODO: make this actually do something
    int window = 1;

    info.GetReturnValue().Set(JS_NUM((uint64_t)window));
  }

  NAN_METHOD(GetWindowSize) {
    Nan::HandleScope scope;
    uint64_t handle = info[0]->IntegerValue();
    if (handle) {
      int w = _state.screen_width;
      int h = _state.screen_height;
      //    GLFWwindow* window = reinterpret_cast<GLFWwindow*>(handle);
      //    glfwGetWindowSize(window, &w, &h);
      Local<Array> arr = Nan::New<Array>(2);
      arr->Set(JS_STR("width"), JS_INT(w));
      arr->Set(JS_STR("height"), JS_INT(h));
      info.GetReturnValue().Set(arr);
    }
    return;
  }

  NAN_METHOD(GetFramebufferSize) {
    Nan::HandleScope scope;
    uint64_t handle = info[0]->IntegerValue();
    if (handle) {
      //    GLFWwindow* window = reinterpret_cast<GLFWwindow*>(handle);
      int w = _state.screen_width;
      int h = _state.screen_height;
      //    glfwGetFramebufferSize(window, &width, &height);
      Local<Array> arr = Nan::New<Array>(2);
      arr->Set(JS_STR("width"), JS_INT(w));
      arr->Set(JS_STR("height"), JS_INT(h));
      info.GetReturnValue().Set(arr);
    }
    return;
  }


  NAN_METHOD(SetWindowTitle) {
    Nan::HandleScope scope;
    /*
      uint64_t handle=info[0]->IntegerValue();
      String::Utf8Value str(info[1]->ToString());
      if(handle) {
      GLFWwindow* window = reinterpret_cast<GLFWwindow*>(handle);
      glfwSetWindowTitle(window, *str);
      }*/
    return;
  }

  NAN_METHOD(WindowHint) {
    Nan::HandleScope scope;
    //  int target       = info[0]->Uint32Value();
    //  int hint         = info[1]->Uint32Value();
    //  glfwWindowHint(target, hint);
    return;
  }

  NAN_METHOD(DefaultWindowHints) {
    Nan::HandleScope scope;
    //  glfwDefaultWindowHints();
    return;
  }

  NAN_METHOD(PollEvents) {
    Nan::HandleScope scope;
    //glfwPollEvents();
    return;
  }

} // namespace glfw

///////////////////////////////////////////////////////////////////////////////
//
// bindings
//
///////////////////////////////////////////////////////////////////////////////
#define JS_GLFW_CONSTANT(name) target->Set(JS_STR( #name ), JS_INT(GLFW_ ## name))
#define JS_GLFW_SET_METHOD(name) Nan::SetMethod(target, #name , glfw::name);

extern "C" {
  NAN_MODULE_INIT(init)
  {
    atexit(glfw::AtExit);
    time_init();

    Nan::HandleScope scope;

    /* EGL initialization, termination and version querying */
    JS_GLFW_SET_METHOD(Init);
    JS_GLFW_SET_METHOD(Terminate);
    JS_GLFW_SET_METHOD(GetVersion);
    JS_GLFW_SET_METHOD(GetVersionString);

    /* Time */
    JS_GLFW_SET_METHOD(GetTime);
    //JS_GLFW_SET_METHOD(SetTime);

    /* Monitor handling */
    //JS_GLFW_SET_METHOD(GetMonitors);

    /* Window handling */
    //JS_GLFW_SET_METHOD(CreateWindow);

    Nan::SetMethod(target, "CreateWindow", glfw::glfw_CreateWindow);

    JS_GLFW_SET_METHOD(WindowHint);
    JS_GLFW_SET_METHOD(DefaultWindowHints);
    /*
    JS_GLFW_SET_METHOD(DestroyWindow);
    JS_GLFW_SET_METHOD(SetWindowShouldClose);
    JS_GLFW_SET_METHOD(WindowShouldClose);
    */
    JS_GLFW_SET_METHOD(SetWindowTitle);
    JS_GLFW_SET_METHOD(GetWindowSize);
    /*
    JS_GLFW_SET_METHOD(SetWindowSize);
    JS_GLFW_SET_METHOD(SetWindowPos);
    JS_GLFW_SET_METHOD(GetWindowPos);
    */
    JS_GLFW_SET_METHOD(GetFramebufferSize);
    /*
    JS_GLFW_SET_METHOD(IconifyWindow);
    JS_GLFW_SET_METHOD(RestoreWindow);
    JS_GLFW_SET_METHOD(ShowWindow);
    JS_GLFW_SET_METHOD(HideWindow);
    JS_GLFW_SET_METHOD(GetWindowAttrib);
    */
    JS_GLFW_SET_METHOD(PollEvents);
    /*
    JS_GLFW_SET_METHOD(WaitEvents);
    */

	/* Input handling */
    /*
    JS_GLFW_SET_METHOD(GetKey);
    JS_GLFW_SET_METHOD(GetMouseButton);
    JS_GLFW_SET_METHOD(GetCursorPos);
    JS_GLFW_SET_METHOD(SetCursorPos);
    */
    /* Context handling */
    JS_GLFW_SET_METHOD(MakeContextCurrent);
    //  JS_GLFW_SET_METHOD(GetCurrentContext);
    JS_GLFW_SET_METHOD(SwapBuffers);
    JS_GLFW_SET_METHOD(SwapInterval);
    //  JS_GLFW_SET_METHOD(ExtensionSupported);

    /* Joystick */
    /*
    JS_GLFW_SET_METHOD(JoystickPresent);
    JS_GLFW_SET_METHOD(GetJoystickAxes);
    JS_GLFW_SET_METHOD(GetJoystickButtons);
    JS_GLFW_SET_METHOD(GetJoystickName);
    */
  }

  NODE_MODULE(glfw, init)
}
