#pragma once

#include <cassert>
#include <new>
#include <utility>

namespace CommonUtilities
{
    template <class T>
    class Stack
    {
    public:
        // Creates an empty stack.
        Stack();
        // Frees all memory that the stack has allocated.
        ~Stack();

        Stack(const Stack& aOther);
        Stack(Stack&& aOther) noexcept;
        Stack& operator=(const Stack& aOther);
        Stack& operator=(Stack&& aOther) noexcept;

        // Returns amount of elements in the stack
        int GetSize() const;

        // Returns true if the stack contains no elements.
        bool IsEmpty() const;

        // Returns current allocated capacity.
        int GetCapacity() const;

        // Returns the top element in the stack. Crashes with assert if the stack is empty.
        const T &GetTop() const;

        // Returns the top element in the stack. Crashes with assert if the stack is empty.
        T &GetTop();

        // Pushes a new element on top of the stack.
        void Push(const T &aValue);
        void Push(T&& aValue);

        // Removes the top element from the stack. Crashes with assert if the stack is empty.
        void Pop();

        // Removes all elements from the stack.
        void Clear();

        // Ensures that at least aCapacity elements can be stored without reallocating.
        void Reserve(int aCapacity);

        // Exchanges the internal state with another stack.
        void Swap(Stack& aOther) noexcept;

    private:
        int mySize;
        int myCapacity;
        T* myData;
    };

    template <class T>
    Stack<T>::Stack()
        : mySize(0)
        , myCapacity(0)
        , myData(nullptr)
    {
    }

    template <class T>
    Stack<T>::~Stack()
    {
        Clear();
        ::operator delete(myData);
    }

    template <class T>
    Stack<T>::Stack(const Stack& aOther)
        : mySize(0)
        , myCapacity(0)
        , myData(nullptr)
    {
        Reserve(aOther.mySize);
        for (int index = 0; index < aOther.mySize; ++index)
        {
            new (&myData[index]) T(aOther.myData[index]);
        }

        mySize = aOther.mySize;
    }

    template <class T>
    Stack<T>::Stack(Stack&& aOther) noexcept
        : mySize(aOther.mySize)
        , myCapacity(aOther.myCapacity)
        , myData(aOther.myData)
    {
        aOther.mySize = 0;
        aOther.myCapacity = 0;
        aOther.myData = nullptr;
    }

    template <class T>
    Stack<T>& Stack<T>::operator=(const Stack& aOther)
    {
        if (this != &aOther)
        {
            Stack copy(aOther);
            Swap(copy);
        }

        return *this;
    }

    template <class T>
    Stack<T>& Stack<T>::operator=(Stack&& aOther) noexcept
    {
        if (this != &aOther)
        {
            Clear();
            ::operator delete(myData);

            mySize = aOther.mySize;
            myCapacity = aOther.myCapacity;
            myData = aOther.myData;

            aOther.mySize = 0;
            aOther.myCapacity = 0;
            aOther.myData = nullptr;
        }

        return *this;
    }

    template <class T>
    int Stack<T>::GetSize() const
    {
        return mySize;
    }

    template <class T>
    bool Stack<T>::IsEmpty() const
    {
        return mySize == 0;
    }

    template <class T>
    int Stack<T>::GetCapacity() const
    {
        return myCapacity;
    }

    template <class T>
    const T &Stack<T>::GetTop() const
    {
        assert(mySize > 0);
        return myData[mySize - 1];
    }

    template <class T>
    T &Stack<T>::GetTop()
    {
        assert(mySize > 0);
        return myData[mySize - 1];
    }

    template <class T>
    void Stack<T>::Push(const T &aValue)
    {
        if (mySize == myCapacity)
        {
            Reserve(myCapacity == 0 ? 4 : myCapacity * 2);
        }

        new (&myData[mySize]) T(aValue);
        ++mySize;
    }

    template <class T>
    void Stack<T>::Push(T&& aValue)
    {
        if (mySize == myCapacity)
        {
            Reserve(myCapacity == 0 ? 4 : myCapacity * 2);
        }

        new (&myData[mySize]) T(std::move(aValue));
        ++mySize;
    }

    template <class T>
    void Stack<T>::Pop()
    {
        assert(mySize > 0);
        --mySize;
        myData[mySize].~T();
    }

    template <class T>
    void Stack<T>::Clear()
    {
        for (int index = mySize - 1; index >= 0; --index)
        {
            myData[index].~T();
        }

        mySize = 0;
    }

    template <class T>
    void Stack<T>::Reserve(int aCapacity)
    {
        if (aCapacity <= myCapacity)
        {
            return;
        }

        T* newData = static_cast<T*>(::operator new(sizeof(T) * static_cast<size_t>(aCapacity)));

        for (int index = 0; index < mySize; ++index)
        {
            new (&newData[index]) T(std::move_if_noexcept(myData[index]));
            myData[index].~T();
        }

        ::operator delete(myData);
        myData = newData;
        myCapacity = aCapacity;
    }

    template <class T>
    void Stack<T>::Swap(Stack& aOther) noexcept
    {
        std::swap(mySize, aOther.mySize);
        std::swap(myCapacity, aOther.myCapacity);
        std::swap(myData, aOther.myData);
    }
}