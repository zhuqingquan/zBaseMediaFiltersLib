#pragma once
#ifndef _Z_AUDIO_MIXER_H_
#define _Z_AUDIO_MIXER_H_

#include <vector>

void mixPcms(const std::vector<short*>& pcms, int samples, const std::vector<double>& volumeFactors, short* output, double masterVolumeFactor);

class AudioMixer
{

};

#endif//_Z_AUDIO_MIXER_H_