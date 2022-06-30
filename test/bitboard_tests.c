#include <stdio.h>
#include "bitboard_tests.h"
#include "../src/bitboard.h"
#include "test_util.h"


void anyMoveAllowedOnEmptyBoard(BitBoard* bitBoard) {
    Square moves[TOTAL_SMALL_SQUARES];
    myAssert(generateMoves(bitBoard, moves) == TOTAL_SMALL_SQUARES);
}


void nineOrEightMovesAllowedAfterFirstMove(BitBoard* bitBoard) {
    Square moves[TOTAL_SMALL_SQUARES];
    int amountOfMoves = generateMoves(bitBoard, moves);
    for (int i = 0; i < amountOfMoves; i++) {
        Square move = moves[i];
        makeMove(bitBoard, move);
        int expectedAmountOfMoves = move.board == move.position? 8 : 9;
        Square nextMoves[TOTAL_SMALL_SQUARES];
        myAssert(generateMoves(bitBoard, nextMoves) == expectedAmountOfMoves);
        revertToCheckpoint(bitBoard);
    }
}


Square to_our_notation(Square rowAndColumn) {
    uint8_t row = rowAndColumn.board;
    uint8_t column = rowAndColumn.position;
    uint8_t board = 3 * (row / 3) + (column / 3);
    uint8_t position = 3 * (row % 3) + (column % 3);
    Square result = {board, position};
    return result;
}


// https://www.codingame.com/replay/296363090
void reCurseVsDaFish() {
    BitBoard* bitBoard = createBitBoard();
    Square playedMoves[49] = {
            {4, 4}, {5, 3}, {6, 2}, {1, 7}, {5, 4}, {8, 3}, {8, 0}, {8, 1}, {7, 3}, {4, 2}, {3, 7}, {1, 5}, {5, 7},
            {7, 5}, {4, 7}, {3, 4}, {0, 5}, {1, 6}, {3, 2}, {1, 8}, {3, 3}, {2, 0}, {7, 1}, {5, 5}, {7, 7}, {4, 5},
            {1, 4}, {3, 5}, {2, 3}, {4, 0}, {4, 1}, {1, 0}, {3, 0}, {2, 2}, {7, 6}, {5, 2}, {6, 6}, {0, 0}, {8, 5},
            {8, 6}, {7, 4}, {8, 7}, {8, 4}, {6, 3}, {7, 8}, {6, 4}, {6, 5}, {5, 0}, {3, 1}
    };
    int possibleMoves[49] = {
            81, 8, 9, 9, 7, 9, 8, 7, 8, 9, 9, 9, 8,
            7, 7, 6, 8, 8, 8, 7, 49, 9, 6, 4, 9, 3,
            38, 2, 35, 29, 6, 27, 5, 7, 8, 4, 7, 6, 15,
            6, 13, 12, 4, 3, 9, 5, 4, 3, 2
    };
    Square ignoredGeneratedMoves[TOTAL_SMALL_SQUARES];
    for (int i = 0; i < 49; i++) {
        myAssert(generateMoves(bitBoard, ignoredGeneratedMoves) == possibleMoves[i]);
        Square move = to_our_notation(playedMoves[i]);
        myAssert(makeMove(bitBoard, move) == (i != 48? NONE : WIN_P1));
    }
    freeBitBoard(bitBoard);
}


void runBitBoardTests() {
    BitBoard* bitBoard = createBitBoard();
    printf("\tanyMoveAllowedOnEmptyBoard...\n");
    anyMoveAllowedOnEmptyBoard(bitBoard);
    printf("\tnineOrEightMovesAllowedAfterFirstMove...\n");
    nineOrEightMovesAllowedAfterFirstMove(bitBoard);
    printf("\treCurseVsDaFish...\n");
    reCurseVsDaFish();
    freeBitBoard(bitBoard);
}