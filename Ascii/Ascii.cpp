/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007-2023 Adobe Inc.                                  */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Inc. and its suppliers, if                    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Inc. and its                    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Inc.            */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

/*	Ascii.cpp

	This is a compiling husk of a project. Fill it in with interesting
	pixel processing code.

	Revision History

	Version		Change													Engineer	Date
	=======		======													========	======
	1.0			(seemed like a good idea at the time)					bbb			6/1/2002

	1.0			Okay, I'm leaving the version at 1.0,					bbb			2/15/2006
				for obvious reasons; you're going to
				copy these files directly! This is the
				first XCode version, though.

	1.0			Let's simplify this barebones sample					zal			11/11/2010

	1.0			Added new entry point									zal			9/18/2017
	1.1			Added 'Support URL' to PiPL and entry point				cjr			3/31/2023

*/

#include "Ascii.h"

static PF_Err
About(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output)
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg,
										  "%s v%d.%d\r%s",
										  STR(StrID_Name),
										  MAJOR_VERSION,
										  MINOR_VERSION,
										  STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err
GlobalSetup(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output)
{
	out_data->my_version = PF_VERSION(MAJOR_VERSION,
									  MINOR_VERSION,
									  BUG_VERSION,
									  STAGE_VERSION,
									  BUILD_VERSION);

	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE; // just 16bpc, not 32bpc
	out_data->out_flags2 = PF_OutFlag2_FLOAT_COLOR_AWARE | PF_OutFlag2_SUPPORTS_THREADED_RENDERING | PF_OutFlag2_SUPPORTS_SMART_RENDER;

	return PF_Err_NONE;
}

static PF_Err
ParamsSetup(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output)
{
	PF_Err err = PF_Err_NONE;

	PF_ParamDef def;

	AEFX_CLR_STRUCT(def);

	PF_ADD_CHECKBOX(STR(StrID_COLOR_Param_Name), STR(StrID_NONE), false, 0, COLOR_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_SLIDER(STR(StrID_EDGE_Param_Name), -4, 64, 0, 64, 8, EDGE_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_CHECKBOX(STR(StrID_TRANSPARENT_Param_Name), STR(StrID_NONE), false, 0, TRANSPARENT_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_SLIDER(STR(StrID_KERNEL_SIZE_Param_Name), 1, 10, 1, 10, 2, KERNEL_SIZE_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_SIGMA_Param_Name), 0.1f, 5.0f, 0.1f, 5.0f, 2.0f, PF_Precision_TEN_THOUSANDTHS, 0, 0, SIGMA_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_SIGMA_SCALE_Param_Name), 0.1f, 5.0f, 0.1f, 5.0f, 1.6f, PF_Precision_TEN_THOUSANDTHS, 0, 0, SIGMA_SCALE_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_TAU_Param_Name), 0.01f, 5.0f, 0.1f, 5.0f, 1.0f, PF_Precision_TEN_THOUSANDTHS, 0, 0, TAU_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_TRESHOLD_Param_Name), 0.001f, 0.01f, 0.001f, 0.01f, 0.005f, PF_Precision_TEN_THOUSANDTHS, 0, 0, TRESHOLD_DISK_ID);

	out_data->num_params = ASCII_NUM_PARAMS;

	return err;
}

static float gaussian(float sigma, float pos) noexcept
{
	return (1.0f / sqrt(2.0f * M_PI * sigma * sigma)) * exp(-(pos * pos) / (2.0f * sigma * sigma));
}

static PF_Pixel32 *sampleIntegral128(PF_EffectWorld &def, int x, int y) noexcept
{
	if (x < 0)
	{
		x = 0;
	}
	else if (x > def.width)
	{
		x = def.width;
	}
	if (y < 0)
	{
		y = 0;
	}
	else if (y > def.height)
	{
		y = def.height;
	}
	return (PF_Pixel32 *)((char *)def.data +
						  (y * def.rowbytes) +
						  (x * sizeof(PF_Pixel32)));
}

static float calcLuminance(
	const PF_Pixel32 *pixel) noexcept
{
	return sqrt(0.299f * pixel->red * pixel->red + 0.587f * pixel->green * pixel->green + 0.114f * pixel->blue * pixel->blue) * pixel->alpha;
}

static bool getAscii(
	float luminence,
	int x,
	int y
) noexcept {
	if (x < 0 || x > 7 || y < 0 || y > 7) {
		return false;
	}
	switch (min(max(int(round(luminence * 12)-0.5),0),11)) {
	case 0:
		return ascci0[y][x];
		break;
	case 1:
		return ascci1[y][x];
		break;
	case 2:
		return ascci2[y][x];
		break;
	case 3:
		return ascci3[y][x];
		break;
	case 4:
		return ascci4[y][x];
		break;
	case 5:
		return ascci5[y][x];
		break;
	case 6:
		return ascci6[y][x];
		break;
	case 7:
		return ascci7[y][x];
		break;
	case 8:
		return ascci8[y][x];
		break;
	case 9:
		return ascci9[y][x];
		break;
	case 10:
		return ascci10[y][x];
		break;
	case 11:
	default:
		return ascci11[y][x];
		break;
	}
}

static PF_Err
BlurPassOne(
	void* refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32* inP,
	PF_Pixel32* outP) noexcept {
	PF_Err err = PF_Err_NONE;

	BlurPass1* biP = reinterpret_cast<BlurPass1*>(refcon);

	float blur[2] = { 0.0f, 0.0f };
	float kernelSum[2] = { 0.0f, 0.0f };

	for (int x = -biP->kernel_size; x <= biP->kernel_size; ++x) {
		float luminance = calcLuminance(sampleIntegral128(*(biP->source), xL + x, yL));
		float gauss[2] = { gaussian(biP->sigma, x),gaussian(biP->sigma * biP->sigma_scale, x) };

		blur[0] += luminance * gauss[0];
		blur[1] += luminance * gauss[1];
		kernelSum[0] += gauss[0];
		kernelSum[1] += gauss[1];
	}

	blur[0] /= kernelSum[0];
	blur[1] /= kernelSum[1];

	outP->red = blur[0];
	outP->green = blur[1];
	outP->blue = 0.0f;
	outP->alpha = 1.0f;

	return err;
}

static PF_Err
BlurPassTwo(
	void* refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32* inP,
	PF_Pixel32* outP) noexcept {
	PF_Err err = PF_Err_NONE;

	BlurPass2* biP = reinterpret_cast<BlurPass2*>(refcon);

	float blur[2] = { 0.0f, 0.0f };
	float kernelSum[2] = { 0.0f, 0.0f };

	for (int y = -biP->kernel_size; y <= biP->kernel_size; ++y) {
		PF_Pixel32* pixel = sampleIntegral128(*(biP->source), xL, yL + y);
		float gauss[2] = { gaussian(biP->sigma,y),gaussian(biP->sigma * biP->sigma_scale, y) };

		blur[0] += pixel->red * gauss[0];
		blur[1] += pixel->green * gauss[1];
		kernelSum[0] += gauss[0];
		kernelSum[1] += gauss[1];
	}

	blur[0] /= kernelSum[0];
	blur[1] /= kernelSum[1];

	float D = (blur[0] - biP->tau * blur[1]);

	if (D >= biP->treshold) {
		D = 1.0f;
	}
	else {
		D = 0.0f;
	}

	outP->red = D;
	outP->green = D;
	outP->blue = D;
	outP->alpha = D;

	return err;
}

static PF_Err
SobelHorizontal(
	void* refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32* inP,
	PF_Pixel32* outP) noexcept {
	PF_Err err = PF_Err_NONE;

	const Sobel* siP = reinterpret_cast<Sobel*>(refcon);

	const float lum1 = sampleIntegral128(*(siP->source), xL - 1, yL)->red;
	const float lum2 = sampleIntegral128(*(siP->source), xL, yL)->red;
	const float lum3 = sampleIntegral128(*(siP->source), xL + 1, yL)->red;

	const float Gx = 3 * lum1 + 0 * lum2 + -3 * lum3;
	const float Gy = 3 + lum1 + 10 * lum2 + 3 * lum3;

	outP->red = Gx;
	outP->green = Gy;
	outP->blue = 0.0f;
	outP->alpha = 1.0f;

	return err;
}

static PF_Err
SobelVertical(
	void* refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32* inP,
	PF_Pixel32* outP) noexcept {
	PF_Err err = PF_Err_NONE;

	const Sobel* siP = reinterpret_cast<Sobel*>(refcon);

	const float grad1x = sampleIntegral128(*(siP->source), xL, yL - 1)->red;
	const float grad2x = sampleIntegral128(*(siP->source), xL, yL)->red;
	const float grad3x = sampleIntegral128(*(siP->source), xL, yL + 1)->red;
	const float grad1y = sampleIntegral128(*(siP->source), xL, yL - 1)->green;
	const float grad2y = sampleIntegral128(*(siP->source), xL, yL)->green;
	const float grad3y = sampleIntegral128(*(siP->source), xL, yL + 1)->green;

	float Gx = 3 * grad1x + 10 * grad2x + 3 * grad3x;
	float Gy = 3 * grad1y + 0 * grad2y  - 3 * grad3y;

	const float length = sqrt(Gx * Gx + Gy * Gy);
	Gx /= length;
	Gy /= length;

	float theta = atan2(Gy, Gx);

	outP->green = theta;

	outP->red = max(0.0f, length);
	outP->green = theta;
	outP->blue = 1 - isnan(theta);
	outP->alpha = 1.0f;

	return err;
}

static PF_Err
CreateAsciiRender(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32 *inP,
	PF_Pixel32 *outP) noexcept{
	PF_Err err = PF_Err_NONE;

	MainPass *miP = reinterpret_cast<MainPass*>(refcon);


	const int x = xL % 8;
	const int y = yL % 8;

	float theta;

	int directionTotal[5] = { 0,0,0,0,0 };

	for (int xt = 0; xt < 8; xt++) {
		for (int yt = 0; yt < 8; yt++) {
			theta = sampleIntegral128(*(miP->sobel), xL - x + xt, yL - y + yt)->green;
			int direction = 4;
			float absTheta = abs(theta) / M_PI;
			if (sampleIntegral128(*(miP->sobel), xL - x + xt, yL - y + yt)->blue == 1.0f) {
				if ((0.0f <= absTheta) && (absTheta < 0.05f)) direction = 0; // VERTICAL
				else if ((0.9f < absTheta) && (absTheta <= 1.0f)) direction = 0;
				else if ((0.45f < absTheta) && (absTheta < 0.55f)) direction = 1; // HORIZONTAL
				else if (0.05f < absTheta && absTheta < 0.45f) direction = theta > 0.0f ? 2 : 3; // DIAGONAL 1
				else if (0.55f < absTheta && absTheta < 0.9f) direction = theta > 0.0f ? 3 : 2; // DIAGONAL 2
			}
			directionTotal[direction]++;
		}
	}

	int biggestDirection = 0;
	for (int k = 1; k < 4; k++) {
		if (directionTotal[k] > directionTotal[biggestDirection]) {
			biggestDirection = k;
		}
	}

	int direction;

	if (directionTotal[biggestDirection] > miP->edge_threshold) {
		direction = biggestDirection;
	}
	else {
		direction = 4;
	}

	bool luminant;

	switch(direction) {
		case 0:
			luminant = ascci12[y][x];
			break;
		case 1:
			luminant = ascci13[y][x];
			break;
		case 2:
			luminant = ascci14[y][x];
			break;
		case 3:
			luminant = ascci15[y][x];
			break;
		default:
			float luminance = 0;

			for (int xt = 0; xt < 8; xt++) {
				for (int yt = 0; yt < 8; yt++) {
					luminance += calcLuminance(sampleIntegral128(*(miP->source), xL - x + xt, yL - y + yt));
				}
			}

			luminance /= 64;
			luminant = getAscii(luminance, x, y);
			break;
	}

	if (luminant) {
		if (miP->color) {
			outP->red = inP->red;
			outP->green = inP->green;
			outP->blue = inP->blue;
			outP->alpha = inP->alpha;
		}
		else {
			outP->red = 1.0f;
			outP->green = 1.0f;
			outP->blue = 1.0f;
			outP->alpha = 1.0f;
		}
	}
	else {
		if (miP->transparent) {
			outP->red = 0.0f;
			outP->green = 0.0f;
			outP->blue = 0.0f;
			outP->alpha = 0.0f;
		}
		else {
			outP->red = 0.0f;
			outP->green = 0.0f;
			outP->blue = 0.0f;
			outP->alpha = 1.0f;
		}
	}

	return err;
}

static PF_Err
ActuallyRender(
	PF_InData *in_data,
	PF_EffectWorld *input,
	PF_OutData *out_data,
	PF_EffectWorld *output,
	PF_ParamDef *params[])
{
	PF_Err err = PF_Err_NONE;
	// PF_Err				err2	= PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PF_PixelFormat format = PF_PixelFormat_INVALID;
	PF_NewWorldFlags flags = PF_NewWorldFlag_CLEAR_PIXELS;
	const PF_Boolean deep = PF_WORLD_IS_DEEP(output);

	if (deep)
	{
		flags |= PF_NewWorldFlag_DEEP_PIXELS;
	}

	PF_WorldSuite2 *wsP = NULL;
	ERR(AEFX_AcquireSuite(in_data, out_data, kPFWorldSuite, kPFWorldSuiteVersion2, "Couldn't load suite.", (void **)&wsP));

	if (!err)
	{
		ERR(wsP->PF_GetPixelFormat(input, &format));

		MainPass miP;
		AEFX_CLR_STRUCT(miP);

		miP.source = input;
		miP.color = params[COLOR_DISK_ID]->u.bd.value;
		miP.transparent = params[TRANSPARENT_DISK_ID]->u.bd.value;
		miP.edge_threshold = params[EDGE_DISK_ID]->u.sd.value;

		PF_EffectWorld maskWorld1, maskWorld2;
		ERR(wsP->PF_NewWorld(in_data->effect_ref, output->width, output->height, FALSE, format, &maskWorld1));
		ERR(wsP->PF_NewWorld(in_data->effect_ref, output->width, output->height, FALSE, format, &maskWorld2));

		switch (format)
		{
		case PF_PixelFormat_ARGB128:

			BlurPass1 biP1;
			biP1.source = input;
			biP1.kernel_size = params[KERNEL_SIZE_DISK_ID]->u.sd.value;
			biP1.sigma = params[SIGMA_DISK_ID]->u.fs_d.value;
			biP1.sigma_scale = params[SIGMA_SCALE_DISK_ID]->u.fs_d.value;
			if (miP.edge_threshold == -4) {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void*)&biP1, BlurPassOne, output));
				break;
			}
			else {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void*)&biP1, BlurPassOne, &maskWorld1));
			}

			BlurPass2 biP2;
			biP2.source = &maskWorld1;
			biP2.kernel_size = biP1.kernel_size;
			biP2.sigma = biP1.sigma;
			biP2.sigma_scale = biP1.sigma_scale;
			biP2.tau = params[TAU_DISK_ID]->u.fs_d.value;
			biP2.treshold = params[TRESHOLD_DISK_ID]->u.fs_d.value;
			if (miP.edge_threshold == -3) {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, maskWorld1.height, &maskWorld1, NULL, (void*)&biP2, BlurPassTwo, output));
				break;
			}
			else {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, maskWorld1.height, &maskWorld1, NULL, (void*)&biP2, BlurPassTwo, &maskWorld2));
			}
			
			Sobel siP;
			siP.source = &maskWorld2;
			if (miP.edge_threshold == -2) {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, maskWorld2.height, &maskWorld2, NULL, (void*)&siP, SobelHorizontal, output));
				break;
			}
			else {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, maskWorld2.height, &maskWorld2, NULL, (void*)&siP, SobelHorizontal, &maskWorld1));
			}
			siP.source = &maskWorld1;
			if (miP.edge_threshold == -1) {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, maskWorld1.height, &maskWorld1, NULL, (void*)&siP, SobelVertical, output));
				break;
			}
			else {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, maskWorld1.height, &maskWorld1, NULL, (void*)&siP, SobelVertical, &maskWorld2));
			}

			miP.sobel = &maskWorld2;
			ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void*)&miP, CreateAsciiRender, output));

			
			break;
		default:
			break;
		}
		ERR(wsP->PF_DisposeWorld(in_data->effect_ref, &maskWorld2));
		ERR(wsP->PF_DisposeWorld(in_data->effect_ref, &maskWorld1));
	}

	return err;
}

static PF_Err
SmartRender(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_SmartRenderExtra *extra)
{

	PF_Err err = PF_Err_NONE,
		   err2 = PF_Err_NONE;

	PF_EffectWorld *input_worldP = NULL,
				   *output_worldP = NULL;

	PF_ParamDef params[ASCII_NUM_PARAMS];
	PF_ParamDef *paramsP[ASCII_NUM_PARAMS];

	AEFX_CLR_STRUCT(params);

	for (int i = 0; i < ASCII_NUM_PARAMS; i++)
	{
		paramsP[i] = &params[i];
	}

	// checkout input & output buffers.
	ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, ASCII_INPUT, &input_worldP)));
	ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

	ERR(PF_CHECKOUT_PARAM(in_data, COLOR_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[ASCII_COLOR]));
	ERR(PF_CHECKOUT_PARAM(in_data, EDGE_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[ASCII_EDGE]));
	ERR(PF_CHECKOUT_PARAM(in_data, TRANSPARENT_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[ASCII_TRANSPARENT]));
	ERR(PF_CHECKOUT_PARAM(in_data, KERNEL_SIZE_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[ASCII_KERNEL_SIZE]));
	ERR(PF_CHECKOUT_PARAM(in_data, SIGMA_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[ASCII_SIGMA]));
	ERR(PF_CHECKOUT_PARAM(in_data, SIGMA_SCALE_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[ASCII_SIGMA_SCALE]));
	ERR(PF_CHECKOUT_PARAM(in_data, TAU_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[ASCII_TAU]));
	ERR(PF_CHECKOUT_PARAM(in_data, TRESHOLD_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[ASCII_TRESHOLD]));

	ERR(ActuallyRender(in_data,
					   input_worldP,
					   out_data,
					   output_worldP,
					   paramsP));

	// Always check in, no matter what the error condition!

	ERR2(PF_CHECKIN_PARAM(in_data, &params[ASCII_TRESHOLD]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[ASCII_TAU]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[ASCII_SIGMA_SCALE]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[ASCII_SIGMA]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[ASCII_KERNEL_SIZE]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[ASCII_TRANSPARENT]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[ASCII_EDGE]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[ASCII_COLOR]));

	return err;
}

extern "C" DllExport
	PF_Err
	PluginDataEntryFunction2(
		PF_PluginDataPtr inPtr,
		PF_PluginDataCB2 inPluginDataCallBackPtr,
		SPBasicSuite *inSPBasicSuitePtr,
		const char *inHostName,
		const char *inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT_EXT2(
		inPtr,
		inPluginDataCallBackPtr,
		"Ascii",													// Name
		"Kirschzelle Ascii",										// Match Name
		"Kirschzelle Plugins",										// Category
		AE_RESERVED_INFO,											// Reserved Info
		"EffectMain",												// Entry point
		"https://github.com/Kirschzelle/AdobeAfterEffectsPlugins"); // support URL

	return result;
}

static PF_Err
PreRender(
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_PreRenderExtra *extra)
{
	PF_Err err = PF_Err_NONE;
	PF_ParamDef channel_param;
	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result;

	AEGP_SuiteHandler suites(in_data->pica_basicP);

	req.channel_mask = PF_ChannelMask_ARGB;
	req.preserve_rgb_of_zero_alpha = TRUE;

	ERR(extra->cb->checkout_layer(in_data->effect_ref, ASCII_INPUT, ASCII_INPUT, &req, in_data->current_time, in_data->time_step, in_data->time_scale, &in_result));

	UnionLRect(&in_result.result_rect, &extra->output->result_rect);
	UnionLRect(&in_result.max_result_rect, &extra->output->max_result_rect);

	return err;
}

PF_Err
EffectMain(
	PF_Cmd cmd,
	PF_InData *in_data,
	PF_OutData *out_data,
	PF_ParamDef *params[],
	PF_LayerDef *output,
	void *extra)
{
	PF_Err err = PF_Err_NONE;

	try
	{
		switch (cmd)
		{
		case PF_Cmd_ABOUT:

			err = About(in_data,
						out_data,
						params,
						output);
			break;

		case PF_Cmd_GLOBAL_SETUP:

			err = GlobalSetup(in_data,
							  out_data,
							  params,
							  output);
			break;

		case PF_Cmd_PARAMS_SETUP:

			err = ParamsSetup(in_data,
							  out_data,
							  params,
							  output);
			break;

		case PF_Cmd_SMART_PRE_RENDER:
			err = PreRender(in_data, out_data, (PF_PreRenderExtra *)extra);
			break;

		case PF_Cmd_SMART_RENDER:

			err = SmartRender(in_data, out_data, (PF_SmartRenderExtra *)extra);
			break;
		}
	}
	catch (PF_Err &thrown_err)
	{
		err = thrown_err;
	}
	return err;
}
