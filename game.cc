#include "game.h"

Game::Game()
	:width_(1), height_(1) {
}

Game::~Game() {
}

void Game::PrepareOpenGL(int width, int height) {
	width_ = width;
	height_ = height;
	device = createDevice(EDT_OGLES2, dimension2d<u32>(width_, height_), 32, false, false, false, 0);
    driver = device->getVideoDriver();
	smgr = device->getSceneManager();
    guienv = device->getGUIEnvironment();
	filesys = device->getFileSystem();
    smgr->addCameraSceneNode(0, vector3df(0,30,-40), vector3df(0,5,0));

	filesys->addFileArchive("sydney.zip");
	IAnimatedMesh* mesh = smgr->getMesh("sydney.md2");
	IAnimatedMeshSceneNode* node = smgr->addAnimatedMeshSceneNode(mesh);
	ITexture* sydneyTex = driver->getTexture("sydney.png");
	if (node) {
		node->setMaterialFlag(EMF_LIGHTING, false);
		node->setMD2Animation(scene::EMAT_STAND);
		node->setMaterialTexture( 0, sydneyTex);
	}
	_isResourcesLoaded = true;
}

void Game::Resize(int width, int height) {
	if(!_isResourcesLoaded)
		return;
	width_ = width;
	height_ = height;
	driver->OnResize(dimension2d<u32>(width_, height_));
}

void Game::Draw() {
	if(!_isResourcesLoaded)
		return;
	if(device->run()) {
		driver->beginScene(true, true, SColor(255,100,101,140));
		smgr->drawAll();
		guienv->drawAll();
		driver->endScene();
	}
}

void Game::MouseUp(int x, int y) {
	_isMouseDown = false;
}

void Game::MouseDown(int x, int y) {
	_isMouseDown = true;
}

void Game::MouseMove(int x, int y) {
	if(_isMouseDown) {
		//drag
	}
}
