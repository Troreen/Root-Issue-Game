#include "GameObjectFactory.h"

#include "GameObject.h"
#include "SceneObjectData.h"

#include <tge/error/ErrorManager.h>

void GameObjectFactory::Register(const std::string& aType, FactoryFunction aFactory)
{
    myFactories[aType] = std::move(aFactory);
}

std::unique_ptr<GameObject> GameObjectFactory::Build(
    const std::string& aFactoryType,
    const SceneObjectData& aData) const
{
    const auto it = myFactories.find(aFactoryType);
    if (it == myFactories.end())
    {
        ERROR_PRINT(
            "GameObjectFactory::Build failed: unregistered factory type: %s",
            aFactoryType.c_str());
        return nullptr;
    }

    return it->second(aData);
}

GameObjectFactory& GameObjectFactory::GetInstance()
{
    static GameObjectFactory instance;
    return instance;
}
