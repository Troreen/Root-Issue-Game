// https://gameprogrammingpatterns.com/state.html 
// Denna klass blir nog bara en exempel klass och ett ställe för mig att anteckna 
// UPPDATERING: Jag använder nu denna som en basklass för alla states för att lättare kunna integrera den med animator
#pragma once
#include <string>

class Animator;

class State 
{
	public:
		State() = default;
		virtual ~State() = default;
		//State(Animator& anAnimator);
		/*virtual void Update(Animator& anAnimator, const float aDeltaTime);
		virtual void HandleInput(Animator& anAnimator);
		virtual void Enter(Animator& anAnimator);
		virtual void Exit(Animator& anAnimator);*/
		virtual std::string GetName() const = 0; // Only for debug
};

// STATEMACHINE
// I en t.ex player klass kommer det finnas en playerstate*. 
// Man vill ha en specifik klass för att alla objekt kommer ha sina egna specifika tillstånd, finns ingen "one size fits all"
// Playerstate* kommer att uppdateras och bytas konstant i själva player klassen oavsett vilken state den har. 
// 
// FSM är som Queen of England, den finns inte

// ANIMATIONS
// Meshcomponent har en modelinstance som vi kan skicka in model factory för att få en animation player. 
// Structure exempel:
// Make modelinstance // Behöver man skicka in en modelinstance eller får man samma resultat av att bara lägga in filepath
// Make MeshComponent (ModelInstance) 
// std::vector(AnimationPlayer) = 
//{
//		modelfactory.getAnimationPlayer("filepath för animation", MeshComponent.GetModelInstance),
//		modelfactory.getAnimationPlayer("filepath för en annan animation", MeshComponent.GetModelInstance)
//};
// 
// public enum AnimationStates{
//		Walk,
//		Run
// };
// 
// AnimationState.Enter(PlayerAnimator& anAnimator) {
//		aPlayer.ChangeAnimation(static_cast<int>(aPlayer.AnimationStates)); 
// }
// 
// 
//