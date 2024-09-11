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
	Ascii.h
*/

#pragma once

#ifndef ASCII_H
#define ASCII_H

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

#include "Ascii_Strings.h"

/* Versioning information */

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define BUG_VERSION 0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 1

/* Parameter defaults */

enum
{
	ASCII_INPUT = 0,
	ASCII_COLOR,
	ASCII_EDGE,
	ASCII_TRANSPARENT,
	ASCII_KERNEL_SIZE,
	ASCII_SIGMA,
	ASCII_SIGMA_SCALE,
	ASCII_TAU,
	ASCII_TRESHOLD,
	ASCII_NUM_PARAMS
};

enum
{
	COLOR_DISK_ID = 1,
	EDGE_DISK_ID,
	TRANSPARENT_DISK_ID,
	KERNEL_SIZE_DISK_ID,
	SIGMA_DISK_ID,
	SIGMA_SCALE_DISK_ID,
	TAU_DISK_ID,
	TRESHOLD_DISK_ID
};

typedef struct MainPass
{
	PF_EffectWorld* source;
	PF_EffectWorld* sobel;
	bool color;
	bool transparent;
	int edge_threshold;
};

typedef struct BlurPass1
{
	PF_EffectWorld* source;
	float sigma;
	float sigma_scale;
	int kernel_size;
};

typedef struct BlurPass2
{
	PF_EffectWorld* source;
	float sigma;
	float sigma_scale;
	int kernel_size;
	float tau;
	float treshold;
};

typedef struct Sobel
{
	PF_EffectWorld* source;
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

const bool ascci0[8][8] = { {false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false}, 
							{false, false, false, false, false, false, false, false}, 
							{false, false, false, false, false, false, false, false}, 
							{false, false, false, false, false, false, false, false}, 
							{false, false, false, false, false, false, false, false}, 
							{false, false, false, false, false, false, false, false}, 
							{false, false, false, false, false, false, false, false}};

const bool ascci1[8][8] = { {false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, false, false, false, false, false} };

const bool ascci2[8][8] = { {false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false} };

const bool ascci3[8][8] = { {false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false} };

const bool ascci4[8][8] = { {false, false, false, false, false, false, false, false},
							{false, false, false, true, true, false, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, true, false, false, false, false, false},
							{false, false, true, false, false, false, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, false, true, true, false, false, false},
							{false, false, false, false, false, false, false, false} };

const bool ascci5[8][8] = { {false, false, false, false, false, false, false, false},
							{false, false, false, true, true, false, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, false, true, true, false, false, false},
							{false, false, false, false, false, false, false, false} };

const bool ascci6[8][8] = { {false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, false, false, false, false, false} };

const bool ascci7[8][8] = { {false, false, true, true, true, false, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, true, true, true, false, false, false},
							{false, false, true, false, false, false, false, false},
							{false, false, true, false, false, false, false, false},
							{false, false, false, false, false, false, false, false} };

const bool ascci8[8][8] = { {false, false, true, true, true, false, false, false},
							{false, true, false, false, false, true, false, false},
							{false, true, false, false, false, true, false, false},
							{false, true, false, false, false, true, false, false},
							{false, true, false, false, false, true, false, false},
							{false, true, false, false, false, true, false, false},
							{false, false, true, true, true, false, false, false},
							{false, false, false, false, false, false, false, false} };

const bool ascci9[8][8] = { {false, false, true, false, false, true, false, false},
							{false, true, true, true, true, true, true, false},
							{false, false, true, false, false, true, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, true, false, false, true, false, false},
							{false, false, true, false, false, true, false, false},
							{false, true, true, true, true, true, true, false},
							{false, false, true, false, false, true, false, false} };

const bool ascci10[8][8] = { {false, false, false, false, false, false, false, false},
							{false, false, true, true, true, true, false, false},
							{false, true, false, false, false, false, true, false},
							{false, true, false, true, true, false, true, false},
							{false, true, false, true, true, false, true, false},
							{false, true, false, true, true, true, false, false},
							{false, true, false, false, false, false, false, false},
							{false, false, true, true, true, true, true, false} };

const bool ascci11[8][8] = { {false, false, false, false, false, false, false, false},
							{false, true, true, true, true, true, true, false},
							{false, true, true, true, true, true, true, false},
							{false, true, true, true, true, true, true, false},
							{false, true, true, true, true, true, true, false},
							{false, true, true, true, true, true, true, false},
							{false, true, true, true, true, true, true, false},
							{false, false, false, false, false, false, false, false} };

const bool ascci12[8][8] = { {false, false, false, false, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, false, false, false, false, false} };

const bool ascci13[8][8] = { {false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, true, true, true, true, true, true, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false} };

const bool ascci14[8][8] = { {false, false, false, false, false, false, false, false},
							{false, false, false, false, false, false, true, false},
							{false, false, false, false, false, true, false, false},
							{false, false, false, false, true, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, true, false, false, false, false, false},
							{false, true, false, false, false, false, false, false},
							{false, false, false, false, false, false, false, false} };

const bool ascci15[8][8] = { {false, false, false, false, false, false, false, false},
							{false, true, false, false, false, false, false, false},
							{false, false, true, false, false, false, false, false},
							{false, false, false, true, false, false, false, false},
							{false, false, false, false, true, false, false, false},
							{false, false, false, false, false, true, false, false},
							{false, false, false, false, false, false, true, false},
							{false, false, false, false, false, false, false, false} };

#endif // ASCII_H