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

/*	Dither.cpp

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

#include "Dither.h"
#include <iostream>
#include <list>
using namespace std;

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

	PF_ADD_SLIDER(STR(StrID_COLORS_RED_Param_Name), 1, 16, 1, 16, 4, COLORS_RED_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_SLIDER(STR(StrID_COLORS_GREEN_Param_Name), 1, 16, 1, 16, 4, COLORS_GREEN_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_SLIDER(STR(StrID_COLORS_BLUE_Param_Name), 1, 16, 1, 16, 4, COLORS_BLUE_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_CHECKBOX(STR(StrID_COLORS_ADAPT_Param_Name), STR(StrID_NONE), false, 0, COLORS_ADAPT_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_SLIDER(STR(StrID_N_Param_Name), 1, 3, 1, 3, 2, N_DISK_ID);

	out_data->num_params = DITHER_NUM_PARAMS;

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

static float getNearestFloat(
	float colors[16],
	float value
) noexcept {
	float lowest_distance = abs(value - colors[0]);
	int best_index = 0;
	for (int k = 1; k < 16; k++) {
		if (abs(value - colors[k]) < lowest_distance && colors[k] != -1.0f) {
			lowest_distance = abs(value - colors[k]);
			best_index = k;
		}
	}
	return colors[best_index];
}

static float getCorrectMValue(
	int n,
	int x,
	int y
) {
	switch (n) {
		case 1:
			return 1.0f/4.0f * float(bayer1[y % 2][x % 2]);
			break;
		case 2:
			return 1.0f / 16.0f * float(bayer2[y % 4][x % 4]);
			break;
		case 3:
			return 1.0f / 64.0f * float(bayer3[y % 8][x % 8]);
			break;
		//default:
			//return bit_reverse(bit_interleave(bitwise_xor(i, j), i)) / n ^ 2; --> Would give any table for 2^n
	}
}

static PF_Err
Dither(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32 *inP,
	PF_Pixel32 *outP) noexcept
{
	PF_Err err = PF_Err_NONE;

	MainPass *miP = reinterpret_cast<MainPass *>(refcon);

	outP->red = getNearestFloat(miP->colors_red, inP->red + ((1.0f / float(miP->n)) * (getCorrectMValue(miP->n, xL, yL) - 0.5f)));
	outP->green = getNearestFloat(miP->colors_green, inP->green + ((1.0f / float(miP->n)) * (getCorrectMValue(miP->n, xL, yL) - 0.5f)));
	outP->blue = getNearestFloat(miP->colors_blue, inP->blue + ((1.0f / float(miP->n)) * (getCorrectMValue(miP->n, xL, yL) - 0.5f)));
	outP->alpha = inP->alpha;

	return err;
}

static void fillColorArray(
	int max_colors,
	float colors[16]
) {
	for (int c = 0; c < 16; c++) {
		if (c < max_colors) {
			colors[c - 1] = (float(c) / (max_colors - 1));
		}
		else {
			colors[c - 1] = -1.0f;
		}
	}
}

static void getColorArrayFromList(
	int max_colors,
	list<float> values,
	float colors[16]
) {
	int size = values.size();
	int index = 0;
	for (int c = 0; c < 16; c++) {
		if (c < max_colors) {
			while(index < size) {
				if (index >= (size / max_colors - 1)*c) {
					colors[c] = values.front();
					break;
				}
				values.pop_front();
				index++;
			}
		}
		else {
			colors[c] = -1.0f;
		}
	}
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
		miP.n = params[N_DISK_ID]->u.sd.value;

		switch (format)
		{
		case PF_PixelFormat_ARGB128:
			if (params[COLORS_ADAPT_DISK_ID]->u.bd.value) {
				list<float> reds;
				list<float> greens;
				list<float> blues;
				for (int y = 0; y < input->height; y++) {
					for (int x = 0; x < input->width; x++) {
						PF_Pixel32* pixel = sampleIntegral128(*input, x, y);
						reds.push_back(pixel->red);
						greens.push_back(pixel->green);
						blues.push_back(pixel->blue);
					}
				}
				reds.sort();
				greens.sort();
				blues.sort();
				getColorArrayFromList(params[COLORS_RED_DISK_ID]->u.sd.value, reds, miP.colors_red);
				getColorArrayFromList(params[COLORS_GREEN_DISK_ID]->u.sd.value, greens, miP.colors_green);
				getColorArrayFromList(params[COLORS_BLUE_DISK_ID]->u.sd.value, blues, miP.colors_blue);
			}
			else {
				fillColorArray(params[COLORS_RED_DISK_ID]->u.sd.value, miP.colors_red);
				fillColorArray(params[COLORS_GREEN_DISK_ID]->u.sd.value, miP.colors_green);
				fillColorArray(params[COLORS_BLUE_DISK_ID]->u.sd.value, miP.colors_blue);
			}

			ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void *)&miP, Dither, output));

			break;
		default:
			break;
		}
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

	PF_ParamDef params[DITHER_NUM_PARAMS];
	PF_ParamDef *paramsP[DITHER_NUM_PARAMS];

	AEFX_CLR_STRUCT(params);

	for (int i = 0; i < DITHER_NUM_PARAMS; i++)
	{
		paramsP[i] = &params[i];
	}

	// checkout input & output buffers.
	ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, DITHER_INPUT, &input_worldP)));
	ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

	ERR(PF_CHECKOUT_PARAM(in_data, COLORS_RED_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[DITHER_COLORS_RED]));
	ERR(PF_CHECKOUT_PARAM(in_data, COLORS_GREEN_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[DITHER_COLORS_GREEN]));
	ERR(PF_CHECKOUT_PARAM(in_data, COLORS_BLUE_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[DITHER_COLORS_BLUE]));
	ERR(PF_CHECKOUT_PARAM(in_data, COLORS_ADAPT_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[DITHER_COLORS_ADAPT]));
	ERR(PF_CHECKOUT_PARAM(in_data, N_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[DITHER_N]));

	ERR(ActuallyRender(in_data,
					   input_worldP,
					   out_data,
					   output_worldP,
					   paramsP));

	// Always check in, no matter what the error condition!

	ERR2(PF_CHECKIN_PARAM(in_data, &params[DITHER_N]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[DITHER_COLORS_ADAPT]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[DITHER_COLORS_BLUE]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[DITHER_COLORS_GREEN]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[DITHER_COLORS_RED]));

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
		"Dither",													// Name
		"Kirschzelle Dither",										// Match Name
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

	ERR(extra->cb->checkout_layer(in_data->effect_ref, DITHER_INPUT, DITHER_INPUT, &req, in_data->current_time, in_data->time_step, in_data->time_scale, &in_result));

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
