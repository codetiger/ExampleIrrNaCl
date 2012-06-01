#include "testirrlicht.h"

IrrlichtDevice *device;
IVideoDriver* driver;
ISceneManager* smgr;
IGUIEnvironment* guienv;


bool setupGraphics(int screenWidth, int screenHeight) 
{
	printf("Hello world NACL");
	device = createDevice( video::EDT_OGLES2, dimension2d<u32>(640, 480), 16, false, false, false, 0);
    driver = device->getVideoDriver();
	smgr = device->getSceneManager();
    guienv = device->getGUIEnvironment();
    guienv->addStaticText(L"Hello World! This is the Irrlicht Software renderer!", rect<s32>(10,10,260,22), true);
    smgr->addCameraSceneNode(0, vector3df(0,30,-40), vector3df(0,5,0));
	return true;
}

void renderFrame() 
{
		driver->beginScene(true, true, SColor(255,100,101,140));
        smgr->drawAll();
		guienv->drawAll();
        driver->endScene();
}

void shutdownGraphics()
{

}
