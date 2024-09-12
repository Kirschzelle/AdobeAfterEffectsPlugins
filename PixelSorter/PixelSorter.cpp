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

/*	PixelSorter.cpp

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

#include "PixelSorter.h"

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

	PF_ADD_FLOAT_SLIDERX(STR(StrID_MASK_MIN_THRESHOLD_Param_Name), 0.0f, 1.0f, 0.0f, 1.0f, 0.4f, PF_Precision_TEN_THOUSANDTHS, 0, 0, MASK_MIN_THRESHOLD_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_MASK_MAX_THRESHOLD_Param_Name), 0.0f, 1.0f, 0.0f, 1.0f, 0.6f, PF_Precision_TEN_THOUSANDTHS, 0, 0, MASK_MAX_THRESHOLD_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_CHECKBOX(STR(StrID_HORIZONTAL_Param_Name), STR(StrID_NONE), true, 0, HORIZONTAL_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_SLIDER(STR(StrID_SORT_TYPE_Param_Name), 0, 8, 0, 8, 0, SORT_TYPE_DISK_ID);

	out_data->num_params = PIXELSORTER_NUM_PARAMS;

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
	const PF_Pixel32* pixel
) {
	return sqrt(0.299f * pixel->red * pixel->red + 0.587f * pixel->green * pixel->green + 0.114f * pixel->blue * pixel->blue) * pixel->alpha;
}

static PF_Err
CreateMask(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32 *inP,
	PF_Pixel32 *outP) noexcept
{
	PF_Err err = PF_Err_NONE;

	MaskGeneration * mgP = reinterpret_cast<MaskGeneration*>(refcon);

	const float luminance = calcLuminance(inP);

	if (luminance > mgP->lower_threshold && luminance < mgP->higher_threshold) {
		outP->red = 1.0f;
		outP->green = 1.0f;
		outP->blue = 1.0f;
		outP->alpha = 1.0f;
	}
	else {
		outP->red = 0.0f;
		outP->green = 0.0f;
		outP->blue = 0.0f;
		outP->alpha = 0.0f;
	}

	return err;
}

static PF_Err
PrepareVerticalImage(
	void* refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32* inP,
	PF_Pixel32* outP) noexcept
{
	PF_Err err = PF_Err_NONE;

	VerticalImagePreperation* viP = reinterpret_cast<VerticalImagePreperation*>(refcon);
	PF_EffectWorld source = *(viP->source);
	PF_EffectWorld output = *(viP->destination);

	PF_Pixel32* inputP = sampleIntegral128(source, xL, yL);
	PF_Pixel32* outputP = sampleIntegral128(output, xL, yL);

	outputP->red = inputP->red;
	outputP->green = inputP->green;
	outputP->blue = inputP->blue;
	outputP->alpha = inputP->alpha;
	
	return err;
}

static PF_Err
CopyImage(
	void* refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32* inP,
	PF_Pixel32* outP) noexcept
{
	PF_Err err = PF_Err_NONE;

	outP->red = inP->red;
	outP->green = inP->green;
	outP->blue = inP->blue;
	outP->alpha = inP->alpha;

	return err;
}

static bool isInMask(
	const PF_Pixel32* pixel
) noexcept {
	if (pixel->red == 1.0f) {
		return true;
	}
	return false;
}

static float Min(float fR, float fG, float fB)
{
	float fMin = fR;
	if (fG < fMin)
	{
		fMin = fG;
	}
	if (fB < fMin)
	{
		fMin = fB;
	}
	return fMin;
}


static float Max(float fR, float fG, float fB)
{
	float fMax = fR;
	if (fG > fMax)
	{
		fMax = fG;
	}
	if (fB > fMax)
	{
		fMax = fB;
	}
	return fMax;
}

static void RGBToHSL(float R, float G, float B, int& H, int& S, int& L)
{


	float fCMin = Min(R, G, B);
	float fCMax = Max(R, G, B);


	L = 50 * (fCMin + fCMax);

	if (fCMin == fCMax)
	{
		S = 0;
		H = 0;
		return;

	}
	else if (L < 50)
	{
		S = 100 * (fCMax - fCMin) / (fCMax + fCMin);
	}
	else
	{
		S = 100 * (fCMax - fCMin) / (2.0 - fCMax - fCMin);
	}

	if (fCMax == R)
	{
		H = 60 * (G - B) / (fCMax - fCMin);
	}
	if (fCMax == G)
	{
		H = 60 * (B - R) / (fCMax - fCMin) + 120;
	}
	if (fCMax == B)
	{
		H = 60 * (R - G) / (fCMax - fCMin) + 240;
	}
	if (H < 0)
	{
		H = H + 360;
	}
}

static float getSortValueFromPixel(
	const PF_Pixel32* pixel,
	const int sort_type
) {
	switch (sort_type) {
	case 0:
		return calcLuminance(pixel);
		break;
	case 1:
		return pixel->red;
		break;
	case 2:
		return  pixel->green;
		break;
	case 3:
		return  pixel->blue;
		break;
	case 4:
		return  pixel->alpha;
		break;
	case 5:
	case 6:
	case 7:
		int h, s, l;
		RGBToHSL(pixel->red, pixel->green, pixel->blue, h, s, l);
		if (sort_type == 5) {
			return h;
		}
		else if (sort_type == 6) {
			return s;
		}
		else {
			return l;
		}
	default:
		return rand() / double(RAND_MAX);
	}
}

static PF_Err
SortHorizontal(
	void* refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32* inP,
	PF_Pixel32* outP) noexcept
{
	PF_Err err = PF_Err_NONE;

	MainPass* miP = reinterpret_cast<MainPass*>(refcon);
	PF_EffectWorld source = *(miP->source);
	PF_EffectWorld mask = *(miP->mask);

	if (isInMask(sampleIntegral128(mask, xL, yL)) && (!isInMask(sampleIntegral128(mask, xL - 1, yL)) || xL == 0)) {
		int n = 0;
		for (int x = 1; xL+x< mask.width - 1 && isInMask(sampleIntegral128(mask, xL + x, yL)); x++, n++);
		while (n > 0) {
			for (int x = 0; x < n; x++) {
				PF_Pixel32* pixel1 = sampleIntegral128(source, xL + x, yL);
				PF_Pixel32* pixel2 = sampleIntegral128(source, xL + x + 1, yL);
				const float value1 = getSortValueFromPixel(pixel1, miP->sort_type);
				const float value2 = getSortValueFromPixel(pixel2, miP->sort_type);
				if (value1 > value2) {
					const float r = pixel1->red;
					const float g = pixel1->green;
					const float b = pixel1->blue;
					const float a = pixel1->alpha;
					pixel1->red = pixel2->red;
					pixel1->green = pixel2->green;
					pixel1->blue = pixel2->blue;
					pixel1->alpha = pixel2->alpha;
					pixel2->red = r;
					pixel2->green = g;
					pixel2->blue = b;
					pixel2->alpha = a;
				}
			}
			n--;
		}
	}

	return err;
}

static PF_Err
SortVertical(
	void* refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32* inP,
	PF_Pixel32* outP) noexcept
{
	PF_Err err = PF_Err_NONE;

	MainPass* miP = reinterpret_cast<MainPass*>(refcon);
	PF_EffectWorld source = *(miP->source);
	PF_EffectWorld mask = *(miP->mask);

	if (isInMask(sampleIntegral128(mask, xL, yL)) && (!isInMask(sampleIntegral128(mask, xL, yL - 1)) || yL == 0)) {
		int n = 0;
		for (int y = 1; yL + y < mask.height - 1 && isInMask(sampleIntegral128(mask, xL, yL + y)); y++, n++);
		while (n > 0) {
			for (int y = 0; y < n; y++) {
				PF_Pixel32* pixel1 = sampleIntegral128(source, xL, yL + y);
				PF_Pixel32* pixel2 = sampleIntegral128(source, xL, yL + y + 1);
				const float value1 = getSortValueFromPixel(pixel1, miP->sort_type);
				const float value2 = getSortValueFromPixel(pixel2, miP->sort_type);
				if (value1 > value2) {
					const float r = pixel1->red;
					const float g = pixel1->green;
					const float b = pixel1->blue;
					const float a = pixel1->alpha;
					pixel1->red = pixel2->red;
					pixel1->green = pixel2->green;
					pixel1->blue = pixel2->blue;
					pixel1->alpha = pixel2->alpha;
					pixel2->red = r;
					pixel2->green = g;
					pixel2->blue = b;
					pixel2->alpha = a;
				}
			}
			n--;
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

	PF_EffectWorld tempWorld1;

	if (deep)
	{
		flags |= PF_NewWorldFlag_DEEP_PIXELS;
	}

	PF_WorldSuite2 *wsP = NULL;
	ERR(AEFX_AcquireSuite(in_data, out_data, kPFWorldSuite, kPFWorldSuiteVersion2, "Couldn't load suite.", (void **)&wsP));

	PF_EffectWorld correctedInput;
	PF_EffectWorld maskWorld;

	if (!err)
	{
		ERR(wsP->PF_GetPixelFormat(input, &format));

		MainPass miP;
		AEFX_CLR_STRUCT(miP);

		ERR(wsP->PF_NewWorld(in_data->effect_ref, output->width, output->height, FALSE, format, &correctedInput));
		ERR(wsP->PF_NewWorld(in_data->effect_ref, output->width, output->height, FALSE, format, &maskWorld));

		miP.source = &correctedInput;
		miP.sort_type = params[SORT_TYPE_DISK_ID]->u.sd.value;

		switch (format)
		{
		case PF_PixelFormat_ARGB128:
			MaskGeneration mgP;
			mgP.lower_threshold = params[MASK_MIN_THRESHOLD_DISK_ID]->u.fs_d.value;
			mgP.higher_threshold = params[MASK_MAX_THRESHOLD_DISK_ID]->u.fs_d.value;
			if (miP.sort_type == 0) {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void*)&mgP, CreateMask, output));
				break;
			}
			ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void*)&mgP, CreateMask, &maskWorld));
			miP.mask = &maskWorld;

			ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, NULL, CopyImage, &correctedInput));

			if (params[HORIZONTAL_DISK_ID]->u.bd.value) {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, correctedInput.height, &correctedInput, NULL, (void*)&miP, SortHorizontal, &correctedInput));
			}
			else {
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, correctedInput.height, &correctedInput, NULL, (void*)&miP, SortVertical, &correctedInput));
			}

			ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, correctedInput.height, &correctedInput, NULL, NULL, CopyImage, output));

			break;
		default:
			break;
		}

		ERR(wsP->PF_DisposeWorld(in_data->effect_ref, &maskWorld));
		ERR(wsP->PF_DisposeWorld(in_data->effect_ref, &correctedInput));
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

	PF_ParamDef params[PIXELSORTER_NUM_PARAMS];
	PF_ParamDef *paramsP[PIXELSORTER_NUM_PARAMS];

	AEFX_CLR_STRUCT(params);

	for (int i = 0; i < PIXELSORTER_NUM_PARAMS; i++)
	{
		paramsP[i] = &params[i];
	}

	// checkout input & output buffers.
	ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, PIXELSORTER_INPUT, &input_worldP)));
	ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

	ERR(PF_CHECKOUT_PARAM(in_data, MASK_MIN_THRESHOLD_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[PIXELSORTER_MASK_MIN_THRESHOLD]));
	ERR(PF_CHECKOUT_PARAM(in_data, MASK_MAX_THRESHOLD_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[PIXELSORTER_MASK_MAX_THRESHOLD]));
	ERR(PF_CHECKOUT_PARAM(in_data, HORIZONTAL_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[PIXELSORTER_HORIZONTAL]));
	ERR(PF_CHECKOUT_PARAM(in_data, SORT_TYPE_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[PIXELSORTER_SORT_TYPE]));

	ERR(ActuallyRender(in_data,
					   input_worldP,
					   out_data,
					   output_worldP,
					   paramsP));

	// Always check in, no matter what the error condition!

	ERR2(PF_CHECKIN_PARAM(in_data, &params[PIXELSORTER_SORT_TYPE]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[PIXELSORTER_HORIZONTAL]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[PIXELSORTER_MASK_MAX_THRESHOLD]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[PIXELSORTER_MASK_MIN_THRESHOLD]));

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
		"PixelSorter",						 // Name
		"Kirschzelle PixelSorter",				 // Match Name
		"Kirschzelle Plugins",					 // Category
		AE_RESERVED_INFO,					 // Reserved Info
		"EffectMain",						 // Entry point
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

	ERR(extra->cb->checkout_layer(in_data->effect_ref, PIXELSORTER_INPUT, PIXELSORTER_INPUT, &req, in_data->current_time, in_data->time_step, in_data->time_scale, &in_result));

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
