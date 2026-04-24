#include "stdafx.h"
#include <tge/animation/Pose.h>
#include <tge/model/Model.h>
#include <tge/shaders/ModelShader.h>

#include "Skeleton.h"

using namespace Tga;

void Skeleton::ConvertPoseToLocalSpace(const ModelSpacePose& in, LocalSpacePose& out) const
{
	for (size_t i = 0; i < Joints.size(); i++)
	{
		const Joint& joint = Joints[i];
		Matrix4x4f localTransform = joint.Parent == -1 ? in.JointTransforms[i] : in.JointTransforms[i] * in.JointTransforms[joint.Parent].GetInverse();

		Vector3f position;
		Quaternionf rotation;
		Vector3f scale;

		localTransform.DecomposeMatrix(position, rotation, scale);

		ScaleRotationTranslationf& jointTransform = out.JointTransforms[i];
		jointTransform.SetTranslation(position);
		jointTransform.SetRotation(rotation);
		jointTransform.SetScale(scale);
	}
}

void Skeleton::ConvertPoseToModelSpace(const LocalSpacePose& in, ModelSpacePose& out) const
{
	ConvertPoseToModelSpace(in, out, 0, Matrix4x4f::CreateIdentityMatrix());
	out.Count = in.Count;
}

void Skeleton::ConvertPoseToModelSpace(const LocalSpacePose& in, ModelSpacePose& out, unsigned aBoneIdx, const Matrix4x4f& aParentTransform) const
{
	const Skeleton::Joint& joint = Joints[aBoneIdx];

	out.JointTransforms[aBoneIdx] = in.JointTransforms[aBoneIdx].GetMatrix()*aParentTransform;

	for (size_t c = 0; c < joint.Children.size(); c++)
	{
		ConvertPoseToModelSpace(in, out, joint.Children[c], out.JointTransforms[aBoneIdx]);
	}
}

void Skeleton::ApplyBindPoseInverse(const ModelSpacePose& in, Matrix4x4f* out) const
{
	for (size_t i = 0; i < Joints.size(); i++)
	{
		const Skeleton::Joint& joint = Joints[i];
		out[i] = joint.BindPoseInverse * in.JointTransforms[i];
	}
}