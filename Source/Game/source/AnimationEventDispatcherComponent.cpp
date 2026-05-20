#include "AnimationEventDispatcherComponent.h"

#include "AnimationEventListener.h"
#include "AnimationGraphComponent.h"
#include "Essentials.h"
#include "GameObject.h"
#include "PostMaster.h"

#include <vector>

void AnimationEventDispatcherComponent::OnStart()
{
    ResolveGraph();
}

void AnimationEventDispatcherComponent::OnEnable()
{
    ClearQueuedEvents();
}

void AnimationEventDispatcherComponent::OnLateUpdate(float aDeltaTime)
{
    aDeltaTime;

    // AnimationGraphComponent updates before this component on the same object,
    // so LateUpdate is the point where the graph event queue is complete.
    AnimationGraphComponent* graph = ResolveGraph();
    if (!graph)
    {
        return;
    }

    std::vector<AnimationEventRecord> events = graph->GetEventQueue().Drain();
    if (events.empty())
    {
        return;
    }

    for (const AnimationEventRecord& eventRecord : events)
    {
        AnimationEventContext context;
        context.record = eventRecord;
        context.owner = GetOwner();
        context.graph = graph;

        // Dispatch locally first so components on the animated object can react without
        // going through the global message bus.
        DispatchToLocalListeners(context);

        DispatchToAnimationEventService(context);

        // Also publish globally for systems that are not components on the animated object.
        DispatchToPostMaster(context);
    }
}

void AnimationEventDispatcherComponent::OnDisable()
{
    ClearQueuedEvents();
    if (Essentials::globalAnimationEvents)
    {
        Essentials::globalAnimationEvents->ReleaseOwner(GetOwner());
    }
}

void AnimationEventDispatcherComponent::OnScriptDestroy()
{
    ClearQueuedEvents();
    if (Essentials::globalAnimationEvents)
    {
        Essentials::globalAnimationEvents->ReleaseOwner(GetOwner());
    }
    myGraph = nullptr;
}

AnimationGraphComponent* AnimationEventDispatcherComponent::ResolveGraph()
{
    if (myGraph)
    {
        return myGraph;
    }

    if (GameObject* owner = GetOwner())
    {
        myGraph = owner->GetComponent<AnimationGraphComponent>();
    }

    return myGraph;
}

void AnimationEventDispatcherComponent::ClearQueuedEvents()
{
    if (AnimationGraphComponent* graph = ResolveGraph())
    {
        graph->GetEventQueue().Clear();
    }
}

void AnimationEventDispatcherComponent::DispatchToLocalListeners(const AnimationEventContext& anEvent) const
{
    if (!anEvent.owner)
    {
        return;
    }

    std::vector<AnimationEventListener*> listeners;
    anEvent.owner->GetComponentsOfType(listeners);

    for (AnimationEventListener* listener : listeners)
    {
        if (!listener)
        {
            continue;
        }

        // Listener interfaces are not necessarily Components, but the current
        // game-side listeners are. Respect disabled components when possible.
        if (const Component* listenerComponent = dynamic_cast<const Component*>(listener))
        {
            if (!listenerComponent->IsEnabled())
            {
                continue;
            }
        }

        listener->OnAnimationEvent(anEvent);
    }
}

void AnimationEventDispatcherComponent::DispatchToAnimationEventService(const AnimationEventContext& anEvent) const
{
    if (Essentials::globalAnimationEvents)
    {
        Essentials::globalAnimationEvents->Dispatch(anEvent);
    }
}

void AnimationEventDispatcherComponent::DispatchToPostMaster(const AnimationEventContext& anEvent) const
{
    if (!Essentials::globalPostMaster || !anEvent.owner)
    {
        return;
    }

    AnimationEventMessage eventMessage;
    eventMessage.record = anEvent.record;
    eventMessage.senderObject = anEvent.owner;
    eventMessage.sourceGraph = anEvent.graph;

    Message message = {};
    message.myMessageType = MessageType::AnimationEvent;
    message.myData = eventMessage;
    message.mySender = anEvent.owner;
    Essentials::globalPostMaster->SendMsg(message);
}
