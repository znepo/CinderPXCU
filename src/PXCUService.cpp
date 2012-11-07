/**
 * author andrewwright 
 * andrew@snepo.com
 */


#include "PXCUService.h"

using namespace snepo;
using namespace snepo::pxcu;
using namespace std;

PXCUService::PXCUService ( PXCUPipeline pipelineFlags, int retryCount ) : _pipelineFlags ( pipelineFlags ), _retryCount ( retryCount )
{
	init();
}

void PXCUService::init ( )
{
	_setupSucceeded = false;
	_isInited = false;
	_waitForFrames = false;

	_colorMap = 0;
	_labelMap = 0;
	_depthMap = 0;
	_irMap = 0;
	_uvMap = 0;

	_gestureHandler = 0;

	_colorMapSize = Vec2f::zero();
	_labelMapSize = Vec2f::zero();
	_depthMapSize = Vec2f::zero();
	_irMapSize = Vec2f::zero();
	_uvMapSize = Vec2f::zero();

}

bool PXCUService::start()
{
	int attempt = 0;

	// On my machine it takes 2 tries to init the service,
	// but this might not be necessary for others. 
	while ( attempt < _retryCount )
	{
		_setupSucceeded = PXCUPipeline_Init ( _pipelineFlags );
		if ( !_setupSucceeded )
		{
			Sleep ( 1000 );
			attempt++;
		}else
		{
			break;
		}
	}

	if ( !_setupSucceeded ) return false;

	int width, height;
	bool success = false;

	// On my machine it takes 2 tries to get the RGB 
	// size but this might not be necessary for others. 

	attempt = 0;

	while ( attempt < _retryCount )
	{
		success = PXCUPipeline_QueryRGBSize ( &width, &height );
		if ( success )
		{
			_colorMapSize = Vec2i ( width, height );
			_colorMap = new uint8_t [ width * height * 4 ];
			break;
		}

		attempt++;
	}

	if ( !success ) return false;

	if ( PXCUPipeline_QueryDepthMapSize ( &width, &height ) )
	{
		_depthMapSize = Vec2i ( width, height );
		_depthMap = new short [ width * height ];
	}else
	{
		return false;
	}

	if ( PXCUPipeline_QueryIRMapSize ( &width, &height ) )
	{
		_irMapSize = Vec2i ( width, height );
		_irMap = new short [ width * height ];
	}else
	{
		return false;
	}

	if ( PXCUPipeline_QueryLabelMapSize ( &width, &height ) )
	{
		_labelMapSize = Vec2i ( width, height );
		_labelMap = new uint8_t [ width * height ];
	}else
	{
		return false;
	}

	if ( PXCUPipeline_QueryUVMapSize ( &width, &height ) )
	{
		_uvMapSize = Vec2i ( width, height );
		_uvMap = new float [ width * height * 2 ];
	}else
	{
		return false;
	}

	_setupSucceeded = true;
	_isInited = true;

	return true;
}

bool PXCUService::update()
{
	bool acquired = PXCUPipeline_AcquireFrame ( false );
	if ( !acquired ) return false;

	if ( _colorMap ) PXCUPipeline_QueryRGB ( _colorMap );
	if ( _depthMap ) PXCUPipeline_QueryDepthMap ( _depthMap );
	if ( _labelMap ) PXCUPipeline_QueryLabelMap ( _labelMap, 0 );
	if ( _irMap    ) PXCUPipeline_QueryIRMap ( _irMap );
	if ( _uvMap    ) PXCUPipeline_QueryUVMap ( _uvMap );

	processGestures();

	PXCUPipeline_ReleaseFrame();
	
	_waitForFrames = true;

	if ( _colorMap )
	{
		_colorMapTexture = gl::Texture ( _colorMap, GL_RGBA, _colorMapSize.x, _colorMapSize.y );
	}

	if ( _labelMap )
	{
		_labelMapTexture = gl::Texture ( _labelMap, GL_LUMINANCE, _labelMapSize.x, _labelMapSize.y );

		if ( _depthMap )
		{
			convertToImage ( _labelMap, (unsigned short*)_depthMap, _depthMapSize.x, _depthMapSize.y, true );
			_depthMapTexture = gl::Texture ( _labelMap, GL_LUMINANCE, _depthMapSize.x, _depthMapSize.y );
		}

		if ( _irMap )
		{
			convertToImage ( _labelMap, (unsigned short*)_irMap, _irMapSize.x, _irMapSize.y, false );
			_irMapTexture = gl::Texture ( _labelMap, GL_RED, _irMapSize.x, _irMapSize.y );	
		}
	}

	return true;
}

Vec2f PXCUService::mapXY ( Vec2f position )
{
	if ( !_depthMap || !_uvMap ) return position;

	int index = (int)position.y * _depthMapSize.x + (int)position.x;

	float ix = _uvMap[index * 2 + 0] * _uvMapSize.x;
	float iy = _uvMap[index * 2 + 1] * _uvMapSize.y;

	return Vec2f ( ix, iy );
}

void PXCUService::processGestures ( )
{
	PXCGesture::Gesture node;

	bool hasGestures = PXCUPipeline_QueryGesture ( PXCGesture::GeoNode::LABEL_ANY, &node );

	for ( vector<PXCGesture::GeoNode::Label>::iterator it = _trackedGeoNodes.begin(); it != _trackedGeoNodes.end(); ++it )
	{
		PXCGesture::GeoNode::Label label = *it;
		PXCGesture::GeoNode node;

		GeoNodeStateRef state = _geoNodeState[label];

		if ( PXCUPipeline_QueryGeoNode ( label, &node ) )
		{
			state->active = true;
			state->node = node;
			state->screen = mapXY ( Vec2f ( node.positionImage.x, node.positionImage.y ) );
		}else
		{
			state->active = false;
			state->screen = Vec2f ( -1, -1 );
		}

	}

	if ( hasGestures && _gestureHandler )
	{
		_gestureHandler ( node );
	}

}

void PXCUService::setTrackedGeoNodes ( vector< PXCGesture::GeoNode::Label> nodes )
{
	_trackedGeoNodes = nodes;
	_geoNodeState.clear();

	for ( vector<PXCGesture::GeoNode::Label>::iterator it = _trackedGeoNodes.begin(); it != _trackedGeoNodes.end(); ++it )
	{
		PXCGesture::GeoNode::Label label = *it;
		_geoNodeState[label] = shared_ptr<GeoNodeState> ( new GeoNodeState() );
	}
}

void PXCUService::setGestureHandler ( GestureHandler _handler )
{
	_gestureHandler = _handler;
}

void PXCUService::removeGestureHandler ( )
{
	_gestureHandler = 0;
}

void PXCUService::convertToImage ( uint8_t* dst, unsigned short* src, int w, int h, bool invert )
{
	float minC = 0xFFFF;
	float maxC = -0xFFFF;

	int dim = w * h;

	for ( int k = 0; k < dim; k++ )
	{
		float vC = (float)src[k] / 0xFFFF;
		if ( minC > vC ) minC = vC;
		if ( maxC < vC ) maxC = vC;
	}

	for ( int i = 0; i < dim; i++ )
	{
		float value = (float)src[i] / 0xFFFF;
		value = 255.0f * sqrt ( ( value - minC ) / ( value / maxC ) );

		dst[i] = invert ? ( 255 - (uint8_t)value ) : (uint8_t)value;
	}

}

bool PXCUService::checkImage ( uint8_t* _image, int w, int h, uint8_t value )
{
	uint8_t *image = _image;
	int dim = w * h;

	for ( int i = 0; i < dim; i++ )
	{
		if ( image[0] < value ) return true;
		image += 4;
	}

	return false;
}

void PXCUService::close ( )
{
	if ( _isInited )
	{
		PXCUPipeline_Close();
		_isInited = false;
	}
}

PXCUService::~PXCUService()
{
	if ( _waitForFrames ) PXCUPipeline_AcquireFrame ( true );

	close();

	if ( _colorMap ) delete[] _colorMap; _colorMap = 0;
	if ( _labelMap ) delete[] _labelMap; _labelMap = 0;
	if ( _depthMap ) delete[] _depthMap; _depthMap = 0;
	if ( _irMap    ) delete[] _irMap   ; _irMap    = 0;
	if ( _uvMap    ) delete[] _uvMap   ; _uvMap    = 0;

}