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

#include "Kuwahara.h"

typedef struct {
	A_u_long	index;
	A_char		str[256];
} TableString;



TableString		g_strs[StrID_NUMTYPES] = {
	StrID_NONE,						"",
	StrID_Name,						"Anisotropic Kuwahara",
	StrID_Description,				"Implementation of the Anisotropic Kuwahara filter.\rImplemented by Kirschzelle\rAdapted from https://github.com/GarrettGunnell/Post-Processing/blob/main/Assets/Kuwahara%20Filter/AnisotropicKuwahara.cs\rUse passes < 1 to output preSteps\rONLY WORKS WITH 32BIT",
	StrID_KERNEL_SIZE_Param_Name, "Kernel Size",
	StrID_SHARPNESS_Param_Name, "Sharpness",
	StrID_HARDNESS_Param_Name, "Hardness",
	StrID_ALPHA_Param_Name, "Alpha",
	StrID_ZERO_CROSSING_Param_Name, "Zero Crossing",
	StrID_ZETA_Param_Name, "Use Zeta",
	StrID_ZETA_VALUE_Param_Name, "Zeta",
	StrID_PASSES_Param_Name, "Passes",
	StrID_WILD_Param_Name, "Wild (Change at own risk)",
	StrID_N_Param_Name, "N"
};


char	*GetStringPtr(int strNum)
{
	return g_strs[strNum].str;
}
	