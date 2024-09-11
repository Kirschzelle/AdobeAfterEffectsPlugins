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

/*	Kuwahara.cpp

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

#include "Kuwahara.h"

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

	PF_ADD_SLIDER(STR(StrID_KERNEL_SIZE_Param_Name), 2, 20, 2, 20, 10, KERNEL_SIZE_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_SHARPNESS_Param_Name), 1, 18.0f, 1.0f, 18.0f, 8.0f, PF_Precision_TEN_THOUSANDTHS, 0, 0, SHARPNESS_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_HARDNESS_Param_Name), 1.0f, 100.0f, 1.0f, 100.0f, 8.0f, PF_Precision_TEN_THOUSANDTHS, 0, 0, HARDNESS_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_ALPHA_Param_Name), 0.01f, 2.0f, 0.01f, 2.0f, 1.0f, PF_Precision_TEN_THOUSANDTHS, 0, 0, ALPHA_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_ZERO_CROSSING_Param_Name), 0.01f, 2.0f, 0.01f, 2.0f, 1.0f, PF_Precision_TEN_THOUSANDTHS, 0, 0, ZERO_CROSSING_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_CHECKBOX(STR(StrID_ZETA_Param_Name), STR(StrID_NONE), true, 0, ZETA_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_ZETA_VALUE_Param_Name), 0.01f, 3.0f, 0.01f, 3.0f, 0.4f, PF_Precision_TEN_THOUSANDTHS, 0, 0, ZETA_VALUE_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_SLIDER(STR(StrID_PASSES_Param_Name), -2, 4, 1, 4, 1, PASSES_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_CHECKBOX(STR(StrID_WILD_Param_Name), STR(StrID_NONE), false, 0, WILD_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_SLIDER(STR(StrID_N_Param_Name), 1, 8, 8, 8, 8, N_DISK_ID);

	out_data->num_params = KUWAHARA_NUM_PARAMS;

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

static PF_Err
Eigenvectors(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32 *inP,
	PF_Pixel32 *outP) noexcept
{
	PF_Err err = PF_Err_NONE;

	SourceInfo *giP = reinterpret_cast<SourceInfo *>(refcon);
	PF_EffectWorld def = *(giP->source);

	PF_FpShort redx;
	PF_FpShort greenx;
	PF_FpShort bluex;
	PF_FpShort redy;
	PF_FpShort greeny;
	PF_FpShort bluey;
	PF_Pixel32 *pixel1 = sampleIntegral128(def, xL - 1, yL - 1);
	PF_Pixel32 *pixel2 = sampleIntegral128(def, xL - 1, yL);
	PF_Pixel32 *pixel3 = sampleIntegral128(def, xL - 1, yL + 1);
	PF_Pixel32 *pixel4 = sampleIntegral128(def, xL + 1, yL - 1);
	PF_Pixel32 *pixel5 = sampleIntegral128(def, xL + 1, yL);
	PF_Pixel32 *pixel6 = sampleIntegral128(def, xL + 1, yL + 1);
	redx = (pixel1->red * 1.0f + pixel2->red * 2.0f + pixel3->red * 1.0f - pixel4->red * 1.0f - pixel5->red * 2.0f - pixel6->red * 1.0f) / 4.0f;
	greenx = (pixel1->green * 1.0f + pixel2->green * 2.0f + pixel3->green * 1.0f - pixel4->green * 1.0f - pixel5->green * 2.0f - pixel6->green * 1.0f) / 4.0f;
	bluex = (pixel1->blue * 1.0f + pixel2->blue * 2.0f + pixel3->blue * 1.0f - pixel4->blue * 1.0f - pixel5->blue * 2.0f - pixel6->blue * 1.0f) / 4.0f;
	pixel1 = sampleIntegral128(def, xL + 1, yL - 1);
	pixel2 = sampleIntegral128(def, xL, yL - 1);
	pixel3 = sampleIntegral128(def, xL - 1, yL - 1);
	pixel4 = sampleIntegral128(def, xL + 1, yL + 1);
	pixel5 = sampleIntegral128(def, xL, yL + 1);
	pixel6 = sampleIntegral128(def, xL - 1, yL + 1);
	redy = (pixel1->red * 1.0f + pixel2->red * 2.0f + pixel3->red * 1.0f - pixel4->red * 1.0f - pixel5->red * 2.0f - pixel6->red * 1.0f) / 4.0f;
	greeny = (pixel1->green * 1.0f + pixel2->green * 2.0f + pixel3->green * 1.0f - pixel4->green * 1.0f - pixel5->green * 2.0f - pixel6->green * 1.0f) / 4.0f;
	bluey = (pixel1->blue * 1.0f + pixel2->blue * 2.0f + pixel3->blue * 1.0f - pixel4->blue * 1.0f - pixel5->blue * 2.0f - pixel6->blue * 1.0f) / 4.0f;

	outP->red = redx * redx + greenx * greenx + bluex * bluex;
	outP->green = redy * redy + greeny * greeny + bluey * bluey;
	outP->blue = redx * redy + greenx * greeny + bluex * bluey;
	outP->alpha = 1.0f;

	return err;
}

static PF_Err
BlurPassOne(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32 *inP,
	PF_Pixel32 *outP) noexcept
{
	PF_Err err = PF_Err_NONE;

	SourceInfo *giP = reinterpret_cast<SourceInfo *>(refcon);
	PF_EffectWorld def = *(giP->source);

	constexpr int kernelRadius = 5;
	PF_FpShort colr = 0.0f;
	PF_FpShort colg = 0.0f;
	PF_FpShort colb = 0.0f;
	PF_FpShort cola = 0.0f;
	float kernelSum = 0.0f;

	for (int x = -kernelRadius; x <= kernelRadius; ++x)
	{
		const PF_Pixel32 *pixel = sampleIntegral128(def, xL + x, yL);
		const float gaus = gaussian(2.0f, x);
		colr += pixel->red * gaus;
		colg += pixel->green * gaus;
		colb += pixel->blue * gaus;
		cola += pixel->alpha * gaus;
		kernelSum += gaus;
	}

	outP->red = colr / kernelSum;
	if (giP->wild)
	{
		outP->green = colb / kernelSum;
		outP->blue = colg / kernelSum;
	}
	else
	{
		outP->green = colg / kernelSum;
		outP->blue = colb / kernelSum;
	}
	outP->alpha = cola / kernelSum;

	return err;
}

static PF_Err
BlurPassTwo(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32 *inP,
	PF_Pixel32 *outP) noexcept
{
	PF_Err err = PF_Err_NONE;

	SourceInfo *giP = reinterpret_cast<SourceInfo *>(refcon);
	PF_EffectWorld def = *(giP->source);

	constexpr int kernelRadius = 5;
	PF_FpShort colr = 0.0f;
	PF_FpShort colg = 0.0f;
	PF_FpShort colb = 0.0f;
	PF_FpShort cola = 0.0f;
	float kernelSum = 0.0f;

	for (int y = -kernelRadius; y <= kernelRadius; ++y)
	{
		const PF_Pixel32 *pixel = sampleIntegral128(def, xL, yL + y);
		const float gaus = gaussian(2.0f, y);
		colr += pixel->red * gaus;
		colg += pixel->green * gaus;
		colb += pixel->blue * gaus;
		cola += pixel->alpha * gaus;
		kernelSum += gaus;
	}

	const PF_FpShort resr = colr / kernelSum;
	const PF_FpShort resg = colg / kernelSum;
	const PF_FpShort resb = colb / kernelSum;

	const float variance = sqrt(resg * resg - 2.0f * resr * resg + resr * resr + 4.0f * resb * resb);

	const float lambda1 = 0.5f * (resg + resr + variance);
	const float lambda2 = 0.5f * (resg + resr - variance);

	float vx = lambda1 - resr;
	float vy = -resb;

	const float vlength = sqrt(vx * vx + vy * vy);
	if (vlength > 0.0f)
	{
		vx = vx / vlength;
		vy = vy / vlength;
	}
	else
	{
		vx = 0.0f;
		vy = 1.0f;
	}

	const float phi = -atan2(vy, vx);

	float a;
	if (lambda1 + lambda2 > 0.0f)
	{
		a = (lambda1 - lambda2) / (lambda1 + lambda2);
	}
	else
	{
		a = 0.0f;
	}

	outP->red = vx;
	if (giP->wild)
	{
		outP->blue = vy;
		outP->green = phi;
	}
	else
	{
		outP->green = vy;
		outP->blue = phi;
	}
	outP->alpha = a;

	return err;
}

static PF_Err
KuwaharaFilter(
	void *refcon,
	A_long xL,
	A_long yL,
	PF_Pixel32 *inP,
	PF_Pixel32 *outP) noexcept
{
	PF_Err err = PF_Err_NONE;

	const MainPass *miP = reinterpret_cast<MainPass *>(refcon);
	PF_EffectWorld input = *(miP->source);

	const int kernelRadius = miP->kernel_size / 2;
	float a, b;
	if (miP->wild)
	{
		a = kernelRadius * min(max((miP->alpha + inP->alpha) / miP->alpha, 0.1f), 2.0f);
		b = kernelRadius * min(max(miP->alpha / (miP->alpha + inP->alpha), 0.1f), 2.0f);
	}
	else
	{
		a = float(kernelRadius) * min(max((miP->alpha + inP->alpha) / miP->alpha, 0.1f), 2.0f);
		b = float(kernelRadius) * min(max(miP->alpha / (miP->alpha + inP->alpha), 0.1f), 2.0f);
	}

	float cos_phi = cos(inP->blue);
	float sin_phi = sin(inP->blue);

	float R[2][2] = {{cos_phi, -sin_phi},
					 {sin_phi, cos_phi}};

	float S[2][2] = {{0.5f / a, 0.0f},
					 {0.0f, 0.5f / b}};

	float SR[2][2] = {{S[0][0] * R[0][0] + S[0][1] * R[1][0], S[0][0] * R[0][1] + S[0][1] * R[1][1]},
					  {S[1][0] * R[0][0] + S[1][1] * R[1][0], S[1][0] * R[0][1] + S[1][1] * R[1][1]}};

	const int max_x = int(sqrt(a * a * cos_phi * cos_phi + b * b * sin_phi * sin_phi));
	const int max_y = int(sqrt(a * a * sin_phi * sin_phi + b * b * cos_phi * cos_phi));

	const float sinZeroCross = sin(miP->zero_crossing);
	const float eta = (miP->zeta + cos(miP->zero_crossing)) / (sinZeroCross * sinZeroCross);
	int k;
	PF_Pixel32 m[8];
	float s[8][3];

	for (k = 0; k < miP->n; k++)
	{
		m[k].alpha = 0.0f;
		m[k].red = 0.0f;
		m[k].green = 0.0f;
		m[k].blue = 0.0f;
		s[k][0] = 0.0f;
		s[k][1] = 0.0f;
		s[k][2] = 0.0f;
	}

	for (int y = -max_y; y <= max_y; y++)
	{
		for (int x = -max_x; x <= max_x; x++)
		{
			float v[2] = {SR[0][0] * x + SR[0][1] * y, SR[1][0] * x + SR[1][1] * y};
			if (v[0] * v[0] + v[1] * v[1] <= 0.25f)
			{
				PF_Pixel32 *c = sampleIntegral128(input, xL + x, yL + y);
				PF_FpShort cred, cgreen, cblue;
				if (miP->wild)
				{
					cred = c->red;
					cgreen = c->green;
					cblue = c->blue;
				}
				else
				{
					cred = min(1.0f, max(0.0f, c->red));
					cgreen = min(1.0f, max(0.0f, c->green));
					cblue = min(1.0f, max(0.0f, c->blue));
				}
				float sum = 0;
				float w[8];
				float z, vxx, vyy;

				vxx = miP->zeta - eta * v[0] * v[0];
				vyy = miP->zeta - eta * v[1] * v[1];
				z = max(0, v[1] + vxx);
				w[0] = z * z;
				sum += w[0];
				z = max(0, -v[0] + vyy);
				w[2] = z * z;
				sum += w[2];
				z = max(0, -v[1] + vxx);
				w[4] = z * z;
				sum += w[4];
				z = max(0, v[0] + vyy);
				w[6] = z * z;
				sum += w[6];

				float vTempX = sqrt(2.0f) / 2.0f * (v[0] - v[1]);
				v[1] = sqrt(2.0f) / 2.0f * (v[0] + v[1]);
				v[0] = vTempX;
				vxx = miP->zeta - eta * v[0] * v[0];
				vyy = miP->zeta - eta * v[1] * v[1];
				z = max(0, v[1] + vxx);
				w[1] = z * z;
				sum += w[1];
				z = max(0, -v[0] + vyy);
				w[3] = z * z;
				sum += w[3];
				z = max(0, -v[1] + vxx);
				w[5] = z * z;
				sum += w[5];
				z = max(0, v[0] + vyy);
				w[7] = z * z;
				sum += w[7];

				const float g = exp(-3.125f * (v[0] * v[0] + v[1] * v[1])) / sum;

				for (int l = 0; l < 8; l++)
				{
					float wl = w[l] * g;
					if (miP->wild)
					{
						m[l].red = cred * wl;
						m[l].green = cgreen * wl;
						m[l].blue = cblue * wl;
						m[l].alpha = wl;
						s[l][0] = cred * cred * wl;
						s[l][1] = cblue * cblue * wl;
						s[l][2] = cgreen * cgreen * wl;
					}
					else
					{
						m[l].red += cred * wl;
						m[l].green += cgreen * wl;
						m[l].blue += cblue * wl;
						m[l].alpha += wl;
						s[l][0] += cred * cred * wl;
						s[l][1] += cgreen * cgreen * wl;
						s[l][2] += cblue * cblue * wl;
					}
				}
			}
		}
	}

	outP->red = 0;
	outP->blue = 0;
	outP->green = 0;
	outP->alpha = 0;

	for (k = 0; k < miP->n; k++)
	{
		m[k].red /= m[k].alpha;
		m[k].green /= m[k].alpha;
		m[k].blue /= m[k].alpha;
		s[k][0] = abs(s[k][0] / m[k].alpha - m[k].red * m[k].red);
		s[k][1] = abs(s[k][1] / m[k].alpha - m[k].green * m[k].green);
		s[k][2] = abs(s[k][2] / m[k].alpha - m[k].blue * m[k].blue);

		float sigma2 = s[k][0] + s[k][1] + s[k][2];
		float w = 1.0f / (1.0f + pow(miP->hardnesss * 1000.0f * sigma2, 0.5f * miP->sharpness));

		outP->red += m[k].red * w;
		if (miP->wild)
		{
			outP->blue += m[k].green * w;
			outP->green += m[k].blue * w;
		}
		else
		{
			outP->green += m[k].green * w;
			outP->blue += m[k].blue * w;
		}
		outP->alpha += w;
	}

	outP->red = max(0.0f, min(1.0f, outP->red / outP->alpha));
	outP->green = max(0.0f, min(1.0f, outP->green / outP->alpha));
	outP->blue = max(0.0f, min(1.0f, outP->blue / outP->alpha));
	outP->alpha = max(0.0f, min(1.0f, outP->alpha / outP->alpha));

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
	PF_EffectWorld tempWorld2;

	if (deep)
	{
		flags |= PF_NewWorldFlag_DEEP_PIXELS;
	}

	PF_WorldSuite2 *wsP = NULL;
	ERR(AEFX_AcquireSuite(in_data, out_data, kPFWorldSuite, kPFWorldSuiteVersion2, "Couldn't load suite.", (void **)&wsP));

	if (!err)
	{
		ERR(wsP->PF_GetPixelFormat(input, &format));

		ERR(wsP->PF_NewWorld(in_data->effect_ref, output->width, output->height, FALSE, format, &tempWorld1));
		ERR(wsP->PF_NewWorld(in_data->effect_ref, output->width, output->height, FALSE, format, &tempWorld2));

		SourceInfo giP;
		AEFX_CLR_STRUCT(giP);
		MainPass miP;
		AEFX_CLR_STRUCT(miP);

		giP.wild = params[WILD_DISK_ID]->u.bd.value;
		miP.wild = giP.wild;

		const int passes = params[PASSES_DISK_ID]->u.sd.value;

		switch (format)
		{
		case PF_PixelFormat_ARGB128:

			giP.source = input;
			if (passes == -2)
			{
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void *)&giP, Eigenvectors, output));
			}
			else
			{
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void *)&giP, Eigenvectors, &tempWorld1));
			}
			giP.source = &tempWorld1;
			if (passes == -1)
			{
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void *)&giP, BlurPassOne, output));
			}
			else
			{
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void *)&giP, BlurPassOne, &tempWorld2));
			}
			giP.source = &tempWorld2;
			if (passes == 0)
			{
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void *)&giP, BlurPassTwo, output));
			}
			else
			{
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, input->height, input, NULL, (void *)&giP, BlurPassTwo, &tempWorld1));
			}
			miP.source = input;
			miP.kernel_size = params[KERNEL_SIZE_DISK_ID]->u.sd.value;
			miP.sharpness = params[SHARPNESS_DISK_ID]->u.fs_d.value;
			miP.hardnesss = params[HARDNESS_DISK_ID]->u.fs_d.value;
			miP.alpha = params[ALPHA_DISK_ID]->u.fs_d.value;
			miP.zero_crossing = params[ZERO_CROSSING_DISK_ID]->u.fs_d.value;
			miP.zeta = params[ZETA_DISK_ID]->u.bd.value ? params[ZETA_VALUE_DISK_ID]->u.fs_d.value : 2.0f / 2.0f / (miP.kernel_size / 2.0f);
			miP.n = params[N_DISK_ID]->u.sd.value;
			if (passes == 1)
			{
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, tempWorld1.height, &tempWorld1, NULL, (void *)&miP, KuwaharaFilter, output));
			}
			else if (passes == 2)
			{
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, tempWorld1.height, &tempWorld1, NULL, (void *)&miP, KuwaharaFilter, &tempWorld1));
				miP.source = &tempWorld1;
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, tempWorld1.height, &tempWorld1, NULL, (void *)&miP, KuwaharaFilter, output));
			}
			else if (passes == 3)
			{
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, tempWorld1.height, &tempWorld1, NULL, (void *)&miP, KuwaharaFilter, output));
				miP.source = output;
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, tempWorld1.height, &tempWorld1, NULL, (void *)&miP, KuwaharaFilter, &tempWorld1));
				miP.source = &tempWorld1;
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, tempWorld1.height, &tempWorld1, NULL, (void *)&miP, KuwaharaFilter, output));
			}
			else if (passes == 4)
			{
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, tempWorld1.height, &tempWorld1, NULL, (void *)&miP, KuwaharaFilter, &tempWorld1));
				miP.source = &tempWorld1;
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, tempWorld1.height, &tempWorld1, NULL, (void *)&miP, KuwaharaFilter, output));
				miP.source = output;
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, tempWorld1.height, &tempWorld1, NULL, (void *)&miP, KuwaharaFilter, &tempWorld1));
				miP.source = &tempWorld1;
				ERR(suites.IterateFloatSuite2()->iterate(in_data, 0, tempWorld1.height, &tempWorld1, NULL, (void *)&miP, KuwaharaFilter, output));
			}
			break;
		default:
			break;
		}
	}

	ERR(wsP->PF_DisposeWorld(in_data->effect_ref, &tempWorld1));
	ERR(wsP->PF_DisposeWorld(in_data->effect_ref, &tempWorld2));

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

	PF_ParamDef params[KUWAHARA_NUM_PARAMS];
	PF_ParamDef *paramsP[KUWAHARA_NUM_PARAMS];

	AEFX_CLR_STRUCT(params);

	for (int i = 0; i < KUWAHARA_NUM_PARAMS; i++)
	{
		paramsP[i] = &params[i];
	}

	// checkout input & output buffers.
	ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, KUWAHARA_INPUT, &input_worldP)));
	ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

	ERR(PF_CHECKOUT_PARAM(in_data, KERNEL_SIZE_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[KUWAHARA_KERNEL_SIZE]));
	ERR(PF_CHECKOUT_PARAM(in_data, SHARPNESS_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[KUWAHARA_SHARPNESS]));
	ERR(PF_CHECKOUT_PARAM(in_data, HARDNESS_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[KUWAHARA_HARDNESS]));
	ERR(PF_CHECKOUT_PARAM(in_data, ALPHA_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[KUWAHARA_ALPHA]));
	ERR(PF_CHECKOUT_PARAM(in_data, ZERO_CROSSING_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[KUWAHARA_ZERO_CROSSING]));
	ERR(PF_CHECKOUT_PARAM(in_data, ZETA_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[KUWAHARA_ZETA]));
	ERR(PF_CHECKOUT_PARAM(in_data, ZETA_VALUE_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[KUWAHARA_ZETA_VALUE]));
	ERR(PF_CHECKOUT_PARAM(in_data, PASSES_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[KUWAHARA_PASSES]));
	ERR(PF_CHECKOUT_PARAM(in_data, WILD_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[KUWAHARA_WILD]));
	ERR(PF_CHECKOUT_PARAM(in_data, N_DISK_ID, in_data->current_time, in_data->time_step, in_data->time_scale, &params[KUWAHARA_N]));

	ERR(ActuallyRender(in_data,
					   input_worldP,
					   out_data,
					   output_worldP,
					   paramsP));

	// Always check in, no matter what the error condition!

	ERR2(PF_CHECKIN_PARAM(in_data, &params[KUWAHARA_N]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[KUWAHARA_WILD]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[KUWAHARA_PASSES]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[KUWAHARA_ZETA_VALUE]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[KUWAHARA_ZETA]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[KUWAHARA_ZERO_CROSSING]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[KUWAHARA_ALPHA]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[KUWAHARA_HARDNESS]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[KUWAHARA_SHARPNESS]));
	ERR2(PF_CHECKIN_PARAM(in_data, &params[KUWAHARA_KERNEL_SIZE]));

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
		"Kuwahara",							 // Name
		"Kirschzelle Kuwahara",					 // Match Name
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

	ERR(extra->cb->checkout_layer(in_data->effect_ref, KUWAHARA_INPUT, KUWAHARA_INPUT, &req, in_data->current_time, in_data->time_step, in_data->time_scale, &in_result));

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
