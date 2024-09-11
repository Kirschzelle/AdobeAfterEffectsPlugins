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

#include "Ascii.h"

typedef struct
{
	A_u_long index;
	A_char str[256];
} TableString;

TableString g_strs[StrID_NUMTYPES] = {
	StrID_NONE, "",
	StrID_Name, "Ascii",
	StrID_Description, "Implementation of an Ascii render.\rImplemented by Kirschzelle\rONLY WORKS WITH 32BIT\rPartly adobted from: https://github.com/GarrettGunnell/Post-Processing/blob/main/Assets/ASCII/ASCII.shader",
	StrID_COLOR_Param_Name, "Output Color?",
	StrID_EDGE_Param_Name, "Edge Treshold",
	StrID_TRANSPARENT_Param_Name, "Transparent?",
	StrID_KERNEL_SIZE_Param_Name, "Kernel Size" ,
	StrID_SIGMA_Param_Name, "Sigma" ,
	StrID_SIGMA_SCALE_Param_Name, "Sigma Scale" ,
	StrID_TAU_Param_Name, "Tau" ,
	StrID_TRESHOLD_Param_Name, "Treshold"};

char *GetStringPtr(int strNum)
{
	return g_strs[strNum].str;
}
