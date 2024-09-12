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

/*
	Dither.h
*/

#pragma once

#ifndef DITHER_H
#define DITHER_H

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned short u_int16;
typedef unsigned long u_long;
typedef short int int16;
#define PF_TABLE_BITS 12
#define PF_TABLE_SZ_16 4096

#define PF_DEEP_COLOR_AWARE 1 // make sure we get 16bpc pixels;
							  // AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
typedef unsigned short PixelType;
#include <Windows.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"
#include "AEConfig.h"
#include "AEFX_SuiteHelper.h"
#include "Smart_Utils.h"

#include "Dither_Strings.h"

/* Versioning information */

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define BUG_VERSION 0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 1

/* Parameter defaults */

enum
{
	DITHER_INPUT = 0,
	DITHER_COLORS_RED,
	DITHER_COLORS_GREEN,
	DITHER_COLORS_BLUE,
	DITHER_COLORS_ADAPT,
	DITHER_N,
	DITHER_NUM_PARAMS
};

enum
{
	COLORS_RED_DISK_ID = 1,
	COLORS_GREEN_DISK_ID,
	COLORS_BLUE_DISK_ID,
	COLORS_ADAPT_DISK_ID,
	N_DISK_ID,
	TRESHOLD_DISK_ID
};

typedef struct MainPass
{
	float colors_red[16];
	float colors_green[16];
	float colors_blue[16];
	int n;
};

extern "C"
{

	DllExport
		PF_Err
		EffectMain(
			PF_Cmd cmd,
			PF_InData *in_data,
			PF_OutData *out_data,
			PF_ParamDef *params[],
			PF_LayerDef *output,
			void *extra);
}

static const int bayer1[2][2] = {
	{0, 2},
	{3, 1}
};

static const int bayer2[4][4] = {
	{0, 8, 2, 10},
	{12, 4, 14, 6},
	{3, 11, 1, 9},
	{15, 7, 13, 5}
};

static const int bayer3[8][8] = {
	{0, 32, 8, 40, 2, 34, 10, 42},
	{48, 16, 56, 24, 50, 18, 58, 26},
	{12, 44,  4, 36, 14, 46,  6, 38},
	{60, 28, 52, 20, 62, 30, 54, 22},
	{3, 35, 11, 43,  1, 33,  9, 41},
	{51, 19, 59, 27, 49, 17, 57, 25},
	{15, 47,  7, 39, 13, 45,  5, 37},
	{63, 31, 55, 23, 61, 29, 53, 21}
};

#endif // DITHER_H