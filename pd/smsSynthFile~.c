#include "m_pd.h"
#include "sms.h"
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

/* ------------------------ smsSynthFile~ ----------------------------- */


#define SOURCE_FLOAT 1
#define SOURCE_SIGNAL 2

// helper functions



//method for opening file in canvas directory.
//Based on zexy's [msgfile], which is based on
//Pd's [textfile]
t_symbol* getFullPathName( t_symbol *infilename,  t_canvas *smsCanvas)
{
    char nameout[MAXPDSTRING], namebuf[MAXPDSTRING];
    char dirresult[MAXPDSTRING], *nameresult;
    char *dirname;
    char *sym = infilename->s_name;    
    dirname = canvas_getdir(smsCanvas)->s_name;

    if((open_via_path(dirname, sym,"", dirresult, &nameresult, MAXPDSTRING, 0)) < 0) 
            return(NULL);

    namebuf[0] = 0;
    if (*dirresult) //append directory
    {
            strcat(namebuf, dirresult);
            strcat(namebuf, "/");
    }
    strcat(namebuf, nameresult); //append name
    sys_bashfilename(namebuf, nameout);    // cross-platformize slashes
    
    if (0)
    {
            post("(open_via_path):");
            post("dirname: %s", dirname);
            post("filename->s_name: %s", sym);
            post("dirresult: %s", dirresult);
            post("nameresult: %s", nameresult);
            post("namebuf: %s", namebuf);
            post("nameout: %s ", nameout);
    }
    

    return(gensym( nameout ));
} 

static t_class *smsSynthFile_class;

typedef struct _smsSynthFile
{
        t_object x_obj; 
        t_canvas *canvas;
        t_symbol *s_filename;
        t_int i_frame, i_frameSource, synthBufPos;
        t_float *synthBuf;
        t_float f;
	FILE *pSmsFile; 
        SMS_SynthParams synthParams;
        SMS_Header *pSmsHeader;
        SMS_Data smsFrameL, smsFrameR, newSmsFrame;
} t_smsSynthFile;

static t_int *smsSynthFile_perform(t_int *w)
{
        t_smsSynthFile *x = (t_smsSynthFile *)(w[1]);
        t_sample *in = (t_float *)(w[2]);
        t_sample *out = (t_float *)(w[3]);
        int n = (int)(w[4]);
        float f;
        int i, iLeftFrame, iRightFrame;

        if(x->pSmsHeader != NULL)        
        {
                if(x->synthBufPos >= x->synthParams.sizeHop)
                {
                        if(x->f >= x->pSmsHeader->nFrames)
                                x->f = x->pSmsHeader->nFrames -1;
                        if(x->f < 0) x->f = 0;
                
                        iLeftFrame = MIN (x->pSmsHeader->nFrames - 1, floor (x->f)); 
                        iRightFrame = (iLeftFrame < x->pSmsHeader->nFrames - 2)
                                ? (1+ iLeftFrame) : iLeftFrame;
                
                        sms_getFrame (x->pSmsFile, x->pSmsHeader, iLeftFrame, &x->smsFrameL);
                        sms_getFrame (x->pSmsFile, x->pSmsHeader, iRightFrame,&x->smsFrameR);
                        sms_interpolateFrames (&x->smsFrameL, &x->smsFrameR, &x->newSmsFrame,
                                               x->f - iLeftFrame);
                
                        sms_synthesize (&x->newSmsFrame, x->synthBuf, &x->synthParams);
                        x->synthBufPos = 0;
                }
                //check when blocksize is larger than hopsize... will probably crash
                for (i = 0; i < n; i++, x->synthBufPos++)
                        out[i] = x->synthBuf[x->synthBufPos];
        }
        else  while(n--) *out++ = 0;

        return (w+5);
}
static void smsSynthFile_sizehop(t_smsSynthFile *x, t_float *fSizeHop)
{
        //what is minimum hopsize?
        post("TODO: set sizeHop and re-init");
}

static void smsSynthFile_source(t_smsSynthFile *x, t_float f_source)
{
        if(f_source > .0001)
        {
                x->i_frameSource = SOURCE_SIGNAL;
                post("smsSynthFile_source: frame source set to signal");
        }
        else
        {
                x->i_frameSource = SOURCE_FLOAT;
                post("smsSynthFile_source: frame source set to float");
        }
}

/* open function:
 * 1. get fullname in a system independant manner
 * 2. read file header
 * 3. initialize the synthesizer based on header/synth params
 * 4. because we are synthesizing from file, allocate space for sms frames
 */
static void smsSynthFile_open(t_smsSynthFile *x, t_symbol *filename)
{
        long iError;
        t_symbol *fullname;

        x->s_filename = gensym(filename->s_name);
        fullname = getFullPathName(filename, x->canvas);
        if(fullname == NULL)
        {
                pd_error(x, "smsSynthFile_open: cannot find file: %s", filename->s_name);
                return;
        }
 
        //check if a file has been opened, close and init if necessary
        if(x->pSmsHeader != NULL)
        {
                post("smsSynthFile_open: re-initializing");
                sms_freeSynth(&x->synthParams);                
                sms_freeFrame(&x->smsFrameL);
                sms_freeFrame(&x->smsFrameR);
                sms_freeFrame(&x->newSmsFrame);
        }
        
        if ((iError = sms_getHeader (x->s_filename->s_name, &x->pSmsHeader, &x->pSmsFile)) < 0)
	{
                pd_error(x, "smsSynthFile_open: %s", sms_errorString(iError));
                return;
        }

        sms_initSynth( x->pSmsHeader, &x->synthParams );
        
	/* setup for synthesis from file */
        /* needs 3 frame buffers: left, right, and interpolated */
	sms_allocFrameH (x->pSmsHeader, &x->smsFrameL);
	sms_allocFrameH (x->pSmsHeader, &x->smsFrameR);
        // I guess I am always ignoring phase information for now..
	sms_allocFrame (&x->newSmsFrame, x->pSmsHeader->nTracks, 
	                   x->pSmsHeader->nStochasticCoeff, 0,
                         x->pSmsHeader->iStochasticType);

        post("sms file initialized: %s ", filename->s_name );

}

static void smsSynthFile_info(t_smsSynthFile *x)
{

        post("sms file : %s ", x->s_filename->s_name );
        post("__arguments__");
        post("samplingrate: %d  ", x->synthParams.iSamplingRate);
/*         if(x->synthParams.iSynthesisType == STYPE_ALL)  */
/*                 post("synthesis type: all "); */
/*         else if(x->synthParams.iSynthesisType == STYPE_DET)  */
/*                 post("synthesis type: deterministic only "); */
/*         else if(x->synthParams.iSynthesisType == STYPE_STOC)  */
/*                 post("synthesis type: stochastic only "); */
/*         if(x->synthParams.iDetSynthType == DET_IFFT)  */
/*                 post("deteministic synthesis method: ifft "); */
/*         else if(x->synthParams.iDetSynthType == DET_OSC)  */
/*                 post("deteministic synthesis method: oscillator bank "); */
        post("sizeHop: %d ", x->synthParams.sizeHop);
        post("__header info__");
        post("fOriginalSRate: %d, iFrameRate: %d, origSizeHop: %d",
             x->pSmsHeader->iOriginalSRate, x->pSmsHeader->iFrameRate, x->synthParams.origSizeHop);
        post("original file length: %f seconds ", (float)  x->pSmsHeader->nFrames *
             x->synthParams.origSizeHop / x->pSmsHeader->iOriginalSRate );



}

static void smsSynthFile_dsp(t_smsSynthFile *x, t_signal **sp)
{
        x->synthParams.iSamplingRate =  sp[0]->s_sr;//todo: check if changed and recompute
        //need x and 2 vectors (in/out), and lastly the vector size:
        dsp_add(smsSynthFile_perform, 4, x,  sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void *smsSynthFile_new(void)
{
        t_smsSynthFile *x = (t_smsSynthFile *)pd_new(smsSynthFile_class);
        outlet_new(&x->x_obj, gensym("signal"));
        
        x->canvas = canvas_getcurrent();
        x->s_filename = NULL;
        x->i_frameSource = SOURCE_FLOAT;

        x->synthParams.iSynthesisType = SMS_STYPE_ALL;
        x->synthParams.iDetSynthType = SMS_DET_IFFT;
        x->synthParams.sizeHop = x->synthBufPos = 512;

        x->synthParams.iSamplingRate = 44100; //should be updated once audio is turned on
        
        sms_init();
    
        x->synthBuf = (t_float *) calloc(x->synthParams.sizeHop, sizeof(t_float));
        
        return (x);
}

static void smsSynthFile_free(t_smsSynthFile *x)
{
        if(x->pSmsHeader != NULL) 
        {
                sms_freeSynth(&x->synthParams);
                sms_freeFrame(&x->smsFrameL);
                sms_freeFrame(&x->smsFrameR);
                sms_freeFrame(&x->newSmsFrame);
        }
}
void smsSynthFile_tilde_setup(void)
{
        smsSynthFile_class = class_new(gensym("smsSynthFile~"), (t_newmethod)smsSynthFile_new, 
                                       (t_method)smsSynthFile_free, sizeof(t_smsSynthFile), 0, A_DEFFLOAT, 0);
        CLASS_MAINSIGNALIN(smsSynthFile_class, t_smsSynthFile, f);
        class_addmethod(smsSynthFile_class, (t_method)smsSynthFile_dsp, gensym("dsp"), 0);
        class_addmethod(smsSynthFile_class, (t_method)smsSynthFile_open, gensym("open"), A_DEFSYM, 0);
        class_addmethod(smsSynthFile_class, (t_method)smsSynthFile_info, gensym("info"),  0);
        class_addmethod(smsSynthFile_class, (t_method)smsSynthFile_sizehop, gensym("sizeHop"), A_DEFFLOAT, 0);
        class_addmethod(smsSynthFile_class, (t_method)smsSynthFile_source, gensym("source"), A_DEFFLOAT, 0);
        
}
