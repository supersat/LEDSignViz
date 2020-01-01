// ============================================================================
#ifndef __SPECTRUM_FFT_H__
#define __SPECTRUM_FFT_H__
// ============================================================================
#include "../JuceLibraryCode/JuceHeader.h"
//=============================================================================
/**
	This FFT code is based on a source code i've found on the web.
*/
//=============================================================================
const int FFT_COSTABBITS = 13;
const int FFT_COSTABSIZE = (1 << FFT_COSTABBITS);
const int FFT_TABLERANGE = (FFT_COSTABSIZE * 4);
const int FFT_TABLEMASK  = (FFT_TABLERANGE - 1);
//=============================================================================
typedef struct
{
    float re;
    float im;
} 
FFT_COMPLEX;
//=============================================================================
#define FFT_ABS(x)    ((x) < 0  ? -(x) : (x))
#define FFT_MAX(a, b) ((a) > (b) ? (a) : (b))
#define FFT_MIN(a, b) ((a) < (b) ? (a) : (b))
//=============================================================================
#define FFT_PI     3.14159265358979323846f
#define FFT_PI2   (3.14159265358979323846f * 2.0f)
#define FFT_PI_2  (3.14159265358979323846f / 2.0f)
#define FFT_SQRT2  1.41421356237309504880f
#define FFT_LOG2   0.693147180559945309417f
//=============================================================================
#ifdef JUCE_WIN32
    #include <math.h>

    #define FFT_SIN   (float)sin
    #define FFT_SINH  (float)sinh
    #define FFT_COS   (float)cos
    #define FFT_TAN   (float)tan
    #define FFT_SQRT  (float)sqrt
    #define FFT_POW   (float)pow
    #define FFT_ATAN  (float)atan
    #define FFT_EXP   (float)exp
    #define FFT_LDEXP (float)ldexp
    #define FFT_FABS  fabsf
    #define FFT_LOG   (float)log
    #define FFT_LOG10 (float)log10
    #define FFT_FMOD  (float)fmod

#elif JUCE_MAC
    #include <math.h>

	#define FFT_SIN   sinf
    #define FFT_SINH  sinhf
    #define FFT_COS   cosf
    #define FFT_TAN   tanf
    #define FFT_SQRT  sqrtf
    #define FFT_POW   powf
    #define FFT_ATAN  atanf
    #define FFT_EXP   expf
    #define FFT_LDEXP ldexpf
    #define FFT_FABS  fabsf
    #define FFT_LOG   logf
    #define FFT_LOG10 log10f
    #define FFT_EXP2(_val) powf(2.0f, _val)
    #define FFT_FMOD  fmodf

#endif
//=============================================================================
class SpectrumFFT
{
	private:
		FFT_COMPLEX		buffer [16 * 1024];
        float			costab [FFT_COSTABSIZE];
		//====================================================================
        inline const float			cosine (float x);
        inline const float			sine (float x);
        inline const unsigned int	reverse (unsigned int val, int bits);
        inline void					process (int bits);
		//====================================================================
	public:
		//====================================================================
		SpectrumFFT ();
		//====================================================================
		typedef enum
		{
			FFT_WINDOW_RECT,            
			FFT_WINDOW_TRIANGLE,        
			FFT_WINDOW_HAMMING,         
			FFT_WINDOW_HANNING,         
			FFT_WINDOW_BLACKMAN,        
			FFT_WINDOW_BLACKMANHARRIS,  
		    
			FFT_WINDOW_MAX,             
			FFT_WINDOW_FORCEINT = 65536 
		} 
		FFT_WINDOW;
		//====================================================================
		void getSpectrum (float *pcmbuffer,
						  unsigned int pcmposition,
						  unsigned int pcmlength,
						  float *spectrum,
						  int length,
						  int channel,
						  int numchannels,
						  FFT_WINDOW windowtype);
		//====================================================================
};
//=============================================================================
#endif // __SPECTRUM_FFT_H__
//=============================================================================