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
	PixelSorter.h
*/

#pragma once

#ifndef PIXELSORTER_H
#define PIXELSORTER_H

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

#include "PixelSorter_Strings.h"

/* Versioning information */

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define BUG_VERSION 0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 1

/* Parameter defaults */

enum
{
	PIXELSORTER_INPUT = 0,
	PIXELSORTER_MASK_MIN_THRESHOLD,
	PIXELSORTER_MASK_MAX_THRESHOLD,
	PIXELSORTER_HORIZONTAL,
	PIXELSORTER_SORT_TYPE,
	PIXELSORTER_NUM_PARAMS
};

enum
{
	MASK_MIN_THRESHOLD_DISK_ID = 1,
	MASK_MAX_THRESHOLD_DISK_ID,
	HORIZONTAL_DISK_ID,
	SORT_TYPE_DISK_ID
};

typedef struct MaskGeneration
{
	float lower_threshold;
	float higher_threshold;
};

typedef struct VerticalImagePreperation
{
	PF_EffectWorld* source;
	PF_EffectWorld* destination;
};

typedef struct MainPass
{
	PF_EffectWorld *source;
	PF_EffectWorld* mask;
	int sort_type;
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

#endif // PIXELSORTER_H