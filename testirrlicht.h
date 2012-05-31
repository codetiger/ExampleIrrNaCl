#ifndef __IRRLICHT_DEMO_H
#define __IRRLICHT_DEMO_H

#include <irrlicht.h>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

bool setupGraphics(int screenWidth, int screenHeight);
void renderFrame();
void shutdownGraphics();

#endif //__IRRLICHT_DEMO_H