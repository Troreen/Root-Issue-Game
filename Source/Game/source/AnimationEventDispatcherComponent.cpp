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

    GameObject* owner = GetOwner();
    if (!owner)
    {
        return;
    }

    std::vector<AnimationEventListener*> listeners;
    owner->GetComponentsOfType(listeners);

    for (const AnimationEventRecord& eventRecord : events)
    {
        AnimationEventContext context;
        context.record = eventRecord;
        context.owner = owner;
        context.graph = graph;

        for (AnimationEventListener* listener : listeners)
        {
            if (!listener)
            {
                continue;
            }

            if (const Component* listenerComponent = dynamic_cast<const Component*>(listener))
            {
                if (!listenerComponent->IsEnabled())
                {
                    continue;
                }
            }

            listener->OnAnimationEvent(context);
        }

        if (Essentials::globalPostMaster)
        {
            AnimationEventMessage eventMessage;
            eventMessage.record = eventRecord;
            eventMessage.senderObject = owner;
            eventMessage.sourceGraph = graph;

            Message message = {};
            message.myMessageType = MessageType::AnimationEvent;
            message.myData = eventMessage;
            message.mySender = owner;
            Essentials::globalPostMaster->SendMsg(message);
        }
    }
}

void AnimationEventDispatcherComponent::OnDisable()
{
    ClearQueuedEvents();
}

void AnimationEventDispatcherComponent::OnScriptDestroy()
{
    ClearQueuedEvents();
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
