#include "AudioManager.h"
#include <tge/settings/settings.cpp>

void AudioManager::Init()
{
	std::string bankRootPath = Tga::Settings::game_assets_path.string() + "/FMODBank/Desktop/";
	SoundEngine::Init(bankRootPath);

	SoundEngine::LoadBank("Master.strings.bank", 0);
	SoundEngine::LoadBank("Master.bank", 0);

	SoundEngine::LoadBus("bus:/SFX", "SFX");
	myBusses.insert({ BusID::eSFX, "SFX" });
	SoundEngine::LoadBus("bus:/MUSIC", "MUSIC");
	myBusses.insert({ BusID::eMusic, "MUSIC" });

	myListener = SoundEngine::GetNextFreeListener();

	myMasterVolume = 1.f;
	myBusVolume[0] = 1.f;
	myBusVolume[1] = 1.f;
	
	SoundEngine::OverrideOtherListeners(myListener);


	RegisterAllEvents();
}

void AudioManager::Update(const float aDeltaTime)
{
	aDeltaTime;
	SoundEngine::Update();

	if (myFadeOutActive)
	{
		myMusicVolume -= myFadeSpeed;
		if (myMusicVolume <= 0.0f)
		{
			SetEventVolume(myFadeID, 0.0f);
			StopMusic(myFadeID, true);
			myFadeOutActive = false;

		}
		SetEventVolume(myFadeID, myMusicVolume);
	}
	else if (myFadeInActive)
	{
		myMusicVolume += myFadeSpeed;
		if (myMusicVolume > 1.0f)
		{
			SetEventVolume(myFadeID, 1.0f);
			PlayMusic(myFadeID, true);
			myFadeInActive = false;
		}
		SetEventVolume(myFadeID, myMusicVolume);
	}
}

void AudioManager::PlaySFX(const SoundID aSoundID)
{
	SoundEngine::PlayEvent(myMusicList[aSoundID]);
}

void AudioManager::PlayMusic(const SoundID aMusicID, const bool aUninterrupted)
{
	if (aUninterrupted)
	{
		if (!IsEventPlaying(aMusicID))
		{
			SoundEngine::PlayEvent(myMusicList[aMusicID]);
		}
	}
	else if (!aUninterrupted)
	{
		SoundEngine::PlayEvent(myMusicList[aMusicID]);
	}
}

void AudioManager::PlaySFXAtLocation(const SoundID aMusicID, const Tga::Vector3f aPosition)
{
	SoundEngine::PlayEventAtLocation(myMusicList[aMusicID], { aPosition.x, aPosition.y, aPosition.z });
}

void AudioManager::PlaySFXAtLocationWithEffect(const SoundID aMusicID, const Tga::Vector3f aPosition, const Tga::Vector3f aCameraTrans, VfxSystem& aVfxSystem, const std::string aVfxName)
{
	float distanceSize = 75.0f;
	std::array<float, 3> TargetPos = { (aPosition.x - aCameraTrans.x)/ distanceSize, (aPosition.y - aCameraTrans.y)/ distanceSize, (aPosition.z - aCameraTrans.z)/ distanceSize };
	SoundEngine::PlayEventAtLocation(myMusicList[aMusicID], TargetPos);
#ifdef _DEBUG
	aVfxSystem.SpawnWorldEffect(aVfxName, aPosition, distanceSize/75.f);
#else
	UNREFERENCED_PARAMETER(aVfxSystem);
	UNREFERENCED_PARAMETER(aVfxName);
#endif //  DEBUG

}

void AudioManager::StopMusic(const SoundID aMusicID, const bool anImmediately)
{
	SoundEngine::StopEvent(myMusicList[aMusicID], anImmediately);
}

void AudioManager::StopAllEvents()
{
	for (const auto& pair : myMusicList)
	{
		if (SoundEngine::IsEventPlaying(myMusicList[pair.first]))
		{
			SoundEngine::StopEvent(myMusicList[pair.first], true);
		}
	}
}

const bool AudioManager::IsEventPlaying(const SoundID aMusicID)
{
	return SoundEngine::IsEventPlaying(myMusicList[aMusicID]);
}

const bool AudioManager::Is3DEventPlaying(const SoundID aMusicID, const Tga::Vector3f aPosition, const Tga::Vector3f aListenerPosition)
{
	SetAudioListener(aPosition - aListenerPosition);
	return SoundEngine::IsEventPlaying(myMusicList[aMusicID]);
}

void AudioManager::SetBusVolume(BusID anID, float aPercentage)
{
	myBusVolume[anID] = aPercentage;
	SoundEngine::SetBusVolume(myBusses[anID], aPercentage);
}

void AudioManager::SetMasterVolume(float aPercentage)
{
	myMasterVolume = aPercentage;
	SoundEngine::SetMasterVolume(aPercentage);
}

void AudioManager::SetEventVolume(const SoundID aSoundID, float aPercentage)
{
	myEventVolume = aPercentage;
	SoundEngine::SetEventVolume(myMusicList[aSoundID], aPercentage);
}

float AudioManager::GetBusVolume(BusID anID) 
{
	return myBusVolume[anID];
}

float AudioManager::GetMasterVolume() 
{
	return myMasterVolume;
}

float AudioManager::GetEventVolume() 
{
	return myEventVolume;
}

void AudioManager::SetAudioListener(const Tga::Vector3f aPosition)
{
	myListenerPosition = aPosition * 0.1f;
	float distanceFallOff = 0.1f; // Convert units here! :D
	SoundEngine::SetListenerPosition(myListener, { myListenerPosition.x * distanceFallOff, myListenerPosition.y * distanceFallOff, myListenerPosition.z * distanceFallOff });
}

const Tga::Vector3f& AudioManager::GetAudioListenerPosition() const
{
	return myListenerPosition;
}

void AudioManager::FadeOut(const SoundID anID, const float aFadeSpeed)
{
	myFadeID = anID;
	myFadeSpeed = aFadeSpeed;
	myFadeOutActive = true;
	myMusicVolume = 0.0f;
}

void AudioManager::FadeIn(const SoundID anID, const float aFadeSpeed)
{
	myFadeID = anID;
	myFadeSpeed = aFadeSpeed;
	myFadeInActive = true;
	myMusicVolume = 1.0f;
}

void AudioManager::ChangePitch(const SoundID anID, const float aPitchChange)
{
	SoundEngine::SetEventPitch(myMusicList[anID], aPitchChange);
}

void AudioManager::ChangeReverb(const SoundID /*anID*/, const float /*aReverbChange*/)
{
	//SoundEngine::SetEventReverbLevel(myMusicList[anID], 0, aReverbChange); // Reverd is currently disabled, due to not understanding anIndex :c
}

void AudioManager::ResetSounds()
{
	myFadeOutActive = false;
	myFadeInActive = false;
}

void AudioManager::RegisterAllEvents()
{
	SoundEngine::RegisterEvent("event:/VineBoom", SoundID::eVineBoom);
	myMusicList.insert({ SoundID::eVineBoom, SoundEngine::CreateEventInstance(SoundID::eVineBoom)});
	SoundEngine::RegisterEvent("event:/MusicLoop", SoundID::eMusicLoop);
	myMusicList.insert({ SoundID::eMusicLoop, SoundEngine::CreateEventInstance(SoundID::eMusicLoop) });
	SoundEngine::RegisterEvent("event:/BasicMonsterVox", SoundID::eBasicVox);
	myMusicList.insert({ SoundID::eBasicVox, SoundEngine::CreateEventInstance(SoundID::eBasicVox) });
	SoundEngine::RegisterEvent("event:/BasicEnemyAttackVox", SoundID::eBasicAttackVox);
	myMusicList.insert({ SoundID::eBasicAttackVox, SoundEngine::CreateEventInstance(SoundID::eBasicAttackVox) });

	SoundEngine::RegisterEvent("event:/HeavyMonsterVox", SoundID::eHeavyVox);
	myMusicList.insert({ SoundID::eHeavyVox, SoundEngine::CreateEventInstance(SoundID::eHeavyVox) });

	SoundEngine::RegisterEvent("event:/HeavyMonsterRolling", SoundID::eRoll);
	myMusicList.insert({ SoundID::eRoll, SoundEngine::CreateEventInstance(SoundID::eRoll) });
	SoundEngine::RegisterEvent("event:/HeavyMonsterRollBegin", SoundID::eRollBegin);
	myMusicList.insert({ SoundID::eRollBegin, SoundEngine::CreateEventInstance(SoundID::eRollBegin) });
	SoundEngine::RegisterEvent("event:/EnemyDeadVox", SoundID::eEnemyDeadVox);
	myMusicList.insert({ SoundID::eEnemyDeadVox, SoundEngine::CreateEventInstance(SoundID::eEnemyDeadVox) });


	SoundEngine::RegisterEvent("event:/PlayerChargeGun", SoundID::eCharge);
	myMusicList.insert({ SoundID::eCharge, SoundEngine::CreateEventInstance(SoundID::eCharge) });

	SoundEngine::RegisterEvent("event:/PlayerGunShoot", SoundID::eShoot);
	myMusicList.insert({ SoundID::eShoot, SoundEngine::CreateEventInstance(SoundID::eShoot) });

	SoundEngine::RegisterEvent("event:/PlayerAttack", SoundID::ePlayerAttack);
	myMusicList.insert({ SoundID::ePlayerAttack, SoundEngine::CreateEventInstance(SoundID::ePlayerAttack) });

	SoundEngine::RegisterEvent("event:/Gore", SoundID::eGore);
	myMusicList.insert({ SoundID::eGore, SoundEngine::CreateEventInstance(SoundID::eGore) });
	SoundEngine::RegisterEvent("event:/RootDoor", SoundID::eRootDoor);
	myMusicList.insert({ SoundID::eRootDoor, SoundEngine::CreateEventInstance(SoundID::eRootDoor) });

	SoundEngine::RegisterEvent("event:/MainMenuMusic", SoundID::eMainMenuMusic);
	myMusicList.insert({ SoundID::eMainMenuMusic, SoundEngine::CreateEventInstance(SoundID::eMainMenuMusic) });
	SoundEngine::RegisterEvent("event:/Level1Music", SoundID::eMusicLevel1);
	myMusicList.insert({ SoundID::eMusicLevel1, SoundEngine::CreateEventInstance(SoundID::eMusicLevel1) });
	SoundEngine::RegisterEvent("event:/Level2Music", SoundID::eMusicLevel2);
	myMusicList.insert({ SoundID::eMusicLevel2, SoundEngine::CreateEventInstance(SoundID::eMusicLevel2) });
	SoundEngine::RegisterEvent("event:/Level3Music", SoundID::eMusicLevel3);
	myMusicList.insert({ SoundID::eMusicLevel3, SoundEngine::CreateEventInstance(SoundID::eMusicLevel3) });

	SoundEngine::RegisterEvent("event:/IntroSFX", SoundID::eIntroSFX);
	myMusicList.insert({ SoundID::eIntroSFX, SoundEngine::CreateEventInstance(SoundID::eIntroSFX) });
	SoundEngine::RegisterEvent("event:/OutroSFX", SoundID::eOutroSFX);
	myMusicList.insert({ SoundID::eOutroSFX, SoundEngine::CreateEventInstance(SoundID::eOutroSFX) });
}

AudioManager::AudioManager()
{
}

AudioManager::~AudioManager()
{
	myMusicList.clear();
	myBusses.clear();
	//SoundEngine::Release();
}
