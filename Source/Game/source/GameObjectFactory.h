#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class GameObject;
struct SceneObjectData;

class GameObjectFactory
{
public:
    using FactoryFunction = std::function<std::unique_ptr<GameObject>(const SceneObjectData&)>;

    void Register(const std::string& aType, FactoryFunction aFactory);
    std::unique_ptr<GameObject> Build(const std::string& aFactoryType, const SceneObjectData& aData) const;

    static GameObjectFactory& GetInstance();

private:
    std::unordered_map<std::string, FactoryFunction> myFactories;
};

