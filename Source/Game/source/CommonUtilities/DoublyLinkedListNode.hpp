#pragma once

template <typename T>
class DoublyLinkedList;

namespace CommonUtilities
{
    template <class T>
    class DoublyLinkedListNode
    {
    public:
        // Copy-ctr and assignment operator are deleted, so the only way to create a node is by inserting a value into a list.
        DoublyLinkedListNode<T>(const DoublyLinkedListNode<T> &) = delete;
        DoublyLinkedListNode<T>& operator=(const DoublyLinkedListNode<T>&) = delete;

        // Returns the value of the node
        const T &GetValue() const;
        T &GetValue();
    
        // Returns the next node in the list, or nullptr if the node is last in the list
        DoublyLinkedListNode<T> *GetNext() const;

        // Returns the previous node in the list, or nullptr if the node is first in the list
        DoublyLinkedListNode<T> *GetPrevious() const;
    private:
        // The constructor and destructor are private, so that nodes cannot be created or
        // deleted from outside. The list class is a friend, so that it can create or delete nodes.
        friend class DoublyLinkedList<T>;

        DoublyLinkedListNode(const T& aValue);
        ~DoublyLinkedListNode() = default;

        T myValue;
        DoublyLinkedListNode<T>* myNext;
        DoublyLinkedListNode<T>* myPrevious;
    };

    template <class T>
    inline DoublyLinkedListNode<T>::DoublyLinkedListNode(const T& aValue)
        : myValue(aValue)
        , myNext(nullptr)
        , myPrevious(nullptr)
    {
    }

    template <class T>
    inline const T& DoublyLinkedListNode<T>::GetValue() const
    {
        return myValue;
    }

    template <class T>
    inline T& DoublyLinkedListNode<T>::GetValue()
    {
        return myValue;
    }

    template <class T>
    inline DoublyLinkedListNode<T>* DoublyLinkedListNode<T>::GetNext() const
    {
        return myNext;
    }

    template <class T>
    inline DoublyLinkedListNode<T>* DoublyLinkedListNode<T>::GetPrevious() const
    {
        return myPrevious;
    }
}