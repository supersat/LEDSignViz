//==============================================================================
#include "SpectrumFFT.h"
//==============================================================================
SpectrumFFT::SpectrumFFT ()
{
    int count;
    for (count = 0; count < FFT_COSTABSIZE; count++)
        costab[count] = (float)FFT_COS(FFT_PI_2 * (float)count / (float)FFT_COSTABSIZE);
}
//==============================================================================
inline const float SpectrumFFT::cosine (float x)
{
    int y;

    x *= FFT_TABLERANGE;
    y = (int)x;
    if (y < 0)
    {
        y = -y; 
    }

    y &= FFT_TABLEMASK;
    switch (y >> FFT_COSTABBITS)
    {
        case 0 : return  costab[y]; 
        case 1 : return -costab[(FFT_COSTABSIZE - 1) - (y - (FFT_COSTABSIZE * 1))];
        case 2 : return -costab[                       (y - (FFT_COSTABSIZE * 2))]; 
        case 3 : return  costab[(FFT_COSTABSIZE - 1) - (y - (FFT_COSTABSIZE * 3))]; 
    }

    return 0.0f;
}
//==============================================================================
inline const float SpectrumFFT::sine (float x)
{
    return cosine(x - 0.25f);
}
//==============================================================================
inline const unsigned int SpectrumFFT::reverse (unsigned int val, int bits)
{
    unsigned int retn = 0;
  
    while (bits--)
    {
        retn <<= 1;
        retn |= (val & 1);
        val >>= 1;
    }
  
    return (retn);
}
//==============================================================================
inline void SpectrumFFT::process (int bits)
{
    register int    count, count2, count3;
    unsigned        i1;	
    int             i2, i3, i4, y;
    int             fftlen = 1 << bits;
    float           a1, a2, b1, b2, z1, z2;
    float           oneoverN= 1.0f / fftlen;

    i1 = fftlen / 2;
    i2 = 1;
    
    for (count = 0; count < bits; count++)
    {
        i3 = 0;
        i4 = i1;

        for (count2 = 0; count2 < i2; count2++)
	    {
	        y  = reverse(i3 / (int)i1, bits);

            z1 =  cosine((float)y * oneoverN);
            z2 =   -sine((float)y * oneoverN);

	        for (count3 = i3; count3 < i4; count3++)
	        {
	            a1 = buffer[count3].re;
	            a2 = buffer[count3].im;

	            b1 = (z1 * buffer[count3+i1].re) - (z2 * buffer[count3+i1].im);
	            b2 = (z2 * buffer[count3+i1].re) + (z1 * buffer[count3+i1].im);

	            buffer[count3].re = a1 + b1;
	            buffer[count3].im = a2 + b2;

	            buffer[count3+i1].re = a1 - b1;
	            buffer[count3+i1].im = a2 - b2;
	        }

	        i3 += (i1 << 1);
	        i4 += (i1 << 1);
	    }

        i1 >>= 1;
        i2 <<= 1;
    }
}
//==============================================================================
void SpectrumFFT::getSpectrum (float *pcmbuffer,
							   unsigned int pcmposition,
							   unsigned int pcmlength,
							   float *spectrum,
							   int length,
							   int channel,
							   int numchannels,
							   FFT_WINDOW windowtype)
{
	int count, bits, bitslength, nyquist;

	bitslength = length;
	bits = 0;
	while (bitslength > 1)
	{
		bitslength >>= 1;
		bits++;
	}

	switch (windowtype)
    {
		//==================================================================
        case FFT_WINDOW_TRIANGLE:
        {
            for (count = 0; count < length; count++)
            {
                float window;
                float percent = (float)count / (float)length;

                window = (percent * 2.0f) - 1.0f; 
                window = FFT_ABS(window);         
                window = 1.0f - window;           

                buffer[count].re = pcmbuffer[(pcmposition * numchannels) + channel] * window;
                buffer[count].re /= (float)length;
                buffer[count].im = 0.00000001f; 

                pcmposition++;
                if (pcmposition >= pcmlength)
                {
                    pcmposition = 0;
                }
            }
            break;
        }
		//==================================================================
        case FFT_WINDOW_HAMMING:
        {
            for (count = 0; count < length; count++)
            {
                float window;
                float percent = (float)count / (float)length;

                window = 0.54f - (0.46f * cosine(percent) );

                buffer[count].re = pcmbuffer[(pcmposition * numchannels) + channel] * window;
                buffer[count].re /= (float)length;
                buffer[count].im = 0.00000001f; 

                pcmposition++;
                if (pcmposition >= pcmlength)
                {
                    pcmposition = 0;
                }
            }
            break;
        }
		//==================================================================
        case FFT_WINDOW_HANNING:
        {
            for (count = 0; count < length; count++)
            {
                float window;
                float percent = (float)count / (float)length;

                window = 0.5f *  (1.0f  - cosine(percent) );

                buffer[count].re = pcmbuffer[(pcmposition * numchannels) + channel] * window;
                buffer[count].re /= (float)length;
                buffer[count].im = 0.00000001f; 

                pcmposition++;
                if (pcmposition >= pcmlength)
                {
                    pcmposition = 0;
                }
            }
            break;
        }
		//==================================================================
        case FFT_WINDOW_BLACKMAN:
        {
            for (count = 0; count < length; count++)
            {
                float window;
                float percent = (float)count / (float)length;

                window = 0.42f - (0.5f  * cosine(percent) ) + (0.08f * cosine(2.0f * percent) );

                buffer[count].re = pcmbuffer[(pcmposition * numchannels) + channel] * window;
                buffer[count].re /= (float)length;
                buffer[count].im = 0.00000001f;

                pcmposition++;
                if (pcmposition >= pcmlength)
                {
                    pcmposition = 0;
                }
            }
            break;
        }
		//==================================================================
        case FFT_WINDOW_BLACKMANHARRIS:
        {
            float a0 = 0.35875f;
            float a1 = 0.48829f;
            float a2 = 0.14128f;
            float a3 = 0.01168f;

            for (count = 0; count < length; count++)
            {
                float window;
                float percent = (float)count / (float)length;
 
                window = a0 - (a1 * cosine(1.0f * percent) ) +
                              (a2 * cosine(2.0f * percent) ) -
                              (a3 * cosine(3.0f * percent) );

                buffer[count].re = pcmbuffer[(pcmposition * numchannels) + channel] * window;
                buffer[count].re /= (float)length;
                buffer[count].im = 0.00000001f; 

                pcmposition++;
                if (pcmposition >= pcmlength)
                {
                    pcmposition = 0;
                }
            }
            break;
        }
		//==================================================================
        case FFT_WINDOW_RECT:
        default:
        {
            for (count = 0; count < length; count++)
            {
                buffer[count].re = pcmbuffer[(pcmposition * numchannels) + channel];
                buffer[count].re /= (float)length;
                buffer[count].im = 0.00000001f;  /* Giving it a very small number besides 0 avoids domain exceptions = big cpu usage */

                pcmposition++;
                if (pcmposition >= pcmlength)
                {
                    pcmposition = 0;
                }
            }
            break;
        }
 		//==================================================================
   };

    process(bits);

    nyquist = length / 2;
    for (count=0; count < nyquist-1; count++)
	{
        float magnitude;
        int n = count;

        n = reverse(n, bits);

        magnitude = FFT_SQRT((buffer[n].re * 
							  buffer[n].re) 
						   + (buffer[n].im * 
							  buffer[n].im));

        magnitude *= 2.5f; 

		if (magnitude > 1.0f)
            magnitude = 1.0f;

        spectrum[count] = magnitude;
    }
}
//==============================================================================