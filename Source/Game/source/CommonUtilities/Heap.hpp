#pragma once

#include <cassert>
#include <utility>
#include <vector>

namespace CommonUtilities
{
    template <class T>
    class Heap
    {
    public:
        Heap();
        ~Heap();
        
        //Returns the number of elements in the heap.
        int GetSize() const;
        
        //Adds the element to the heap
        void Enqueue(const T &aElement);
        
        //Returns the largest element in the heap without removing it.
        //Crashes with an assert if the heap is empty.
        const T &GetTop() const;
        
        //Removes the largest element from the heap and returns it.
        //Crashes with an assert if the heap is empty.
        T Dequeue();

    private:
        void SiftUp(int aIndex);
        void SiftDown(int aIndex);

        std::vector<T> myData;
    };

    template <class T>
    inline Heap<T>::Heap() = default;

    template <class T>
    inline Heap<T>::~Heap() = default;

    template <class T>
    inline int Heap<T>::GetSize() const
    {
        return static_cast<int>(myData.size());
    }

    template <class T>
    inline void Heap<T>::Enqueue(const T& aElement)
    {
        myData.push_back(aElement);
        SiftUp(static_cast<int>(myData.size()) - 1);
    }

    template <class T>
    inline const T& Heap<T>::GetTop() const
    {
        assert(!myData.empty());
        return myData[0];
    }

    template <class T>
    inline T Heap<T>::Dequeue()
    {
        assert(!myData.empty());

        T topValue = std::move(myData[0]);

        if (myData.size() == 1)
        {
            myData.pop_back();
            return topValue;
        }

        myData[0] = std::move(myData.back());
        myData.pop_back();
        SiftDown(0);

        return topValue;
    }

    template <class T>
    inline void Heap<T>::SiftUp(int aIndex)
    {
        while (aIndex > 0)
        {
            const int parentIndex = (aIndex - 1) / 2;

            if (!(myData[parentIndex] < myData[aIndex]))
            {
                return;
            }

            std::swap(myData[parentIndex], myData[aIndex]);
            aIndex = parentIndex;
        }
    }

    template <class T>
    inline void Heap<T>::SiftDown(int aIndex)
    {
        const int size = static_cast<int>(myData.size());

        while (true)
        {
            const int leftChildIndex = (2 * aIndex) + 1;
            if (leftChildIndex >= size)
            {
                return;
            }

            const int rightChildIndex = leftChildIndex + 1;
            int largerChildIndex = leftChildIndex;

            if (rightChildIndex < size && myData[largerChildIndex] < myData[rightChildIndex])
            {
                largerChildIndex = rightChildIndex;
            }

            if (!(myData[aIndex] < myData[largerChildIndex]))
            {
                return;
            }

            std::swap(myData[aIndex], myData[largerChildIndex]);
            aIndex = largerChildIndex;
        }
    }
}