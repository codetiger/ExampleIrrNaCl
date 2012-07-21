#include <irrlicht.h>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

class Game {
public:
	Game();
	~Game();

	void PrepareOpenGL(int width, int height);
	void Resize(int width, int height);
	void Draw();
	void MouseUp(int x, int y);
	void MouseDown(int x, int y);
	void MouseMove(int x, int y);

	const int width() const {
		return width_;
	}

	const int height() const {
		return height_;
	}

private:
	IrrlichtDevice *device;
	IVideoDriver* driver;
	ISceneManager* smgr;
	IGUIEnvironment* guienv;
	IFileSystem* filesys;

	int width_;
	int height_;
	bool _isMouseDown;
	bool _isResourcesLoaded;
};
