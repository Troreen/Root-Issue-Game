#pragma once

#include <cstdio>

#include <TGAFBXImporter/source/Importer.h>
#include "Go.h"

int main(const int argc, const char* argv[])
{
	TGA::FBX::Importer::SetShouldBakeSkeletonRootTransforms(true);

	if (argc > 1)
		printf("argv[1] => %s", argv[1]);
	else
		printf("argc => %d", argc);
	Go(argc > 1 ? argv[1] : nullptr);
	return 0;
}
