#pragma once

#include "DoublyLinkedListNode.hpp"


namespace CommonUtilities
{
    template <typename T>
    class DoublyLinkedList
    {
    public:
        // Creates an empty list.
        DoublyLinkedList();

        // Frees all memory that the list has allocated.
        ~DoublyLinkedList();

        // Returns amount of elements in the list
        int GetSize() const;

        // Returns the first node in the list, or nullptr if the list is empty.
        DoublyLinkedListNode<T>* GetFirst() const;

        // Returns the last node in the list, or nullptr if the list is empty.
        DoublyLinkedListNode<T>* GetLast() const;

        // Insert a new element at the beginning of the list
        void InsertFirst(const T& aValue);

        // Insert a new element at the end of the list
        void InsertLast(const T& aValue);

        // Insert a new element before aNode
        void InsertBefore(DoublyLinkedListNode<T> *aNode, const T &aValue);
        
        // Insert a new element after aNode
        void InsertAfter(DoublyLinkedListNode<T> *aNode, const T &aValue);
        
        // Removes a node from the list and frees its memory. 
        // (It is ok to assume that aNode is a node in the list, and not from another list)
        void Remove(DoublyLinkedListNode<T> *aNode);
        
        // Finds the first element in the list that has a certain value. 
        // The comparison is done with operator==. 
        // If no element is found, nullptr is returned.
        DoublyLinkedListNode<T> *FindFirst(const T &aValue);
        
        // Finds the last element in the list that has a certain value. 
        // The comparison is done with operator==. 
        // If no element is found, nullptr is returned.
        DoublyLinkedListNode<T> *FindLast(const T &aValue);
        
        // Removes the frist element in the list that has a certain value. 
        // The comparison is done with operator==. If no element is found, nothing is done. 
        // Returns true if an element was removed, and false otherwise.
        bool RemoveFirst(const T &aValue);

        // Removes the last element in the list that has a certain value.
        // The comparison is done with operator==. If no element is found, nothing is done
        // Returns true if an element was removed, and false otherwise.
        bool RemoveLast(const T &aValue);

    private:
        // Add all the private members you need here. 
        void LinkBetween(DoublyLinkedListNode<T>* aNewNode, DoublyLinkedListNode<T>* aPrevious, DoublyLinkedListNode<T>* aNext);
        void UnlinkNode(DoublyLinkedListNode<T>* aNode);

        DoublyLinkedListNode<T>* myFirst;
        DoublyLinkedListNode<T>* myLast;
        int mySize;
    };

    template <typename T>
    inline DoublyLinkedList<T>::DoublyLinkedList()
        : myFirst(nullptr)
        , myLast(nullptr)
        , mySize(0)
    {
    }

    template <typename T>
    inline DoublyLinkedList<T>::~DoublyLinkedList()
    {
        DoublyLinkedListNode<T>* current = myFirst;
        while (current != nullptr)
        {
            DoublyLinkedListNode<T>* next = current->myNext;
            delete current;
            current = next;
        }

        myFirst = nullptr;
        myLast = nullptr;
        mySize = 0;
    }

    template <typename T>
    inline int DoublyLinkedList<T>::GetSize() const
    {
        return mySize;
    }

    template <typename T>
    inline DoublyLinkedListNode<T>* DoublyLinkedList<T>::GetFirst() const
    {
        return myFirst;
    }

    template <typename T>
    inline DoublyLinkedListNode<T>* DoublyLinkedList<T>::GetLast() const
    {
        return myLast;
    }

    template <typename T>
    inline void DoublyLinkedList<T>::InsertFirst(const T& aValue)
    {
        DoublyLinkedListNode<T>* newNode = new DoublyLinkedListNode<T>(aValue);
        LinkBetween(newNode, nullptr, myFirst);
    }

    template <typename T>
    inline void DoublyLinkedList<T>::InsertLast(const T& aValue)
    {
        DoublyLinkedListNode<T>* newNode = new DoublyLinkedListNode<T>(aValue);
        LinkBetween(newNode, myLast, nullptr);
    }

    template <typename T>
    inline void DoublyLinkedList<T>::InsertBefore(DoublyLinkedListNode<T>* aNode, const T& aValue)
    {
        if (aNode == nullptr)
        {
            return;
        }

        DoublyLinkedListNode<T>* newNode = new DoublyLinkedListNode<T>(aValue);
        LinkBetween(newNode, aNode->myPrevious, aNode);
    }

    template <typename T>
    inline void DoublyLinkedList<T>::InsertAfter(DoublyLinkedListNode<T>* aNode, const T& aValue)
    {
        if (aNode == nullptr)
        {
            return;
        }

        DoublyLinkedListNode<T>* newNode = new DoublyLinkedListNode<T>(aValue);
        LinkBetween(newNode, aNode, aNode->myNext);
    }

    template <typename T>
    inline void DoublyLinkedList<T>::Remove(DoublyLinkedListNode<T>* aNode)
    {
        if (aNode == nullptr)
        {
            return;
        }

        UnlinkNode(aNode);
        delete aNode;
    }

    template <typename T>
    inline DoublyLinkedListNode<T>* DoublyLinkedList<T>::FindFirst(const T& aValue)
    {
        DoublyLinkedListNode<T>* current = myFirst;
        while (current != nullptr)
        {
            if (current->myValue == aValue)
            {
                return current;
            }

            current = current->myNext;
        }

        return nullptr;
    }

    template <typename T>
    inline DoublyLinkedListNode<T>* DoublyLinkedList<T>::FindLast(const T& aValue)
    {
        DoublyLinkedListNode<T>* current = myLast;
        while (current != nullptr)
        {
            if (current->myValue == aValue)
            {
                return current;
            }

            current = current->myPrevious;
        }

        return nullptr;
    }

    template <typename T>
    inline bool DoublyLinkedList<T>::RemoveFirst(const T& aValue)
    {
        DoublyLinkedListNode<T>* node = FindFirst(aValue);
        if (node == nullptr)
        {
            return false;
        }

        Remove(node);
        return true;
    }

    template <typename T>
    inline bool DoublyLinkedList<T>::RemoveLast(const T& aValue)
    {
        DoublyLinkedListNode<T>* node = FindLast(aValue);
        if (node == nullptr)
        {
            return false;
        }

        Remove(node);
        return true;
    }

    template <typename T>
    inline void DoublyLinkedList<T>::LinkBetween(DoublyLinkedListNode<T>* aNewNode, DoublyLinkedListNode<T>* aPrevious, DoublyLinkedListNode<T>* aNext)
    {
        aNewNode->myPrevious = aPrevious;
        aNewNode->myNext = aNext;

        if (aPrevious != nullptr)
        {
            aPrevious->myNext = aNewNode;
        }
        else
        {
            myFirst = aNewNode;
        }

        if (aNext != nullptr)
        {
            aNext->myPrevious = aNewNode;
        }
        else
        {
            myLast = aNewNode;
        }

        ++mySize;
    }

    template <typename T>
    inline void DoublyLinkedList<T>::UnlinkNode(DoublyLinkedListNode<T>* aNode)
    {
        if (aNode->myPrevious != nullptr)
        {
            aNode->myPrevious->myNext = aNode->myNext;
        }
        else
        {
            myFirst = aNode->myNext;
        }

        if (aNode->myNext != nullptr)
        {
            aNode->myNext->myPrevious = aNode->myPrevious;
        }
        else
        {
            myLast = aNode->myPrevious;
        }

        aNode->myPrevious = nullptr;
        aNode->myNext = nullptr;

        --mySize;
    }
}