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

#include "PixelSorter.h"

typedef struct
{
	A_u_long index;
	A_char str[256];
} TableString;

TableString g_strs[StrID_NUMTYPES] = {
	StrID_NONE, "",
	StrID_Name, "PixelSorter",
	StrID_Description, "Implementation of a PixelSorter.\rImplemented by Kirschzelle\rONLY WORKS WITH 32BIT",
	StrID_MASK_MIN_THRESHOLD_Param_Name , "Lower luminance threshold (mask)",
	StrID_MASK_MAX_THRESHOLD_Param_Name , "Upper luminance threshold (mask)",
	StrID_HORIZONTAL_Param_Name			, "Use Horizontal sorting?",
	StrID_SORT_TYPE_Param_Name ,	"0=Mask,r,g,b,a,h,s,l,random=8"};

char *GetStringPtr(int strNum)
{
	return g_strs[strNum].str;
}
