#include "ofAppGlutWindow.h"
#include "engine.h"
#include "constants.h"

//--------------------------------------------------------------
int main(int argc, char **argv){
	#ifndef __linux
	ofAppGlutWindow window; // create a window
	// set width, height, mode (OF_WINDOW or OF_FULLSCREEN)
	if (_DEBUG_) {
		ofSetupOpenGL(&window, WINDOW_SETTINGS::width, WINDOW_SETTINGS::height, OF_WINDOW);
	} else {
		ofSetupOpenGL(&window, WINDOW_SETTINGS::width, WINDOW_SETTINGS::height, OF_FULLSCREEN);
		ofHideCursor();
	}
	#else
	ofSetupOpenGL(WINDOW_SETTINGS::width, WINDOW_SETTINGS::height, OF_WINDOW);
	#endif
	std::string world_name;
	bool automaticTest = false;
	for (int i = 1; i<argc; ++i){
		if (argv[i][0] == '-'){
			if (std::string(argv[i])=="-auto"){
				automaticTest = true;
			}
		} else {
			world_name = argv[i];
		}
	}
	Engine * engine = new Engine(world_name, automaticTest);
	ofRunApp(engine); // start the app
}
