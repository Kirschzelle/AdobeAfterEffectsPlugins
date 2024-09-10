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
	Kuwahara.h
*/

#pragma once

#ifndef KUWAHARA_H
#define KUWAHARA_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;
#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels; 
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

#include "Kuwahara_Strings.h"

/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	1
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1


/* Parameter defaults */

enum {
	KUWAHARA_INPUT = 0,
	KUWAHARA_KERNEL_SIZE,
	KUWAHARA_SHARPNESS,
	KUWAHARA_HARDNESS,
	KUWAHARA_ALPHA,
	KUWAHARA_ZERO_CROSSING,
	KUWAHARA_ZETA,
	KUWAHARA_ZETA_VALUE,
	KUWAHARA_PASSES,
	KUWAHARA_WILD,
	KUWAHARA_N,
	KUWAHARA_NUM_PARAMS
};

enum {
	KERNEL_SIZE_DISK_ID = 1,
	SHARPNESS_DISK_ID = 2,
	HARDNESS_DISK_ID = 3,
	ALPHA_DISK_ID = 4,
	ZERO_CROSSING_DISK_ID = 5,
	ZETA_DISK_ID = 6,
	ZETA_VALUE_DISK_ID = 7,
	PASSES_DISK_ID = 8,
	WILD_DISK_ID = 9,
	N_DISK_ID = 10
};

typedef struct SourceInfo{
	PF_EffectWorld*	source;
	bool wild;
} SourceInfo, * SourceInfoP, ** SourceInfoH;

typedef struct MainPass {
	PF_EffectWorld* source;
	int kernel_size;
	float sharpness;
	float hardnesss;
	float alpha;
	float zero_crossing;
	float zeta;
	int n;
	bool wild;
};


extern "C" {

	DllExport
	PF_Err
	EffectMain(
		PF_Cmd			cmd,
		PF_InData		*in_data,
		PF_OutData		*out_data,
		PF_ParamDef		*params[],
		PF_LayerDef		*output,
		void			*extra);

}

#endif // KUWAHARA_H