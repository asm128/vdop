#include "llc_framework.h"
#include "llc_gui.h"
#include "llc_geometry.h"

#ifndef APPLICATION_H_098273498237423
#define APPLICATION_H_098273498237423

//struct SCameraVectors {
//						::llc::SCoord3<float>								Right, Up, Front;					
//};

struct SRasterCache {
						::llc::array_pod<::llc::SCoord2<int32_t>>			TrianglePixelCoords								= {};
						::llc::array_pod<::llc::STriangle2D<int32_t>>		Triangle2dListDrawn								= {};
						::llc::array_pod<int32_t>							TriangleIndicesDrawn							= {};
						::llc::array_pod<uint32_t>							PixelCounts										= {};
						::llc::array_pod<::llc::STriangle2D<int32_t>>		Triangle2dList									= {};
						::llc::array_pod<::llc::STriangle3D<float>>			Triangle3dList									= {};
						::llc::array_pod<::llc::SColorBGRA>					Triangle3dColorList								= {};
};

struct SCamera {
						::llc::SCoord3<float>								Position, Target;
};

struct SRasterTransform {
						::llc::SMatrix4<float>								View;
						::llc::SMatrix4<float>								Projection;
						::llc::SMatrix4<float>								Viewport;
						::llc::SMatrix4<float>								ViewportInverseTranslated;
};

struct SLightDirectional {
						::llc::SCoord3<float>								Direction;
						::llc::SColorBGRA									Color;
};

struct SObjectAxes {
						::llc::SCoord3<float>								Front;
						::llc::SCoord3<float>								Right;
						::llc::SCoord3<float>								Up;
};

struct SCameraDepth {
						float												Far;
						float												Near;
};

struct SApplication {
						::llc::SFramework									Framework										;

						::llc::STexture<::llc::SColorBGRA>					TextureFont										= {};
						::llc::STextureMonochrome<uint32_t>					TextureFontMonochrome							= {};
						::llc::SGUI											GUI												= {};

						::llc::SModelBase									ModelBox										= {};
						::llc::SModelPivot									BoxPivot										= {};
						::llc::SModelTransform								BoxTransform									= {};
						::SRasterTransform									RasterTransform									= {};
						::SRasterCache										RasterCache										= {};
						double												CameraAngle										= .25;//.25;
						::SCamera											Camera											= {{10, 5, 0}, {}};
						::SObjectAxes										CameraAxes										= 
							{ {1, 0, 0}
							, {0, 1, 0}
							, {0, 0, 1}
							};
						::SCameraDepth										CameraDepth										= {};
						::SLightDirectional									LightDirectional								= {};

																			SApplication									(::llc::SRuntimeValues& runtimeValues)			noexcept	: Framework(runtimeValues)	{}
};

#endif // APPLICATION_H_098273498237423
