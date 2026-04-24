#pragma once
#include <map>
#include <string>
#include <vector>
#include <unordered_set>
#include <tge/animation/animation.h>
#include <tge/graphics/Vertex.h>
#include <tge/model/model.h>
#include <tge/model/AnimatedModelInstance.h>

struct ID3D11Device;
struct BoxSphereBounds;
namespace Tga
{

class Texture;
class AnimatedModel;

class ModelInstance;
class Model;

class ModelFactory
{
	bool InitUnitCube();
	bool InitUnitPlane();
	bool InitPrimitives();
private:
	ModelFactory();
	~ModelFactory();
public:

	static ModelFactory& GetInstance() { if (!myInstance) { myInstance = new ModelFactory(); } return *myInstance; }
	static void DestroyInstance() { if (myInstance) { delete myInstance; myInstance = nullptr; } }

	std::shared_ptr<Model> GetModel(const std::string& someFilePath);
	AnimatedModelInstance GetAnimatedModelInstance(const std::string& someFilePath);
	ModelInstance GetModelInstance(const std::string& someFilePath);

	std::shared_ptr<Animation> GetAnimation(const std::string& someFilePath, const std::shared_ptr<Model>& aModel);
	AnimationPlayer GetAnimationPlayer(const std::string& someFilePath, const std::shared_ptr<Model>& aModel);

	ModelInstance GetUnitCube();
	ModelInstance GetUnitPlane();

protected:
	
	std::shared_ptr<Model> LoadModel(const std::string& someFilePath);
	Tga::BoxSphereBounds CalculateBoxSphereBounds(std::vector<Tga::Vertex> somePositions);
private:	
	void OnModelChanged(const std::string& aUnresolvedPath);

	struct AnimationIdentifer
	{
		std::string Path;
		std::shared_ptr<Model> Model;

		bool operator==(const AnimationIdentifer& other) const
		{
			return Path == other.Path && Model == other.Model;
		};
	};

	struct AnimationIdentiferHash
	{
		std::size_t operator()(const AnimationIdentifer& identifier) const
		{
			return (std::hash<std::string>()(identifier.Path)) * 31 + std::hash<std::shared_ptr<Model>>()(identifier.Model);
		}
	};

	std::unordered_set<std::string> myWatchedPaths;
	std::unordered_map<std::string, std::shared_ptr<Model>> myLoadedModels;	
	std::unordered_map<AnimationIdentifer, std::shared_ptr<Animation>, AnimationIdentiferHash> myLoadedAnimations;

	static ModelFactory* myInstance;
};

} // namespace Tga
