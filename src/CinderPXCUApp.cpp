#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#include "PXCUService.h"

using snepo::pxcu::PXCUService;
using snepo::pxcu::PXCUServiceRef;
using snepo::pxcu::GestureHandler;
using snepo::pxcu::GeoNodeStateMap;
using snepo::pxcu::GeoNodeStateRef;

class CinderPXCUApp : public AppBasic 
{

public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();

	void onGesture ( PXCGesture::Gesture gesture );

	// The geonode coordinates are relative to
	// the tracked images 320x240 dimensions,
	// so all coordinates will need to be multiplied
	// by Vec2f ( screenWidth / 320.0f, screenHeight / 240.0f );
	float screenScale;
	PXCUServiceRef pxcu;
};

void CinderPXCUApp::setup()
{
	screenScale = 2.0f;

	pxcu = PXCUService::create ( (PXCUPipeline) ( PXCU_PIPELINE_CAPTURE | PXCU_PIPELINE_GESTURE ), 2 );

	GestureHandler handler = bind ( &CinderPXCUApp::onGesture, this, _1 );
	pxcu->setGestureHandler ( handler );

	vector<PXCGesture::GeoNode::Label> trackedGeoNodes;
	trackedGeoNodes.push_back ( PXCGesture::GeoNode::LABEL_BODY_HAND_PRIMARY );
	trackedGeoNodes.push_back ( PXCGesture::GeoNode::LABEL_BODY_HAND_PRIMARY | PXCGesture::GeoNode::LABEL_FINGER_THUMB );
	trackedGeoNodes.push_back ( PXCGesture::GeoNode::LABEL_BODY_HAND_PRIMARY | PXCGesture::GeoNode::LABEL_FINGER_INDEX );
	trackedGeoNodes.push_back ( PXCGesture::GeoNode::LABEL_BODY_HAND_PRIMARY | PXCGesture::GeoNode::LABEL_FINGER_MIDDLE );
	trackedGeoNodes.push_back ( PXCGesture::GeoNode::LABEL_BODY_HAND_PRIMARY | PXCGesture::GeoNode::LABEL_FINGER_RING );
	trackedGeoNodes.push_back ( PXCGesture::GeoNode::LABEL_BODY_HAND_PRIMARY | PXCGesture::GeoNode::LABEL_FINGER_PINKY );

	pxcu->setTrackedGeoNodes ( trackedGeoNodes );

	bool running = pxcu->start();

	console() << "PXCURunning? " << running << endl;
}

void CinderPXCUApp::onGesture ( PXCGesture::Gesture gesture )
{
	console() << "Got gesture of type " << gesture.label << endl;
}

void CinderPXCUApp::mouseDown( MouseEvent event )
{
}

void CinderPXCUApp::update()
{
	pxcu->update();

}

void CinderPXCUApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::color ( 1, 1, 1 );

	if ( pxcu->getColorMap() )
	{
		gl::draw ( pxcu->getColorMap() );
	}

	GeoNodeStateMap stateMap = pxcu->getGeoNodes();

	for ( GeoNodeStateMap::iterator it = stateMap.begin(); it != stateMap.end(); ++it )
	{
		GeoNodeStateRef state = (*it).second;
		if ( state->active )
		{
			gl::color ( 1, 0, 0 );
			gl::drawSolidCircle ( state->screen * screenScale, 5 + ( state->node.openness / 100.0f ) * 30.0f );
		}
	}
}

CINDER_APP_BASIC( CinderPXCUApp, RendererGl )
