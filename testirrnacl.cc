#include <cassert>
#include <cstdio>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <sys/time.h>

// NaCl
#include "ppapi/c/ppb_opengles2.h"
#include "ppapi/gles2/gl2ext_ppapi.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/cpp/graphics_3d_client.h"
#include "ppapi/cpp/file_system.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/graphics_3d.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/size.h"
#include "ppapi/cpp/var.h"

#include <GLES2/gl2.h>
#include <vector>

#include "game.h"

#include <nacl-mounts/base/KernelProxy.h>
#include <nacl-mounts/base/MainThreadRunner.h>
#include <nacl-mounts/http2/HTTP2Mount.h>
#include <nacl-mounts/pepper/PepperMount.h>

std::string ModifierToString(uint32_t modifier) {
  std::string s;
  if (modifier & PP_INPUTEVENT_MODIFIER_SHIFTKEY) {
    s += "shift ";
  }
  if (modifier & PP_INPUTEVENT_MODIFIER_CONTROLKEY) {
    s += "ctrl ";
  }
  if (modifier & PP_INPUTEVENT_MODIFIER_ALTKEY) {
    s += "alt ";
  }
  if (modifier & PP_INPUTEVENT_MODIFIER_METAKEY) {
    s += "meta ";
  }
  if (modifier & PP_INPUTEVENT_MODIFIER_ISKEYPAD) {
    s += "keypad ";
  }
  if (modifier & PP_INPUTEVENT_MODIFIER_ISAUTOREPEAT) {
    s += "autorepeat ";
  }
  if (modifier & PP_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN) {
    s += "left-button-down ";
  }
  if (modifier & PP_INPUTEVENT_MODIFIER_MIDDLEBUTTONDOWN) {
    s += "middle-button-down ";
  }
  if (modifier & PP_INPUTEVENT_MODIFIER_RIGHTBUTTONDOWN) {
    s += "right-button-down ";
  }
  if (modifier & PP_INPUTEVENT_MODIFIER_CAPSLOCKKEY) {
    s += "caps-lock ";
  }
  if (modifier & PP_INPUTEVENT_MODIFIER_NUMLOCKKEY) {
    s += "num-lock ";
  }
  return s;
}

std::string MouseButtonToString(PP_InputEvent_MouseButton button) {
  switch (button) {
    case PP_INPUTEVENT_MOUSEBUTTON_NONE:
      return "None";
    case PP_INPUTEVENT_MOUSEBUTTON_LEFT:
      return "Left";
    case PP_INPUTEVENT_MOUSEBUTTON_MIDDLE:
      return "Middle";
    case PP_INPUTEVENT_MOUSEBUTTON_RIGHT:
      return "Right";
    default:
      std::ostringstream stream;
      stream << "Unrecognized ("
             << static_cast<int32_t>(button)
             << ")";
      return stream.str();
  }
}

int						width, height;
Game					*game;
pp::Graphics3D			m_context;
static int				isFileLoaderDone, isTextureUpdated;

static void *FileLoader(void *arg);
void RendererCallBack(void* data, int32_t result);
static long startTime, endTime;

static long _getTime(void) {
	struct timeval  now;
	gettimeofday(&now, NULL);
	return (long)(now.tv_sec*1000 + now.tv_usec/1000);
}

class EventInstance : public pp::Instance, public pp::Graphics3DClient {
 public:
	explicit EventInstance(PP_Instance instance, pp::Module* module): pp::Instance(instance), pp::Graphics3DClient(this) {
		m_gles2 = static_cast<const struct PPB_OpenGLES2*>(module->GetBrowserInterface(PPB_OPENGLES2_INTERFACE));

		RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_WHEEL);
		RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
	}

	virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
		width = 640;
		height = 480;
		InitGL();
		game = new Game();
		FlushContext();
		LoadFiles();
		return true;
	}

	virtual ~EventInstance() {
		if (runner)
			delete runner;
	}

	void LoadFiles() {
	    runner = new MainThreadRunner(this);
		isFileLoaderDone = false;
		isTextureUpdated = false;
		pthread_create(&fileLoaderThread, NULL, FileLoader, this);
	}

	void DidChangeFocus(bool focus) {
		PostMessage(pp::Var("DidChangeFocus"));
	}

	void DidChangeView(const pp::Rect& position, const pp::Rect& clip) {
		game->Resize(position.size().width(), position.size().height());
	}

	void GotKeyEvent(const pp::KeyboardInputEvent& key_event, const std::string& kind) {
		std::ostringstream stream;
		stream << pp_instance() << ":"
								<< " Key event:" << kind
								<< " modifier:" << ModifierToString(key_event.GetModifiers())
								<< " key_code:" << key_event.GetKeyCode()
								<< " time:" << key_event.GetTimeStamp()
								<< " text:" << key_event.GetCharacterText().DebugString()
								<< "\n";
		PostMessage(stream.str());
	}

	void GotMouseEvent(const pp::MouseInputEvent& mouse_event, const std::string& kind) {
		std::ostringstream stream;
		stream << pp_instance() << ":"
								<< " Mouse event:" << kind
								<< " modifier:" << ModifierToString(mouse_event.GetModifiers())
								<< " button:" << MouseButtonToString(mouse_event.GetButton())
								<< " x:" << mouse_event.GetPosition().x()
								<< " y:" << mouse_event.GetPosition().y()
								<< " click_count:" << mouse_event.GetClickCount()
								<< " time:" << mouse_event.GetTimeStamp()
								<< "\n";
		PostMessage(stream.str());
	}

	void GotWheelEvent(const pp::WheelInputEvent& wheel_event) {
		std::ostringstream stream;
		stream << pp_instance() << ": Wheel event."
								<< " modifier:" << ModifierToString(wheel_event.GetModifiers())
								<< " deltax:" << wheel_event.GetDelta().x()
								<< " deltay:" << wheel_event.GetDelta().y()
								<< " wheel_ticks_x:" << wheel_event.GetTicks().x()
								<< " wheel_ticks_y:"<< wheel_event.GetTicks().y()
								<< " scroll_by_page: "
								<< (wheel_event.GetScrollByPage() ? "true" : "false")
								<< "\n";
		PostMessage(stream.str());
	}

	virtual bool HandleInputEvent(const pp::InputEvent& event) {
		switch (event.GetType()) {
			case PP_INPUTEVENT_TYPE_UNDEFINED:
				break;
			case PP_INPUTEVENT_TYPE_MOUSEDOWN: {
					pp::MouseInputEvent mouse_event = pp::MouseInputEvent(event);
					if(mouse_event.GetButton() == PP_INPUTEVENT_MOUSEBUTTON_LEFT) {
						game->MouseDown(mouse_event.GetPosition().x(), mouse_event.GetPosition().y());
					}
					GotMouseEvent(mouse_event, "Down");
				}
				break;
			case PP_INPUTEVENT_TYPE_MOUSEUP: {
					pp::MouseInputEvent mouse_event = pp::MouseInputEvent(event);
					if(mouse_event.GetButton() == PP_INPUTEVENT_MOUSEBUTTON_LEFT) {
						game->MouseUp(mouse_event.GetPosition().x(), mouse_event.GetPosition().y());
					}
					GotMouseEvent(mouse_event, "Up");
				}
				break;
			case PP_INPUTEVENT_TYPE_MOUSEMOVE: {
					pp::MouseInputEvent mouse_event = pp::MouseInputEvent(event);
					if(mouse_event.GetButton() == PP_INPUTEVENT_MOUSEBUTTON_LEFT) {
						game->MouseMove(mouse_event.GetPosition().x(), mouse_event.GetPosition().y());
					}
					GotMouseEvent(mouse_event, "Move");
				}
				break;
			case PP_INPUTEVENT_TYPE_MOUSEENTER:
				GotMouseEvent(pp::MouseInputEvent(event), "Enter");
				break;
			case PP_INPUTEVENT_TYPE_MOUSELEAVE:
				GotMouseEvent(pp::MouseInputEvent(event), "Leave");
				break;
			case PP_INPUTEVENT_TYPE_WHEEL:
				GotWheelEvent(pp::WheelInputEvent(event));
				break;
			case PP_INPUTEVENT_TYPE_RAWKEYDOWN:
				GotKeyEvent(pp::KeyboardInputEvent(event), "RawKeyDown");
				break;
			case PP_INPUTEVENT_TYPE_KEYDOWN:
				GotKeyEvent(pp::KeyboardInputEvent(event), "Down");
				break;
			case PP_INPUTEVENT_TYPE_KEYUP:
				GotKeyEvent(pp::KeyboardInputEvent(event), "Up");
				break;
			case PP_INPUTEVENT_TYPE_CHAR:
				GotKeyEvent(pp::KeyboardInputEvent(event), "Character");
				break;
			case PP_INPUTEVENT_TYPE_CONTEXTMENU:
				GotKeyEvent(pp::KeyboardInputEvent(event), "Context");
				break;
			// Note that if we receive an IME event we just send a message back
			// to the browser to indicate we have received it.
			case PP_INPUTEVENT_TYPE_IME_COMPOSITION_START:
				PostMessage(pp::Var("PP_INPUTEVENT_TYPE_IME_COMPOSITION_START"));
				break;
			case PP_INPUTEVENT_TYPE_IME_COMPOSITION_UPDATE:
				PostMessage(pp::Var("PP_INPUTEVENT_TYPE_IME_COMPOSITION_UPDATE"));
				break;
			case PP_INPUTEVENT_TYPE_IME_COMPOSITION_END:
				PostMessage(pp::Var("PP_INPUTEVENT_TYPE_IME_COMPOSITION_END"));
				break;
			case PP_INPUTEVENT_TYPE_IME_TEXT:
				PostMessage(pp::Var("PP_INPUTEVENT_TYPE_IME_COMPOSITION_TEXT"));
				break;
			default:
				assert(false);
				return false;
		}
		return true;
	}

	virtual void Graphics3DContextLost() {
		PostMessage(pp::Var("Lost Graphics Context\n"));
	}

	void InitGL() {
		if (m_context.is_null()) {
			int32_t attribs[] = {
				PP_GRAPHICS3DATTRIB_ALPHA_SIZE,			8,
				PP_GRAPHICS3DATTRIB_DEPTH_SIZE,			24,
				PP_GRAPHICS3DATTRIB_STENCIL_SIZE,		8,
				PP_GRAPHICS3DATTRIB_SAMPLES,			0,
				PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS,		0,
				PP_GRAPHICS3DATTRIB_WIDTH,				width,
				PP_GRAPHICS3DATTRIB_HEIGHT,				height,
				PP_GRAPHICS3DATTRIB_NONE
			};
			m_context = pp::Graphics3D(this, pp::Graphics3D(), attribs);
			if (m_context.is_null()) {
				glSetCurrentContextPPAPI(0);
				return;
			}
			BindGraphics(m_context);
		}
		glSetCurrentContextPPAPI(m_context.pp_resource());
	}

	void DrawSelf() {
		if(isFileLoaderDone && !isTextureUpdated) {
			isTextureUpdated = true;
			game->PrepareOpenGL(width, height);
		}
		game->Draw();
		FlushContext();
	}

	void FlushContext() {
		pp::CompletionCallback cc = pp::CompletionCallback(RendererCallBack, this);
		m_context.SwapBuffers(cc);
	}

	MainThreadRunner* runner;

private:
	const struct PPB_OpenGLES2* m_gles2;
	pthread_t fileLoaderThread;
};

void LoadFile(const char* url, const char* filename, MainThreadRunner* runner) {
	UrlLoaderJob* job = new UrlLoaderJob;
	job->set_url(url);
	std::vector<char> data;
	job->set_dst(&data);
	runner->RunJob(job);
	int fh = open(filename, O_CREAT | O_WRONLY);
	write(fh, &data[0], data.size());
	close(fh);
}

void *FileLoader(void *data) {
	EventInstance *eventInstance = static_cast<EventInstance*>(data);

	setenv("HOME", "/myhome", 1);
	setenv("TERM", "xterm-color", 1);
	setenv("USER", "", 1);
	setenv("LOGNAME", "", 1);

	mkdir("/ccdata", 0777);
	chdir("/ccdata");
	{
		LoadFile("shaders/COGLES2FixedPipeline.fsh", "COGLES2FixedPipeline.fsh", eventInstance->runner);
		LoadFile("shaders/COGLES2FixedPipeline.vsh", "COGLES2FixedPipeline.vsh", eventInstance->runner);
		LoadFile("shaders/COGLES2NormalMap.fsh", "COGLES2NormalMap.fsh", eventInstance->runner);
		LoadFile("shaders/COGLES2NormalMap.vsh", "COGLES2NormalMap.vsh", eventInstance->runner);
		LoadFile("shaders/COGLES2ParallaxMap.fsh", "COGLES2ParallaxMap.fsh", eventInstance->runner);
		LoadFile("shaders/COGLES2ParallaxMap.vsh", "COGLES2ParallaxMap.vsh", eventInstance->runner);
		LoadFile("shaders/COGLES2Renderer2D.fsh", "COGLES2Renderer2D.fsh", eventInstance->runner);
		LoadFile("shaders/COGLES2Renderer2D.vsh", "COGLES2Renderer2D.vsh", eventInstance->runner);
		LoadFile("builtInFont.bmp", "builtInFont.bmp", eventInstance->runner);
		LoadFile("sydney.zip", "sydney.zip", eventInstance->runner);
	}

	isFileLoaderDone = true;
	return 0;
}

void RendererCallBack(void* data, int32_t result) {
	EventInstance *eventInstance = static_cast<EventInstance*>(data);
//	eventInstance->PostMessage(pp::Var("flush_callback\n"));
	eventInstance->DrawSelf();
	/*
	endTime = _getTime();
	std::ostringstream stream;
	stream	<< "Current FPS = "
			<< 1000.0/(endTime - startTime)
			<< "\n";
	eventInstance->PostMessage(stream.str());
	startTime = _getTime();*/
}

class EventModule : public pp::Module {
public:
	EventModule() : pp::Module() {}
	virtual ~EventModule() {
		glTerminatePPAPI();
	}

	virtual bool Init() {
		return glInitializePPAPI(get_browser_interface()) == GL_TRUE;
	}

	virtual pp::Instance* CreateInstance(PP_Instance instance) {
		return new EventInstance(instance, this);
	}
};

namespace pp {
	Module* CreateModule() {
		return new EventModule();
	}
}
