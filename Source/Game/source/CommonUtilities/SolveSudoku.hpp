#pragma once

#include <array>
#include <iostream>
#include "DoublyLinkedList.hpp"

namespace CommonUtilities
{
    struct CellPosition
    {
        int row;
        int col;
    
        bool operator==(const CellPosition& aOther) const
        {
            return row == aOther.row && col == aOther.col;
        }
    };

    
    int GetIndex(const int aRow, const int aCol)
    {
        return aRow * 9 + aCol;
    }
    
    bool IsValueInRange(const int aValue)
    {
        return aValue >= 0 && aValue <= 9;
    }
    
    bool IsValidPlacement(const std::array<int, 81>& aBoard, const int aRow, const int aCol, const int aValue)
    {
        for (int col = 0; col < 9; ++col)
        {
            if (col != aCol && aBoard[GetIndex(aRow, col)] == aValue)
            {
                return false;
            }
        }
        
        for (int row = 0; row < 9; ++row)
        {
            if (row != aRow && aBoard[GetIndex(row, aCol)] == aValue)
            {
                return false;
            }
        }
        
        const int boxStartRow = (aRow / 3) * 3;
        const int boxStartCol = (aCol / 3) * 3;
        for (int row = boxStartRow; row < boxStartRow + 3; ++row)
        {
            for (int col = boxStartCol; col < boxStartCol + 3; ++col)
            {
                if ((row != aRow || col != aCol) && aBoard[GetIndex(row, col)] == aValue)
                {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    bool IsBoardStateValid(const std::array<int, 81>& aBoard)
    {
        for (int row = 0; row < 9; ++row)
        {
            for (int col = 0; col < 9; ++col)
            {
                const int value = aBoard[GetIndex(row, col)];
                if (!IsValueInRange(value))
                {
                    return false;
                }
                
                if (value != 0 && !IsValidPlacement(aBoard, row, col, value))
                {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    void BuildEmptyCellList(const std::array<int, 81>& aBoard, DoublyLinkedList<CellPosition>& aOutEmptyCells)
    {
        for (int row = 0; row < 9; ++row)
        {
            for (int col = 0; col < 9; ++col)
            {
                if (aBoard[GetIndex(row, col)] == 0)
                {
                    aOutEmptyCells.InsertLast({ row, col });
                }
            }
        }
    }
    
    bool SolveFromNode(std::array<int, 81>& aBoard, const std::array<bool, 81>& aFixedCells, DoublyLinkedListNode<CellPosition>* aCurrentNode)
    {
        if (aCurrentNode == nullptr)
        {
            return true;
        }
        
        const CellPosition& position = aCurrentNode->GetValue();
        const int index = GetIndex(position.row, position.col);
        if (aFixedCells[index])
        {
            return false;
        }

        for (int candidate = 1; candidate <= 9; ++candidate)
        {
            if (IsValidPlacement(aBoard, position.row, position.col, candidate))
            {
                aBoard[index] = candidate;

                if (SolveFromNode(aBoard, aFixedCells, aCurrentNode->GetNext()))
                {
                    return true;
                }

                aBoard[index] = 0;
            }
        }
        
        return false;
    }
    // return true if a solution is found, false otherwise. The solved board will be stored in aBoard.
    bool SolveSudoku(std::array<int, 81>& aBoard)
    {
        const std::array<int, 81> originalBoard = aBoard;
        
        if (!IsBoardStateValid(aBoard))
        {
            return false;
        }
        
        std::array<bool, 81> fixedCells = {};
        for (size_t index = 0; index < aBoard.size(); ++index)
        {
            fixedCells[index] = aBoard[index] != 0;
        }
        
        DoublyLinkedList<CellPosition> emptyCells;
        BuildEmptyCellList(aBoard, emptyCells);
        if (emptyCells.GetSize() == 0)
        {
            return true;
        }
        
        const bool solved = SolveFromNode(aBoard, fixedCells, emptyCells.GetFirst());
        if (!solved)
        {
            aBoard = originalBoard;
        }
    
        return solved;
    }
    
}