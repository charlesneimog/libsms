/* 
 * Copyright (c) 2008 MUSIC TECHNOLOGY GROUP (MTG)
 *                         UNIVERSITAT POMPEU FABRA 
 * 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */
/*! \file tables.c
 * \brief sin and sinc tables.
 * 
 * contains functions for creating and indexing the tables
 */
#include "sms.h"

/*! \brief value to scale the sine-table-lookup phase */
static float fSineScale;
/*! \brief value to scale the sinc-table-lookup phase */
static float fSincScale;
/*! \brief global pointer to the sine table */
float *sms_tab_sine;
/*! \brief global pointer to the sinc table */
float *sms_tab_sinc;

/*! \brief prepares the sine table
 * \param  nTableSize    size of table
 * \return error code \see SMS_MALLOC in SMS_ERRORS
 */
int sms_prepSine (int nTableSize)
{
  register int i;
  float fTheta;
  
  if((sms_tab_sine = (float *)malloc(nTableSize*sizeof(float))) == 0)
    return (SMS_MALLOC);
  fSineScale =  (float)(TWO_PI) / (float)(nTableSize - 1);
  fTheta = 0.0;
  for(i = 0; i < nTableSize; i++) 
  {
    fTheta = fSineScale * (float)i;
    sms_tab_sine[i] = sin(fTheta);
  }
  return (SMS_OK);
}
/*! \brief clear sine table */
void sms_clearSine()
{
  if(sms_tab_sine)
    free(sms_tab_sine);
  sms_tab_sine = 0;
}

/*! \brief table-lookup sine method
 * \param fTheta    angle in radians
 * \return approximately sin(fTheta)
 */
float sms_sine (float fTheta)
{
  float fSign = 1.0, fT;
  int i;
  
  fTheta = fTheta - floor(fTheta / TWO_PI) * TWO_PI;
  
  if(fTheta < 0)
  {
    fSign = -1;
    fTheta = -fTheta;
  }
  
  i = fTheta / fSineScale + .5;
  fT = sms_tab_sine[i];
  
  if (fSign == 1)
    return(fT);
  else
    return(-fT);
}

/*! \brief Sinc method to generate the lookup table
 */
static float Sinc (float x, float N)	
{
	return (sinf ((N/2) * x) / sinf (x/2));
}

/*! \brief prepare the Sinc table
 *
 * used for the main lobe of a frequency domain 
 * BlackmanHarris92 window
 *
 * \param  nTableSize    size of table
 * \return error code \see SMS_MALLOC in SMS_ERRORS
 */
int sms_prepSinc (int nTableSize)
{
        int i, m;
	float N = 512.0;
	float fA[4] = {.35875, .48829, .14128, .01168},
		fMax = 0;
	float fTheta = -4.0 * TWO_PI / N, 
	       fThetaIncr = (8.0 * TWO_PI / N) / (nTableSize);

	if((sms_tab_sinc = (float *) calloc (nTableSize, sizeof(float))) == 0)
		return (SMS_MALLOC);

	for(i = 0; i < nTableSize; i++) 
	{
		for (m = 0; m < 4; m++)
			sms_tab_sinc[i] +=  -1 * (fA[m]/2) * 
				(Sinc (fTheta - m * TWO_PI/N, N) + 
			     Sinc (fTheta + m * TWO_PI/N, N));
		fTheta += fThetaIncr;
	}
	fMax = sms_tab_sinc[(int) nTableSize / 2];
	for (i = 0; i < nTableSize; i++) 
		sms_tab_sinc[i] = sms_tab_sinc[i] / fMax;

        fSincScale = (float) nTableSize / 8.0;

	return (SMS_ERRORS);
}
/*! \brief clear sine table */
void sms_clearSinc()
{
  if(sms_tab_sinc)
    free(sms_tab_sinc);
  sms_tab_sinc = 0;
}

/*! \brief global sinc table-lookup method
 * 
 * fTheta has to be from 0 to 8
 *
 * \param fTheta    angle in radians
 * \return approximately sinc(fTheta)
 */
float sms_sinc (float fTheta)
{
	int index = (int) (.5 + fSincScale * fTheta);

	return (sms_tab_sinc[index]);
}
