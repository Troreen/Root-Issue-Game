#pragma once
#include <tge/math/vector2.h>

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace Tga
{
	class Audio
	{
	public:
		Audio();
		~Audio();
		void Init(const char* aPath, bool aIsLooping = false);
		void Play();
		void SetVolume(float aVolume);
		void SetPosition(Vector2f aPosition);

		// Stops playing the sample in two ways - by immediately and by
		// set loop flag to 0, therefore next loop is not come
		void Stop(bool aImmediately = true);

	private:
		static class AudioOut* myAudioOut;
		long myLoadedAudio;
		DWORD myChannel;
		Vector2f myPosition;
		float myVolume;
	};
}