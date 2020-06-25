// zAudioMixer.cpp : 定义静态库的函数。
//

#include "framework.h"
#include "zAudioMixer.h"

static const int DLT_MIN = -10;
static const int DLT_MAX = 10;

void mixPcms(const std::vector<short*>& pcms, int samples, const std::vector<double>& volumeFactors, short* output, double masterVolumeFactor)
{
#if 0
	// By pass, just use the first one
	memcpy(output, pcms.front(), samples * 2);
#else
	for (int i = 0; i < samples; i++)
	{
		double mixTotal = 0;
		for (size_t j = 0; j < pcms.size(); j++)
		{
			// Simple mix it with volume adjust
			short curVal = pcms[j][i];
			double d = curVal / 32767.0f;
			d = d * volumeFactors[j];
			mixTotal += d;

			// Relationship between t(treshold) and a(alpha) are pre-calculated as a mapping table, here we take t=0.6
			// treshold: 0     0.1   0.2   0.3   0.4   0.5   0.6   0.7    0.8    0.9
			// alpha:    2.51  2.84  3.26  3.82  4.59  5.71  7.48  10.63  17.51  41.15
			double out = 0;
			double t = 0.9;
			double a = 41.15;
			double x = mixTotal;
			int n = pcms.size();

			if (pcms.size() == 1 || (x >= -t && x <= t))
			{
				out = x;
			}
			else if (curVal >= DLT_MIN && curVal <= DLT_MAX)
			{
				out = x;
			}
			else
			{
				out = (x / fabs(x)) * (t + (1 - t) * log(1 + a * ((fabs(x) - t) / (n - t))) / log(1 + a));
			}

			// Use adjusted value instead, avoid the overfolow
			mixTotal = (double)out;

			// Adjust master volume
			mixTotal = mixTotal * masterVolumeFactor;
		}

		// Convert to short
		int m = (short)(mixTotal * 32767);
		if (m > 32767)
		{
			m = 32767;
		}
		else if (m < -32767)
		{
			m = -32767;
		}

		// Assign to output
		output[i] = m;
	}
#endif
}
