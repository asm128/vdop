// Tip: Hold Left ALT + SHIFT while tapping or holding the arrow keys in order to select multiple columns and write on them at once. 
//		Also useful for copy & paste operations in which you need to copy a bunch of variable or function names and you can't afford the time of copying them one by one.
#include "application.h"

#include "llc_bitmap_target.h"
#include "llc_bitmap_file.h"
#include "llc_grid_copy.h"
#include "llc_grid_scale.h"
#include "llc_bit_array_view.h"
#include "llc_geometry.h"

#include "llc_app_impl.h"

static constexpr	const uint32_t										ASCII_SCREEN_WIDTH							= 132	;
static constexpr	const uint32_t										ASCII_SCREEN_HEIGHT							= 50	;

LLC_DEFINE_APPLICATION_ENTRY_POINT(::SApplication);	

					::SApplication										* g_ApplicationInstance						= 0;

static				::llc::error_t										updateSizeDependentResources				(::SApplication& applicationInstance)											{ 
	const ::llc::SCoord2<uint32_t>												newSize										= applicationInstance.Framework.MainDisplay.Size; 
	::llc::STexture<::llc::SColorBGRA>											& viewportOffscreen							= applicationInstance.Framework.Offscreen;
	if( viewportOffscreen.View.width () != newSize.x
	 || viewportOffscreen.View.height() != newSize.y
	 ) {
	
	}
	::llc::updateSizeDependentTarget(viewportOffscreen, newSize);
	return 0;
}

// --- Cleanup application resources.
					::llc::error_t										cleanup										(::SApplication& applicationInstance)											{
	::llc::SDisplayPlatformDetail												& displayDetail								= applicationInstance.Framework.MainDisplay.PlatformDetail;
	if(displayDetail.WindowHandle) {
		error_if(0 == ::DestroyWindow(displayDetail.WindowHandle), "Not sure why would this fail.");
		error_if(errored(::llc::displayUpdate(applicationInstance.Framework.MainDisplay)), "Not sure why this would fail");
	}
	::UnregisterClass(displayDetail.WindowClassName, displayDetail.WindowClass.hInstance);
	g_ApplicationInstance													= 0;
	return 0;
}

					::llc::error_t										mainWindowCreate							(::llc::SDisplay& mainWindow, HINSTANCE hInstance);
					::llc::error_t										setup										(::SApplication& applicationInstance)											{ 
	g_ApplicationInstance													= &applicationInstance;
	::llc::SDisplay																& mainWindow								= applicationInstance.Framework.MainDisplay;
	error_if(errored(::mainWindowCreate(mainWindow, applicationInstance.Framework.RuntimeValues.PlatformDetail.EntryPointArgs.hInstance)), "Failed to create main window why?????!?!?!?!?");
	static constexpr	const char												bmpFileName2	[]							= "Codepage-437-24.bmp";
	error_if(errored(::llc::bmpFileLoad((::llc::view_const_string)bmpFileName2, applicationInstance.TextureFont				)), "Failed to load bitmap from file: %s.", bmpFileName2);
	const ::llc::SCoord2<uint32_t>												& textureFontMetrics						= applicationInstance.TextureFont.View.metrics();
	applicationInstance.TextureFontMonochrome.resize(textureFontMetrics);
	for(uint32_t y = 0, yMax = textureFontMetrics.y; y < yMax; ++y)
	for(uint32_t x = 0, xMax = textureFontMetrics.x; x < xMax; ++x)
		applicationInstance.TextureFontMonochrome.View[y * textureFontMetrics.x + x]	
		=	0 != applicationInstance.TextureFont.View[y][x].r
		||	0 != applicationInstance.TextureFont.View[y][x].g
		||	0 != applicationInstance.TextureFont.View[y][x].b
		;

	// Load and pretransform our cube geometry.
	static constexpr	const ::llc::SCoord3<float>								cubeCenter									= {0.5f, 0.5f, 0.5f};
	::llc::generateCubeGeometry(applicationInstance.ModelBox.Geometry.Positions, applicationInstance.ModelBox.Geometry.Normals, applicationInstance.ModelBox.Geometry.UVs);
	for(uint32_t iTriangle = 0; iTriangle < 12; ++iTriangle) {
		::llc::STriangle3D<float>													& transformedTriangle						= applicationInstance.ModelBox.Geometry.Positions[iTriangle];
		transformedTriangle.A													-= cubeCenter;
		transformedTriangle.B													-= cubeCenter;
		transformedTriangle.C													-= cubeCenter;
	}
	::llc::SCoord3<float>														& lightDir									= applicationInstance.LightDirectional.Direction; //
	lightDir																= {10, 5, 0};

	applicationInstance.BoxPivot.Scale										= {.5, 1, 2.5};
	applicationInstance.BoxPivot.Position									= {0, 0, 0};
	applicationInstance.BoxPivot.Orientation								= applicationInstance.BoxPivot.Orientation;

	// Set up camera
	applicationInstance.CameraAngle											= .25;//.25;
	applicationInstance.Camera												= {{10, 5, 0}, {}};
	applicationInstance.CameraAxes											= {};
	applicationInstance.CameraDepth											= {100.f, 0.01f};
	ree_if(errored(::updateSizeDependentResources(applicationInstance)), "Cannot update offscreen and textures and this could cause an invalid memory access later on.");
	return 0;
}

					::llc::error_t										update										(::SApplication& applicationInstance, bool systemRequestedExit)					{ 
	retval_info_if(1, systemRequestedExit, "Exiting because the runtime asked for close. We could also ignore this value and just continue execution if we don't want to exit.");
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::error_t																frameworkResult								= ::llc::updateFramework(framework);
	::llc::SFrameInfo															& frameInfo									= framework.FrameInfo;
	ree_if(errored(frameworkResult), "Unknown error.");
	rvi_if(1, frameworkResult == 1, "Framework requested close. Terminating execution.");
	ree_if(errored(::updateSizeDependentResources(applicationInstance)), "Cannot update offscreen and this could cause an invalid memory access later on.");
	if(applicationInstance.Framework.Input.KeyboardCurrent.KeyState[VK_ADD		])	applicationInstance.CameraAngle += frameInfo.Seconds.LastFrame * .05f;
	if(applicationInstance.Framework.Input.KeyboardCurrent.KeyState[VK_SUBTRACT	])	applicationInstance.CameraAngle -= frameInfo.Seconds.LastFrame * .05f;

	// ------------------------------ Update Light
	::llc::SCoord3<float>														& lightDir									= applicationInstance.LightDirectional.Direction; 
	//lightDir.RotateY(frameInfo.Seconds.LastFrame);
	lightDir.Normalize();

	// ------------------------------ Update Camera
	::SCamera																	& camera									= applicationInstance.Camera;
	float																		cameraRotation								= 0;
	cameraRotation															+= (float)framework.Input.MouseCurrent.Deltas.x / 20.0f;
	if(framework.Input.MouseCurrent.Deltas.z) {
		::llc::SCoord3<float>														cameraZoomDirection							= camera.Position;
		cameraZoomDirection.Normalize();
		camera.Position															+= cameraZoomDirection * (framework.Input.MouseCurrent.Deltas.z / WHEEL_DELTA) * frameInfo.Seconds.LastFrame * 3;
	}
	camera.Position	.RotateY(cameraRotation);
	//camera.Position	.RotateY(frameInfo.Microseconds.Total / 1000000.0f);
	::SObjectAxes																& cameraAxes								= applicationInstance.CameraAxes;
	//::llc::SMatrix4<float>														mrotx, mroty;
	//mrotx.RotationY				( framework.Input.MouseCurrent.Deltas.y );
	//mroty.RotationArbitraryAxis	( cameraAxes.Right, (float)framework.Input.MouseCurrent.Deltas.x );
	//cameraAxes.Front.Normalize();
	//cameraAxes.Front														= mrotx.Transform(cameraAxes.Front);
	//cameraAxes.Front														= mroty.Transform(cameraAxes.Front);
	//cameraAxes.Right														= mroty.Transform(cameraAxes.Right);
	//cameraAxes.Up															= mrotx.Transform(cameraAxes.Up);
	//cameraAxes.Up															= mroty.Transform(cameraAxes.Up);
	//cameraAxes.Front.Normalize();
	cameraAxes.Front														= (camera.Target - camera.Position).Normalize();
	cameraAxes.Up															= {0, 1, 0};
	cameraAxes.Right														= cameraAxes.Up.Cross(cameraAxes.Front).Normalize();
	cameraAxes.Up.Cross(cameraAxes.Front, cameraAxes.Right).Normalize();

	//----------------------------
	::llc::SMatrix4<float>														& projection								= applicationInstance.RasterTransform.Projection;
	projection.Identity();
	::llc::SMatrix4<float>														& viewMatrix								= applicationInstance.RasterTransform.View;
	viewMatrix.View3D(camera.Position, cameraAxes.Right, cameraAxes.Up, cameraAxes.Front);

	//viewMatrix.LookAt(camera.Position, camera.Target, cameraUp);
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	::SCameraDepth																& cameraDepth								= applicationInstance.CameraDepth;
	projection.FieldOfView(applicationInstance.CameraAngle * ::llc::math_pi, offscreenMetrics.x / (double)offscreenMetrics.y, cameraDepth.Near, cameraDepth.Far);
	projection																= viewMatrix * projection;

	::llc::SMatrix4<float>														& viewport									= applicationInstance.RasterTransform.Viewport;
	viewport.Viewport(offscreenMetrics, cameraDepth.Far, cameraDepth.Near);
	::llc::SMatrix4<float>														& viewportInverse							= applicationInstance.RasterTransform.ViewportInverseTranslated;
	viewportInverse.SetInverse(viewport);
	const ::llc::SCoord2<int32_t>												screenCenter								= {(int32_t)offscreenMetrics.x / 2, (int32_t)offscreenMetrics.y / 2};
	viewportInverse._41														+= screenCenter.x;
	viewportInverse._42														+= screenCenter.y;
	projection																*= viewportInverse;
	applicationInstance.BoxPivot.Orientation.y								+= (float)(sinf((float)frameInfo.Seconds.Total / 2) * ::llc::math_pi);
	applicationInstance.BoxPivot.Orientation.w								= 1;
	applicationInstance.BoxPivot.Orientation.Normalize();
	return 0;
}

static				::llc::error_t										textCalcSizeLine							(const ::llc::SCoord2<int32_t>& sizeCharCell, const ::llc::view_const_string& text0)	{ return (::llc::error_t)(sizeCharCell.x * (text0.size() - 1)); }
static				::llc::error_t										textDrawFixedSize							(::llc::grid_view<::llc::SColorBGRA>& bmpTarget, const ::llc::grid_view<::llc::SColorBGRA>& viewTextureFont, uint32_t characterCellsX, int32_t dstOffsetY, const ::llc::SCoord2<int32_t>& sizeCharCell, const ::llc::view_const_string& text0, const ::llc::SCoord2<int32_t> dstTextOffset)	{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	for(int32_t iChar = 0, charCount = (int32_t)text0.size(); iChar < charCount; ++iChar) {
		int32_t																	coordTableX										= text0[iChar] % characterCellsX;
		int32_t																	coordTableY										= text0[iChar] / characterCellsX;
		const ::llc::SCoord2<int32_t>											coordCharTable									= {coordTableX * sizeCharCell.x, coordTableY * sizeCharCell.y};
		const ::llc::SCoord2<int32_t>											dstOffset1										= {sizeCharCell.x * iChar, dstOffsetY};
		const ::llc::SRectangle2D<int32_t>										srcRect0										= ::llc::SRectangle2D<int32_t>{{coordCharTable.x, (int32_t)viewTextureFont.height() - sizeCharCell.y - coordCharTable.y}, sizeCharCell};
		error_if(errored(::llc::grid_copy_alpha(bmpTarget, viewTextureFont, dstTextOffset + dstOffset1, srcRect0, {0xFF, 0x00, 0xFF, 0xFF})), "I believe this never fails.");
		//error_if(errored(::llc::grid_copy(bmpTarget, viewTextureFont, dstTextOffset + dstOffset1, srcRect0)), "I believe this never fails.");
	}
	return 0;
}

static				::llc::error_t										textDrawAlignedFixedSize					(::llc::grid_view<::llc::SColorBGRA>& targetView, const ::llc::grid_view<::llc::SColorBGRA>& fontAtlas, uint32_t lineOffset, const ::llc::SCoord2<uint32_t>& targetSize, const ::llc::SCoord2<int32_t>& sizeCharCell, const ::llc::view_const_string& text0 )	{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	const ::llc::SCoord2<int32_t>												dstTextOffset								= {(int32_t)targetSize.x / 2 - (int32_t)textCalcSizeLine(sizeCharCell, text0) / 2, };
	uint32_t																	dstOffsetY									= (int32_t)(targetSize.y - lineOffset * sizeCharCell.y - sizeCharCell.y);
	return ::textDrawFixedSize(targetView, fontAtlas, 32, dstOffsetY, sizeCharCell, text0, dstTextOffset);
}

///---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static				::llc::error_t										textDrawFixedSize							(::llc::grid_view<::llc::SColorBGRA>& bmpTarget, const ::llc::bit_array_view<uint32_t>& viewTextureFont, const ::llc::SCoord2<uint32_t> & viewMetrics, uint32_t characterCellsX, int32_t dstOffsetY, const ::llc::SCoord2<int32_t>& sizeCharCell, const ::llc::view_const_string& text0, const ::llc::SCoord2<int32_t> dstTextOffset, const ::llc::SColorBGRA& color)	{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::array_pod<::llc::SCoord2<uint32_t>>									dstCoords;
	for(int32_t iChar = 0, charCount = (int32_t)text0.size(); iChar < charCount; ++iChar) {
		const int32_t																coordTableX										= text0[iChar] % characterCellsX;
		const int32_t																coordTableY										= text0[iChar] / characterCellsX;
		const ::llc::SCoord2<int32_t>												coordCharTable									= {coordTableX * sizeCharCell.x, coordTableY * sizeCharCell.y};
		const ::llc::SCoord2<int32_t>												dstOffset1										= {sizeCharCell.x * iChar, dstOffsetY};
		const ::llc::SRectangle2D<int32_t>											srcRect0										= ::llc::SRectangle2D<int32_t>{{coordCharTable.x, (int32_t)viewMetrics.y - sizeCharCell.y - coordCharTable.y}, sizeCharCell};
		//error_if(errored(::llc::grid_copy_alpha_bit(bmpTarget, viewTextureFont, dstTextOffset + dstOffset1, viewMetrics, color, srcRect0)), "I believe this never fails.");
		dstCoords.clear();
		error_if(errored(::llc::grid_raster_alpha_bit(bmpTarget, viewTextureFont, dstTextOffset + dstOffset1, viewMetrics, srcRect0, dstCoords)), "I believe this never fails.");
		for(uint32_t iCoord = 0; iCoord < dstCoords.size(); ++iCoord)
			::llc::drawPixelLight(bmpTarget, dstCoords[iCoord], color, 0.05f, 0.75);
	}
	return 0;
}

static				::llc::error_t										textDrawAlignedFixedSize					(::llc::grid_view<::llc::SColorBGRA>& targetView, const ::llc::bit_array_view<uint32_t>& fontAtlas, const ::llc::SCoord2<uint32_t> & viewMetrics, uint32_t lineOffset, const ::llc::SCoord2<uint32_t>& targetSize, const ::llc::SCoord2<int32_t>& sizeCharCell, const ::llc::view_const_string& text0, const ::llc::SColorBGRA& color)	{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	const ::llc::SCoord2<int32_t>												dstTextOffset								= {(int32_t)targetSize.x / 2 - (int32_t)textCalcSizeLine(sizeCharCell, text0) / 2, };
	uint32_t																	dstOffsetY									= (int32_t)(targetSize.y - lineOffset * sizeCharCell.y - sizeCharCell.y);
	return ::textDrawFixedSize(targetView, fontAtlas, viewMetrics, 32, dstOffsetY, sizeCharCell, text0, dstTextOffset, color);
}

					::llc::error_t										draw										(::SApplication& applicationInstance)											{	// --- This function will draw some coloured symbols in each cell of the ASCII screen.
	::llc::SFramework															& framework									= applicationInstance.Framework;
	::llc::SFramework::TOffscreen												& offscreen									= framework.Offscreen;
	const ::llc::SCoord2<uint32_t>												& offscreenMetrics							= offscreen.View.metrics();
	::memset(offscreen.Texels.begin(), 0, sizeof(::llc::SFramework::TOffscreen::TTexel) * offscreen.Texels.size());	// Clear target.
	//------------------------------------------------
	::SRasterCache																& rasterCache								= applicationInstance.RasterCache;

	const ::llc::SCoord3<float>													& lightDir									= applicationInstance.LightDirectional.Direction; //{10, 5, 0};
	uint32_t																	totalPixels									= 0;
	::llc::SMatrix4<float>														& projection								= applicationInstance.RasterTransform.Projection;
	::SObjectAxes																& cameraAxes								= applicationInstance.CameraAxes;
	::SCameraDepth																& cameraDepth								= applicationInstance.CameraDepth;
	for(uint32_t iBox = 0; iBox < 15; ++iBox) {
		rasterCache.Triangle3dList		.resize(12);
		rasterCache.Triangle3dColorList	.resize(12);
		rasterCache.Triangle2dList		.resize(12);
		rasterCache.TrianglePixelCoords	.clear();
		rasterCache.Triangle2dListDrawn	.clear();
		rasterCache.TriangleIndicesDrawn.clear();
		rasterCache.PixelCounts			.clear();
		::llc::SMatrix4<float>														matrixScale, matrixRotation, matrixTranslation;
		matrixScale			.Scale			(applicationInstance.BoxPivot.Scale, true);
		matrixRotation		.SetOrientation	((applicationInstance.BoxPivot.Orientation + ::llc::SQuaternion<float>{0, (float)(iBox / ::llc::math_2pi), 0, 0}).Normalize());
		matrixTranslation	.SetTranslation	(applicationInstance.BoxPivot.Position + ::llc::SCoord3<float>{(float)iBox, 0, 0}, true);
		::llc::SMatrix4<float>														matrixWorld									= matrixScale * matrixRotation * matrixTranslation;
		::llc::SMatrix4<float>														matrixWP									= matrixWorld * projection;
		for(uint32_t iTriangle = 0, countTris = rasterCache.Triangle3dList.size(); iTriangle < countTris; ++iTriangle) {
			::llc::STriangle3D<float>													& transformedTriangle						= rasterCache.Triangle3dList[iTriangle];
			transformedTriangle														= applicationInstance.ModelBox.Geometry.Positions[iTriangle];
			::llc::transform(transformedTriangle, matrixWP);
		}
		for(uint32_t iTriangle = 0, countTris = rasterCache.Triangle3dList.size(); iTriangle < countTris; ++iTriangle) {
			::llc::SCoord3<float>														normaltransformed							= matrixRotation.Transform(applicationInstance.ModelBox.Geometry.Normals[iTriangle]);
			normaltransformed.Normalize();
			double																		lightFactor									= normaltransformed.Dot(lightDir);
			rasterCache.Triangle3dColorList[iTriangle]								= ::llc::GREEN * lightFactor;
		}
		for(uint32_t iTriangle = 0, countTris = rasterCache.Triangle3dList.size(); iTriangle < countTris; ++iTriangle) {
			const ::llc::STriangle3D<float>												& transformedTriangle3D						= rasterCache.Triangle3dList[iTriangle];
			::llc::STriangle2D<int32_t>													& transformedTriangle2D						= rasterCache.Triangle2dList[iTriangle];
			transformedTriangle2D.A													= {(int32_t)transformedTriangle3D.A.x, (int32_t)transformedTriangle3D.A.y};
			transformedTriangle2D.B													= {(int32_t)transformedTriangle3D.B.x, (int32_t)transformedTriangle3D.B.y};
			transformedTriangle2D.C													= {(int32_t)transformedTriangle3D.C.x, (int32_t)transformedTriangle3D.C.y};
		}
		::llc::array_pod<::llc::SCoord2<int32_t>>									wireframePixelCoords						= {};
		::llc::SCoord2<int32_t>														offscreenMetricsI							= offscreenMetrics.Cast<int32_t>();
		for(uint32_t iTriangle = 0, countTris = rasterCache.Triangle3dList.size(); iTriangle < countTris; ++iTriangle) {
			const ::llc::SCoord3<float>													normaltransformed							= matrixWorld.Transform(applicationInstance.ModelBox.Geometry.Normals[iTriangle]).Normalize();
			double																		cameraFactor								= normaltransformed.Dot(cameraAxes.Front);
			if(cameraFactor > 0)
				continue;
			const ::llc::STriangle2D<int32_t>											& currentTriangle							= rasterCache.Triangle2dList[iTriangle];
			if(currentTriangle.A.x < 0 && currentTriangle.B.x < 0 && currentTriangle.C.x < 0) 
				continue;
			if(currentTriangle.A.y < 0 && currentTriangle.B.y < 0 && currentTriangle.C.y < 0) 
				continue;
			if( currentTriangle.A.x >= offscreenMetricsI.x
			 && currentTriangle.B.x >= offscreenMetricsI.x
			 && currentTriangle.C.x >= offscreenMetricsI.x
			 )
				continue;
			if( currentTriangle.A.y >= offscreenMetricsI.y
			 && currentTriangle.B.y >= offscreenMetricsI.y
			 && currentTriangle.C.y >= offscreenMetricsI.y
			 )
				continue;
			const ::llc::STriangle3D<float>												& transformedTriangle3D						= rasterCache.Triangle3dList[iTriangle];
			if(transformedTriangle3D.A.z >= cameraDepth.Far		|| transformedTriangle3D.B.z >= cameraDepth.Far		|| transformedTriangle3D.C.z >= cameraDepth.Far ) continue;
			if(transformedTriangle3D.A.z <= cameraDepth.Near	|| transformedTriangle3D.B.z <= cameraDepth.Near	|| transformedTriangle3D.C.z <= cameraDepth.Near) continue;
			llc_necall(rasterCache.Triangle2dListDrawn	.push_back(rasterCache.Triangle2dList[iTriangle])	, "out of memory?");
			llc_necall(rasterCache.TriangleIndicesDrawn	.push_back(iTriangle)								, "out of memory?");
		}
		for(uint32_t iTriangle = 0, triCount = rasterCache.TriangleIndicesDrawn.size(); iTriangle < triCount; ++iTriangle) {
			::llc::error_t																pixelCount;
			ce_if(errored(pixelCount = ::llc::drawTriangle(offscreenMetrics, rasterCache.Triangle2dListDrawn[iTriangle], rasterCache.TrianglePixelCoords)), "Not sure if these functions could ever fail");
			llc_necall(rasterCache.PixelCounts.push_back(pixelCount), "out of memory?");
		}
		uint32_t																	processedPixels								= 0;
		for(uint32_t iTriangle = 0, triCount = rasterCache.TriangleIndicesDrawn.size(); iTriangle < triCount; ++iTriangle) {
			for(uint32_t iPixel = 0; iPixel < rasterCache.PixelCounts[iTriangle]; ++iPixel) {
				const ::llc::SCoord2<int32_t>												& pixCoord									= rasterCache.TrianglePixelCoords[processedPixels + iPixel];
				offscreen.View[pixCoord.y][pixCoord.x]									= rasterCache.Triangle3dColorList[rasterCache.TriangleIndicesDrawn[iTriangle]];
				//::llc::drawPixelBrightness(offscreen.View, pixCoord, rasterCache.Triangle3dColorList[rasterCache.TriangleIndicesDrawn[iTriangle]], 0.1f, 1.0f);
			}
			processedPixels															+= rasterCache.PixelCounts[iTriangle];
		}
		totalPixels																+= processedPixels;
	}
	//------------------------------------------------
	static constexpr const ::llc::SCoord2<int32_t>								sizeCharCell								= {9, 16};
	uint32_t																	lineOffset									= 0;
	static constexpr const char													textLine0	[]								= "W: Up, S: Down, A: Left, D: Right";
	static constexpr const char													textLine1	[]								= "T: Shoot. Y: Thrust. U: Handbrake.";
	static constexpr const char													textLine2	[]								= "Press ESC to exit.";
	::llc::grid_view<::llc::SColorBGRA>											& offscreenView								= applicationInstance.Framework.Offscreen.View;
	const ::llc::grid_view<::llc::SColorBGRA>									& fontAtlasView								= applicationInstance.TextureFont.View;
	::textDrawAlignedFixedSize(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), lineOffset = offscreenMetrics.y / sizeCharCell.y - 1, offscreenMetrics, sizeCharCell, textLine2, ::llc::SColorBGRA{0, framework.FrameInfo.FrameNumber % 0xFFU, 0, 0xFFU});
	::llc::STimer																& timer										= applicationInstance.Framework.Timer;
	::llc::SDisplay																& mainWindow								= applicationInstance.Framework.MainDisplay;
	char																		buffer		[256]							= {};
	uint32_t																	textLen										= (uint32_t)sprintf_s(buffer, "[%u x %u]. FPS: %g. Last frame seconds: %g.", mainWindow.Size.x, mainWindow.Size.y, 1 / timer.LastTimeSeconds, timer.LastTimeSeconds);
	::textDrawAlignedFixedSize(offscreenView, applicationInstance.TextureFontMonochrome.View, fontAtlasView.metrics(), --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen}, ::llc::SColorBGRA{0, framework.FrameInfo.FrameNumber % 0xFFU, 0, 0xFFU});;
	textLen										= (uint32_t)sprintf_s(buffer, "Pixels drawn: %u. Last frame seconds: %g.", totalPixels, timer.LastTimeSeconds);
	::textDrawAlignedFixedSize(offscreenView, fontAtlasView, --lineOffset, offscreenMetrics, sizeCharCell, {buffer, textLen});
	return 0;																																																
}
	