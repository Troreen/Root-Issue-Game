#pragma once

#include "BSTSetNode.hpp"

namespace CommonUtilities
{
    template <class T>
    class BSTSet
    {
    public:
        BSTSet();
        ~BSTSet();

        BSTSet(const BSTSet<T>&) = delete;
        BSTSet<T>& operator=(const BSTSet<T>&) = delete;

        // Returns the root.
        const BSTSetNode<T>* GetRoot() const;

        // Returns true if the element is in the set, and false otherwise.
        bool HasElement(const T& aData) const;

        // Inserts the element into the set, if it's not already there. Does nothing otherwise.
        void Insert(const T& aData);

        // Removes the element from the set, if it exists. Does nothing otherwise.
        void Remove(const T& aData);

        // Balances the tree using the DSW algorithm.
        void DSWBalance();

    private:
        static bool IsLess(const T& aLeft, const T& aRight);
        static bool IsEqual(const T& aLeft, const T& aRight);

        static void RotateLeftAt(BSTSetNode<T>** aLink);
        static void RotateRightAt(BSTSetNode<T>** aLink);

        static void RemoveNodeAt(BSTSetNode<T>** aLink);

        void DeleteSubtree(BSTSetNode<T>* aNode);
        int CreateBackbone();
        void Compress(int aCount);

        BSTSetNode<T>* myRoot;
    };

    template <class T>
    inline BSTSet<T>::BSTSet()
        : myRoot(nullptr)
    {
    }

    template <class T>
    inline BSTSet<T>::~BSTSet()
    {
        DeleteSubtree(myRoot);
        myRoot = nullptr;
    }

    template <class T>
    inline const BSTSetNode<T>* BSTSet<T>::GetRoot() const
    {
        return myRoot;
    }

    template <class T>
    inline bool BSTSet<T>::HasElement(const T& aData) const
    {
        BSTSetNode<T>* current = myRoot;

        while (current != nullptr)
        {
            if (IsLess(aData, current->myData))
            {
                current = current->myLeft;
            }
            else if (IsLess(current->myData, aData))
            {
                current = current->myRight;
            }
            else
            {
                return true;
            }
        }

        return false;
    }

    template <class T>
    inline void BSTSet<T>::Insert(const T& aData)
    {
        BSTSetNode<T>** link = &myRoot;

        while (*link != nullptr)
        {
            if (IsLess(aData, (*link)->myData))
            {
                link = &((*link)->myLeft);
            }
            else if (IsLess((*link)->myData, aData))
            {
                link = &((*link)->myRight);
            }
            else
            {
                return;
            }
        }

        *link = new BSTSetNode<T>(aData);
    }

    template <class T>
    inline void BSTSet<T>::Remove(const T& aData)
    {
        BSTSetNode<T>** link = &myRoot;

        while (*link != nullptr)
        {
            if (IsLess(aData, (*link)->myData))
            {
                link = &((*link)->myLeft);
            }
            else if (IsLess((*link)->myData, aData))
            {
                link = &((*link)->myRight);
            }
            else
            {
                RemoveNodeAt(link);
                return;
            }
        }
    }

    template <class T>
    inline void BSTSet<T>::DSWBalance()
    {
        const int nodeCount = CreateBackbone();
        if (nodeCount < 2)
        {
            return;
        }

        int greatestPowerOfTwo = 1;
        while (greatestPowerOfTwo <= nodeCount + 1)
        {
            greatestPowerOfTwo <<= 1;
        }

        greatestPowerOfTwo >>= 1;

        const int perfectNodeCount = greatestPowerOfTwo - 1;
        const int leafCount = nodeCount - perfectNodeCount;

        Compress(leafCount);

        for (int remaining = perfectNodeCount; remaining > 1; remaining /= 2)
        {
            Compress(remaining / 2);
        }
    }

    template <class T>
    inline bool BSTSet<T>::IsLess(const T& aLeft, const T& aRight)
    {
        return aLeft < aRight;
    }

    template <class T>
    inline bool BSTSet<T>::IsEqual(const T& aLeft, const T& aRight)
    {
        return !IsLess(aLeft, aRight) && !IsLess(aRight, aLeft);
    }

    template <class T>
    inline void BSTSet<T>::RotateLeftAt(BSTSetNode<T>** aLink)
    {
        BSTSetNode<T>* parent = *aLink;
        BSTSetNode<T>* rightChild = parent->myRight;

        parent->myRight = rightChild->myLeft;
        rightChild->myLeft = parent;
        *aLink = rightChild;
    }

    template <class T>
    inline void BSTSet<T>::RotateRightAt(BSTSetNode<T>** aLink)
    {
        BSTSetNode<T>* parent = *aLink;
        BSTSetNode<T>* leftChild = parent->myLeft;

        parent->myLeft = leftChild->myRight;
        leftChild->myRight = parent;
        *aLink = leftChild;
    }

    template <class T>
    inline void BSTSet<T>::RemoveNodeAt(BSTSetNode<T>** aLink)
    {
        BSTSetNode<T>* node = *aLink;

        if (node->myLeft == nullptr)
        {
            *aLink = node->myRight;
            delete node;
            return;
        }

        if (node->myRight == nullptr)
        {
            *aLink = node->myLeft;
            delete node;
            return;
        }

        BSTSetNode<T>** successorLink = &(node->myRight);
        while ((*successorLink)->myLeft != nullptr)
        {
            successorLink = &((*successorLink)->myLeft);
        }

        BSTSetNode<T>* successor = *successorLink;
        *successorLink = successor->myRight;

        successor->myLeft = node->myLeft;
        if (successor != node->myRight)
        {
            successor->myRight = node->myRight;
        }

        *aLink = successor;
        delete node;
    }

    template <class T>
    inline void BSTSet<T>::DeleteSubtree(BSTSetNode<T>* aNode)
    {
        if (aNode == nullptr)
        {
            return;
        }

        DeleteSubtree(aNode->myLeft);
        DeleteSubtree(aNode->myRight);
        delete aNode;
    }

    template <class T>
    inline int BSTSet<T>::CreateBackbone()
    {
        int nodeCount = 0;
        BSTSetNode<T>** link = &myRoot;

        while (*link != nullptr)
        {
            if ((*link)->myLeft != nullptr)
            {
                RotateRightAt(link);
            }
            else
            {
                ++nodeCount;
                link = &((*link)->myRight);
            }
        }

        return nodeCount;
    }

    template <class T>
    inline void BSTSet<T>::Compress(int aCount)
    {
        BSTSetNode<T>** link = &myRoot;

        for (int index = 0; index < aCount; ++index)
        {
            if (*link == nullptr || (*link)->myRight == nullptr)
            {
                return;
            }

            RotateLeftAt(link);
            link = &((*link)->myRight);
        }
    }
}
