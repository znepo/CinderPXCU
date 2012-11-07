/**
 * author andrewwright 
 * andrew@snepo.com
 */

#ifndef SNEPO_PXCUSERVICE_H
#define SNEPO_PXCUSERVICE_H

#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/gl.h"

#include "pxcupipeline.h"

using namespace ci;
using namespace ci::app;

namespace snepo {
namespace pxcu {

class PXCUService;

struct GeoNodeState
{
	bool active;
	PXCGesture::GeoNode node;
	Vec2f screen;

	GeoNodeState() : active(false), screen( Vec2f::zero() ) {};
};

typedef std::shared_ptr<GeoNodeState> GeoNodeStateRef;
typedef std::shared_ptr<PXCUService> PXCUServiceRef;
typedef std::function <void ( PXCGesture::Gesture gesture ) > GestureHandler;
typedef std::map < PXCGesture::GeoNode::Label, GeoNodeStateRef> GeoNodeStateMap;

class PXCUService
{	

protected:

	int 			_retryCount;
	PXCUPipeline 	_pipelineFlags;
	bool			_setupSucceeded;
	bool			_isInited;
	bool			_waitForFrames;

	uint8_t			*_colorMap;
	uint8_t 		*_labelMap;
	short			*_depthMap;
	short			*_irMap;
	float			*_uvMap;

	Vec2i			_colorMapSize;
	Vec2i			_labelMapSize;
	Vec2i			_depthMapSize;
	Vec2i			_irMapSize;
	Vec2i			_uvMapSize;

	gl::Texture 	_colorMapTexture;
	gl::Texture 	_labelMapTexture;
	gl::Texture 	_depthMapTexture;
	gl::Texture 	_irMapTexture;
	gl::Texture 	_uvMapTexture;

	GestureHandler	_gestureHandler;
	std::vector<PXCGesture::GeoNode::Label> _trackedGeoNodes;
	
	GeoNodeStateMap _geoNodeState;

	void init();
	void processGestures();
	Vec2f mapXY ( Vec2f position );

	// These are lifted from the openframeworks sample 
	// that comes with the Intel Perceptual Computing SDK
	void convertToImage ( uint8_t* dst, unsigned short* src, int w, int h, bool invert );
	bool checkImage ( uint8_t* image, int w, int h, uint8_t value );
	
	PXCUService ( PXCUPipeline pipelineFlags, int retryCount = 2 );

public:

	~PXCUService ( );

	virtual bool start();
	virtual bool update();
	virtual void close();

	bool isRunning ( ) { return _setupSucceeded && _isInited; };

	uint8_t* getRawColorMap() { return _colorMap; };
	uint8_t* getRawLabelMap() { return _labelMap; };
	short*	 getRawDepthMap() { return _depthMap; };
	short*	 getRawIRMap() { return _irMap; };
	float*	 getRawUVMap() { return _uvMap; };

	void setGestureHandler ( GestureHandler _handler );
	void removeGestureHandler ( );

	gl::Texture getColorMap() { return _colorMapTexture; }
	gl::Texture getLabelMap() { return _labelMapTexture; }
	gl::Texture getDepthMap() { return _depthMapTexture; }
	gl::Texture getIRMap() { return _irMapTexture; }
	gl::Texture getUVMap() { return _uvMapTexture; }

	Vec2i getColorMapSize() { return _colorMapSize; };
	Vec2i getLabelMapSize() { return _labelMapSize; };
	Vec2i getDepthMapSize() { return _depthMapSize; };
	Vec2i getIRMapSize() { return _irMapSize; };
	Vec2i getUVMapSize() { return _depthMapSize; };

	GeoNodeStateMap getGeoNodes() { return _geoNodeState; };
	GeoNodeStateRef getGeoNode ( PXCGesture::GeoNode::Label label ) { return _geoNodeState[label]; };

	void setTrackedGeoNodes ( std::vector<PXCGesture::GeoNode::Label> nodes );
	std::vector<PXCGesture::GeoNode::Label> getTrackedGeoNodes ( ) { return _trackedGeoNodes; }; 

	static PXCUServiceRef create ( PXCUPipeline pipelineFlags, int retryCount = 2 )
	{
		return PXCUServiceRef ( new PXCUService ( pipelineFlags, retryCount ) );
	};
};

}}


#endif