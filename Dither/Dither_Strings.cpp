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

#include "Dither.h"

typedef struct
{
	A_u_long index;
	A_char str[256];
} TableString;

TableString g_strs[StrID_NUMTYPES] = {
	StrID_NONE, "",
	StrID_Name, "Dither",
	StrID_Description, "Implementation of Bayer Dither.\rImplemented by Kirschzelle\rONLY WORKS WITH 32BIT",
	StrID_COLORS_RED_Param_Name, "Red Channel Color Amount",
	StrID_COLORS_GREEN_Param_Name, "Green Channel Color Amount",
	StrID_COLORS_BLUE_Param_Name, "Blue Channel Color Amount",
	StrID_COLORS_ADAPT_Param_Name, "Adapt Colors to Image?",
	StrID_N_Param_Name, "n"};

char *GetStringPtr(int strNum)
{
	return g_strs[strNum].str;
}
