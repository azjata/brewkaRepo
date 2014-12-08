#include "videoSphere.h"
#include "resourcemanager.h"

static ShaderResource shader_blend("Shaders/videoBlend/videoBlend.vert", "Shaders/videoBlend/videoBlend.frag", "", "");
static ShaderResource shader_noblend("Shaders/videoBlend/videoBlend.vert", "Shaders/videoBlend/videoBlend.frag", "", "#define NO_BLEND");

VideoSphere::VideoSphere(Engine *engine_, string videoSrc) : engine(engine_),
videoColor(NULL),
videoAlpha(NULL),
#ifdef _WIN32
dPlayerColor(NULL),
dPlayerAlpha(NULL),
#endif
sphere(NULL)
{

#ifdef NO_VIDEO
	return;
#endif

	videoColor	= new ofVideoPlayer();
	#ifdef _WIN32
	dPlayerColor = new ofDirectShowPlayer();
	ofPtr <ofBaseVideoPlayer> ptrColor(dPlayerColor);
	videoColor->setPlayer(ptrColor);
	#endif

	videoAlpha  = 0;
	#ifdef _WIN32
	dPlayerAlpha = 0;
	#endif

	sphere		= new ofSpherePrimitive();

    ofDisableArbTex();

	videoColor->loadMovie(videoSrc);
	videoColor->update();
    ofEnableArbTex();

	loopVideo(false);

	sphere->set(128, 32);
	sphere->rotate(180, 0, 0, 1);
}

VideoSphere::VideoSphere(Engine *engine, string videoSrc, string videoAlphaSrc){
#ifdef NO_VIDEO
	return;
#endif
	this->engine = engine;

	videoColor	= new ofVideoPlayer();
	videoAlpha	= new ofVideoPlayer();

	#ifdef _WIN32
	dPlayerColor = new ofDirectShowPlayer();
	ofPtr <ofBaseVideoPlayer> ptrColor(dPlayerColor);
	videoColor->setPlayer(ptrColor);
	dPlayerAlpha = new ofDirectShowPlayer();
	ofPtr <ofBaseVideoPlayer> ptrAlpha(dPlayerAlpha);
	videoAlpha->setPlayer(ptrAlpha);
	#endif

	sphere		= new ofSpherePrimitive();

    ofDisableArbTex();

	videoColor->loadMovie(videoSrc);
	videoAlpha->loadMovie(videoAlphaSrc);

	videoColor->update();
	videoAlpha->update();

    ofEnableArbTex();

	loopVideo(false);

	sphere->set(128, 32);
	sphere->rotate(180, 0, 0, 1);

}

VideoSphere::~VideoSphere(){
#ifdef NO_VIDEO
	return;
#endif
	delete videoColor;
	delete videoAlpha;
	/*
	#ifdef _WIN32
	delete dPlayerColor;
	delete dPlayerAlpha;
	#endif
	*/
	delete sphere;
}

void VideoSphere::play(){
#ifdef NO_VIDEO
	return;
#endif
	videoColor->setPaused(false);
	videoColor->play();
	if(videoAlpha != 0) {
		videoAlpha->setPaused(false);
		videoAlpha->play();
	}
}

void VideoSphere::pause(){
#ifdef NO_VIDEO
	return;
#endif
	videoColor->setPaused(true);
	if(videoAlpha != 0)
		videoAlpha->setPaused(true);
}

void VideoSphere::rewind(){
#ifdef NO_VIDEO
	return;
#endif
	videoColor->setFrame(0);
	if(videoAlpha != 0)
		videoAlpha->setFrame(0);
}

void VideoSphere::close() {
#ifdef NO_VIDEO
	return;
#endif
	videoColor->closeMovie();
	if (videoAlpha!=NULL) {
		videoAlpha->closeMovie();
	}
}

bool VideoSphere::isPlaying(){
#ifdef NO_VIDEO
	return false;
#endif
	return videoColor->isPlaying();
}

bool VideoSphere::hasFinished(){
#ifdef NO_VIDEO
	return false;
#endif
	return videoColor->getIsMovieDone();
}

void VideoSphere::setFrame(int frame)
{
#ifdef NO_VIDEO
	return;
#endif
	videoColor->setFrame(frame);
}

void VideoSphere::loopVideo(bool loop)
{
#ifdef NO_VIDEO
	return;
#endif
	if(loop)
	{
		#ifdef _WIN32
		dPlayerColor->setLoopState(OF_LOOP_NORMAL);
		#endif
		videoColor->setLoopState(OF_LOOP_NORMAL);
		if(videoAlpha != 0) {
			#ifdef _WIN32
			dPlayerAlpha->setLoopState(OF_LOOP_NORMAL);
			#endif
			videoAlpha->setLoopState(OF_LOOP_NORMAL);
		}
	}
	else {
		#ifdef _WIN32
		dPlayerColor->setLoopState(OF_LOOP_NONE);
		#endif
		videoColor->setLoopState(OF_LOOP_NONE);
		if(videoAlpha != 0) {
			#ifdef _WIN32
			dPlayerAlpha->setLoopState(OF_LOOP_NONE);
			#endif
			videoAlpha->setLoopState(OF_LOOP_NONE);
		}
	}
}

void VideoSphere::update(){
#ifdef NO_VIDEO
	return;
#endif

	if(videoColor->isLoaded())/// avoid the log spam when the video did not load.
	{
		videoColor->update();
		if(videoAlpha != 0 && videoAlpha->isLoaded())
			videoAlpha->update();
	}

	sphere->setPosition(engine->getCurrentWorld()->camera.getPosition());
}
void VideoSphere::draw(){
#ifdef NO_VIDEO
	return;
#endif
	if(!videoColor->isLoaded())return;/// avoid the log spam from setUniformTexture when the video did not load.

	ofShader &shader = videoAlpha ? shader_blend() : shader_noblend();

	unsigned int colorTextureID = videoColor->getTextureReference().getTextureData().textureID;
	unsigned int alphaTextureID = 0;

	if (videoAlpha != 0){
		alphaTextureID = videoAlpha->getTextureReference().getTextureData().textureID;
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}else{
		glDisable(GL_BLEND);
	}

	glDisable(GL_DEPTH_TEST);


	shader.begin();

	//shader().setUniformTexture("textureColor", GL_TEXTURE_2D, colorTextureID, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorTextureID);
	shader.setUniform1i("textureColor", 0);

	if (videoAlpha) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, alphaTextureID);
		shader.setUniform1i("textureAlpha", 1);
		//shader().setUniformTexture("textureAlpha", GL_TEXTURE_2D, alphaTextureID, 1);
	}

	sphere->draw();

	if (videoAlpha) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	shader.end();

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}
void VideoSphere::keyPressed(int key){
}
void VideoSphere::keyReleased(int key){
}
void VideoSphere::drawDebugOverlay(){
}
