/* 
 * Copyright (c) 2009 John Glover, National University of Ireland, Maynooth
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

/*! \file modify.c
 * \brief modify sms data 
 */

#include "sms.h"

/*! \brief linear interpolation between 2 spectral envelopes.
 *
 * The values in env2 are overwritten by the new interpolated envelope values.
 */
void sms_interpEnvelopes(int sizeEnv, sfloat *env1, sfloat *env2, float interpFactor)
{
        if(sizeEnv <= 0)
        {       
                return;
        }
        
        int i;
        float amp1, amp2;
        
        for(i = 0; i < sizeEnv; i++)
        {
                amp1 = env1[i];
                amp2 = env2[i];
                if(amp1 <= 0) amp1 = amp2;
                if(amp2 <= 0) amp2 = amp1;
                env2[i] = amp1 + (interpFactor * (amp2 - amp1));
        }
}

/*! \brief apply the spectral envelope of 1 sound to another 
 *
 * Changes the amplitude of spectral peaks in a target sound (pFreqs, pMags) to match those
 * in the envelope (pCepEnvFreqs, pCepEnvMags) of another, up to a maximum frequency of maxFreq.
 */
void sms_applyEnvelope(int numPeaks, sfloat *pFreqs, sfloat *pMags, int sizeEnv, sfloat *pEnvMags, int maxFreq)
{
	if(sizeEnv <= 0 || maxFreq <= 0)
        {
                return;
        }

	int i, envPos;
        float frac, binSize = (float)maxFreq / (float)sizeEnv;
        
        for(i = 0; i < numPeaks; i++)
	{
                /* convert peak freqs into bin positions for quicker envelope lookup */
                pFreqs[i] /= binSize;

		/* if current peak is within envelope range, set its mag to the envelope mag */
		if(pFreqs[i] <= maxFreq && pFreqs[i] > 0)
		{
                        envPos = (int)pFreqs[i];
                        frac = pFreqs[i] - envPos;
                        if(envPos < sizeEnv - 1)
                        {
			        pMags[i] = ((1.0 - frac) * pEnvMags[envPos]) + (frac * pEnvMags[envPos+1]);
                        }
                        else
                        {       printf("should this ever happen?\n");
                                pMags[i] = pEnvMags[sizeEnv-1];
                        }
                }
		else
		{
			pMags[i] = 0;
		}
        
                /* convert back to frequency values */
                pFreqs[i] *= binSize;
	}

}

/*! \brief basic transposition
 *
 * Multiply the frequencies of the deterministic component by a constant
 */
void sms_transpose(SMS_Data *frame, sfloat transpositionFactor)
{
}

/*! \brief transposition maintaining spectral envelope
 *
 * Multiply the frequencies of the deterministic component by a constant, then change
 * their amplitudes so that the original spectral envelope is maintained
 */
void sms_transposeKeepEnv(SMS_Data *frame)
{
}

/*! \brief modify a frame (SMS_Data object) 
 *
 * Performs a modification on a SMS_Data object. The type of modification and any additional
 * parameters are specified in the given SMS_ModifyParams structure.
 */
void sms_modify(SMS_Data *frame, SMS_ModifyParams *params)
{
        switch(params->modifyType)
        {
                case SMS_MTYPE_TRANSPOSE:
                        sms_transpose(frame, params->transposition);
                        break;

                case SMS_MTYPE_TRANSPOSE_KEEP_ENV:
                        printf("transpose keep env\n");
                        break;

                case SMS_MTYPE_USE_ENV:
                        sms_applyEnvelope(frame->nTracks, frame->pFSinFreq, frame->pFSinAmp, params->sizeEnv, params->env, params->maxFreq);
                        break;

                case SMS_MTYPE_INTERP_ENV:
                        sms_interpEnvelopes(params->sizeEnv, frame->pSpecEnv, params->env, params->envInterp);
                        break;
        }
}
