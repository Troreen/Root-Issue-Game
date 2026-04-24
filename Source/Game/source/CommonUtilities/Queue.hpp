#pragma once

#include <cassert>
#include <new>
#include <utility>

namespace CommonUtilities
{
    template <class T>
    class Queue
    {
    public:
        // Creates an empty queue.
        Queue();
        // Frees all memory that the queue has allocated.
        ~Queue();

        Queue(const Queue& aOther);
        Queue(Queue&& aOther) noexcept;
        Queue& operator=(const Queue& aOther);
        Queue& operator=(Queue&& aOther) noexcept;

        // Returns amount of elements in the queue
        int GetSize() const;

        // Returns true if the queue contains no elements.
        bool IsEmpty() const;

        // Returns current allocated capacity.
        int GetCapacity() const;

        // Returns the front element in the queue. Crashes with assert if the queue is empty.
        const T &GetFront() const;

        // Returns the front element in the queue. Crashes with assert if the queue is empty.
        T &GetFront();

        // Pushes a new element at the back of the queue.
        void Enqueue(const T &aValue);
        void Enqueue(T&& aValue);

        // Removes the front element from the queue. Crashes with assert if the queue is empty.
        T Dequeue();

        // Removes all elements from the queue.
        void Clear();

        // Ensures that at least aCapacity elements can be stored without reallocating.
        void Reserve(int aCapacity);

        // Exchanges the internal state with another queue.
        void Swap(Queue& aOther) noexcept;

    private:
        int GetPhysicalIndex(int aLogicalIndex) const;

        int mySize;
        int myCapacity;
        int myFront;
        T* myData;
    };

    template <class T>
    Queue<T>::Queue()
        : mySize(0)
        , myCapacity(0)
        , myFront(0)
        , myData(nullptr)
    {
    }

    template <class T>
    Queue<T>::~Queue()
    {
        Clear();
        ::operator delete(myData);
    }

    template <class T>
    Queue<T>::Queue(const Queue& aOther)
        : mySize(0)
        , myCapacity(0)
        , myFront(0)
        , myData(nullptr)
    {
        Reserve(aOther.mySize);
        for (int index = 0; index < aOther.mySize; ++index)
        {
            const int sourceIndex = aOther.GetPhysicalIndex(index);
            new (&myData[index]) T(aOther.myData[sourceIndex]);
        }

        mySize = aOther.mySize;
    }

    template <class T>
    Queue<T>::Queue(Queue&& aOther) noexcept
        : mySize(aOther.mySize)
        , myCapacity(aOther.myCapacity)
        , myFront(aOther.myFront)
        , myData(aOther.myData)
    {
        aOther.mySize = 0;
        aOther.myCapacity = 0;
        aOther.myFront = 0;
        aOther.myData = nullptr;
    }

    template <class T>
    Queue<T>& Queue<T>::operator=(const Queue& aOther)
    {
        if (this != &aOther)
        {
            Queue copy(aOther);
            Swap(copy);
        }

        return *this;
    }

    template <class T>
    Queue<T>& Queue<T>::operator=(Queue&& aOther) noexcept
    {
        if (this != &aOther)
        {
            Clear();
            ::operator delete(myData);

            mySize = aOther.mySize;
            myCapacity = aOther.myCapacity;
            myFront = aOther.myFront;
            myData = aOther.myData;

            aOther.mySize = 0;
            aOther.myCapacity = 0;
            aOther.myFront = 0;
            aOther.myData = nullptr;
        }

        return *this;
    }

    template <class T>
    int Queue<T>::GetSize() const
    {
        return mySize;
    }

    template <class T>
    bool Queue<T>::IsEmpty() const
    {
        return mySize == 0;
    }

    template <class T>
    int Queue<T>::GetCapacity() const
    {
        return myCapacity;
    }

    template <class T>
    const T& Queue<T>::GetFront() const
    {
        assert(mySize > 0);
        return myData[myFront];
    }

    template <class T>
    T& Queue<T>::GetFront()
    {
        assert(mySize > 0);
        return myData[myFront];
    }

    template <class T>
    void Queue<T>::Enqueue(const T& aValue)
    {
        if (mySize == myCapacity)
        {
            Reserve(myCapacity == 0 ? 4 : myCapacity * 2);
        }

        const int insertIndex = GetPhysicalIndex(mySize);
        new (&myData[insertIndex]) T(aValue);
        ++mySize;
    }

    template <class T>
    void Queue<T>::Enqueue(T&& aValue)
    {
        if (mySize == myCapacity)
        {
            Reserve(myCapacity == 0 ? 4 : myCapacity * 2);
        }

        const int insertIndex = GetPhysicalIndex(mySize);
        new (&myData[insertIndex]) T(std::move(aValue));
        ++mySize;
    }

    template <class T>
    T Queue<T>::Dequeue()
    {
        assert(mySize > 0);

        T value(std::move(myData[myFront]));
        myData[myFront].~T();

        myFront = (myFront + 1) % myCapacity;
        --mySize;

        if (mySize == 0)
        {
            myFront = 0;
        }

        return value;
    }

    template <class T>
    void Queue<T>::Clear()
    {
        for (int index = 0; index < mySize; ++index)
        {
            myData[GetPhysicalIndex(index)].~T();
        }

        mySize = 0;
        myFront = 0;
    }

    template <class T>
    void Queue<T>::Reserve(int aCapacity)
    {
        if (aCapacity <= myCapacity)
        {
            return;
        }

        T* newData = static_cast<T*>(::operator new(sizeof(T) * static_cast<size_t>(aCapacity)));

        for (int index = 0; index < mySize; ++index)
        {
            const int sourceIndex = GetPhysicalIndex(index);
            new (&newData[index]) T(std::move_if_noexcept(myData[sourceIndex]));
            myData[sourceIndex].~T();
        }

        ::operator delete(myData);
        myData = newData;
        myCapacity = aCapacity;
        myFront = 0;
    }

    template <class T>
    void Queue<T>::Swap(Queue& aOther) noexcept
    {
        std::swap(mySize, aOther.mySize);
        std::swap(myCapacity, aOther.myCapacity);
        std::swap(myFront, aOther.myFront);
        std::swap(myData, aOther.myData);
    }

    template <class T>
    int Queue<T>::GetPhysicalIndex(int aLogicalIndex) const
    {
        assert(myCapacity > 0);
        return (myFront + aLogicalIndex) % myCapacity;
    }
}