#pragma once
#include "GodKingFMod/fmod.h"
#include "GodKingFMod/SoundEngine.h"
#include "VfxSystem.h"
#include <tge/script/BaseProperties.h>


enum SoundID 
{
	eVineBoom,
	eMusicLoop,
	eStep,
	eBasicVox,
	eHeavyVox,
	eCharge,
	eShoot,
	eRollBegin,
	eRoll,
	ePlayerAttack,
	eEnemyDeadVox,
	eBasicAttackVox,
	eGore,
	eRootDoor,
	eMainMenuMusic,
	eMusicLevel1,
	eMusicLevel2,
	eMusicLevel3,
	eIntroSFX,
	eOutroSFX,
	eUnknown
};

enum BusID 
{
	eSFX,
	eMusic
};

class AudioManager
{
	public:

		AudioManager();
		~AudioManager();

		AudioManager(const AudioManager&) = delete;
		AudioManager& operator=(const AudioManager&) = delete;

		void Init();
		void Update(const float aDeltaTime);

		void PlaySFX(const SoundID aSoundID);
		void PlayMusic(const SoundID aMusicID, const bool aUninterrupted = false);

		void PlaySFXAtLocation(const SoundID aMusicID, const Tga::Vector3f aPosition);
		void PlaySFXAtLocationWithEffect(const SoundID aMusicID, const Tga::Vector3f aPosition, const Tga::Vector3f aCameraTrans, VfxSystem& aVfxSystem, const std::string aVfxName);

		void StopMusic(const SoundID aMusicID, const bool anImmediately);
		void StopAllEvents();

		const bool IsEventPlaying(const SoundID aMusicID);
		const bool Is3DEventPlaying(const SoundID aMusicID, const Tga::Vector3f aPosition, const Tga::Vector3f aListenerPosition);

		void SetBusVolume(BusID anID, float aPercentage);
		void SetMasterVolume(float aPercentage);
		void SetEventVolume(const SoundID aSoundID, float aPercentage);
		float GetBusVolume(BusID anID);
		float GetMasterVolume();
		float GetEventVolume();

		void SetAudioListener(const Tga::Vector3f aPosition);
		const Tga::Vector3f& GetAudioListenerPosition() const;

		void PlayEffectOnPosition(const SoundID aSoundID, const Tga::Vector3f aPosition);

		void FadeOut(const SoundID anID, const float aFadeSpeed);
		void FadeIn(const SoundID anID, const float aFadeSpeed);

		void ChangePitch(const SoundID anID, const float aPitchChange);
		void ChangeReverb(const SoundID anID, const float aReverbChange);

		void ResetSounds();

	private:

		void RegisterAllEvents();

		std::unordered_map<SoundID, SoundEventInstanceHandle> myMusicList;
		std::unordered_map<BusID, std::string> myBusses;
		SoundEngine::ListenerHandle myListener;

		bool myFadeOutActive;
		bool myFadeInActive;
		float myFadeSpeed;
		float myMusicVolume;
		static inline float myMasterVolume;
		static inline float myBusVolume[2];
		float myEventVolume;
		SoundID myFadeID;

		Tga::Vector3f myListenerPosition;
};

