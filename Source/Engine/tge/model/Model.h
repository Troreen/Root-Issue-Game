#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include <tge/graphics/Vertex.h>
#include <tge/Animation/Skeleton.h>
#include <tge/Math/Vector.h>
#include <tge/Math/Matrix4x4.h>
#include <tge/EngineDefines.h>

struct ID3D11Buffer;

namespace Tga
{

class TextureResource;

struct BoxSphereBounds
{
	// The radius of the Sphere
	float Radius;
	// The extents of the Box
	Vector3f BoxExtents;
	// The local-space center of the shape.
	Vector3f Center;
};

class Model
{
public:

	friend class ModelFactory;

	struct MeshData
	{
		std::string Name;
		std::string MaterialName;
		uint32_t NumberOfVertices;
		uint32_t NumberOfIndices;
		uint32_t Stride;
		uint32_t Offset;
		ID3D11Buffer* VertexBuffer;
		ID3D11Buffer* IndexBuffer;
		BoxSphereBounds Bounds;
		std::vector<Vertex> Vertices;
		std::vector<unsigned int> Indices;
	};
		
	void Init(MeshData& aMeshData, const std::string& aPath);
	void Init(std::vector<MeshData>& someMeshData, const std::string& aPath);

	const char* GetMaterialName(int meshIndex) const { return myMeshData[meshIndex].MaterialName.c_str(); }
	const char* GetMeshName(int meshIndex) const { return myMeshData[meshIndex].Name.c_str(); }

	size_t GetMeshCount() const {return myMeshData.size();}
	MeshData const& GetMeshData(unsigned int anIndex) const { return myMeshData[anIndex]; }
	const std::vector<MeshData>& GetMeshDataList() const { return myMeshData; }

	const std::string& GetPath() { return myPath; }
	const Skeleton* GetSkeleton() const { return &mySkeleton; }

	void SetDefaultTexture(int meshIndex, int textureIndex, TextureResource* texture) { myDefaultTextures[meshIndex][textureIndex] = texture; }
	const TextureResource* const* GetDefaultTextures(size_t meshIndex) const { return myDefaultTextures[meshIndex]; }
private:

	Skeleton mySkeleton;
	std::vector<MeshData> myMeshData;
	std::string myPath;

	const TextureResource* myDefaultTextures[MAX_MESHES_PER_MODEL][4] = {};
};

} // namespace Tga