#include "Bullet.h"
#include <tge/model/ModelFactory.h>

Tga::Matrix4x4f ToTgaMatrix(const CommonUtilities::Matrix4x4<float>& aMatrix)
{
    Tga::Matrix4x4f result;
    for (int r = 1; r < 5; ++r)
    {
        for (int c = 1; c < 5; ++c)
        {
            result(r, c) = aMatrix(r, c);
        }
    }
    return result;
}


Bullet::Bullet(Tga::ModelInstance aInstance)
{
    myInstance = aInstance;
}

Bullet::~Bullet()
{

}

void Bullet::Init(CommonUtilities::Transform<float> aTransform)
{
    myIsDelete = false;
    mySpeed = 1000.f;
    myTimer = 2.f;
    myInstance.SetTransform(ToTgaMatrix(aTransform.GetWorldMatrix()));
}

void Bullet::Update(float aDeltaTime)
{
    myTimer -= aDeltaTime;
    if (myTimer < 0)
    {
        myIsDelete = true;
    }
    myInstance.GetTransform().Translate(myInstance.GetTransform().GetForward() * mySpeed * aDeltaTime);
}

void Bullet::Render(Tga::ModelDrawer& aDrawer)
{
    aDrawer.DrawPbr(myInstance);
}

bool Bullet::IsDelete()
{
    return myIsDelete;
}
