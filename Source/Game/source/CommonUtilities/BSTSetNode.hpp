#pragma once

namespace CommonUtilities
{
    template <class T>
    class BSTSet;

    template <class T>
    class BSTSetNode
    {
    public:
        BSTSetNode(const BSTSetNode<T>&) = delete;
        BSTSetNode<T>& operator=(const BSTSetNode<T>&) = delete;

        const T& GetData() const;
        const BSTSetNode<T>* GetLeft() const;
        const BSTSetNode<T>* GetRight() const;

    private:
        friend class BSTSet<T>;

        explicit BSTSetNode(const T& aData);
        ~BSTSetNode() = default;

        T myData;
        BSTSetNode<T>* myLeft;
        BSTSetNode<T>* myRight;
    };

    template <class T>
    inline BSTSetNode<T>::BSTSetNode(const T& aData)
        : myData(aData)
        , myLeft(nullptr)
        , myRight(nullptr)
    {
    }

    template <class T>
    inline const T& BSTSetNode<T>::GetData() const
    {
        return myData;
    }

    template <class T>
    inline const BSTSetNode<T>* BSTSetNode<T>::GetLeft() const
    {
        return myLeft;
    }

    template <class T>
    inline const BSTSetNode<T>* BSTSetNode<T>::GetRight() const
    {
        return myRight;
    }
}
