#include <string.h>
#include "smart_rollout.h"
#include "util.h"


uint16_t precalculatedInstantWinBoards[512];
uint16_t calculateInstantWinBoards_(uint16_t bigBoard) {
    uint16_t lineAlreadyFilled[8] = {0x7, 0x38, 0x1c0, 0x49, 0x92, 0x124, 0x111, 0x54};
    uint16_t hasInstantWinBoardMasks[24] = {
            3, 5, 6,
            24, 40, 48,
            192, 320, 384,
            9, 65, 72,
            18, 130, 144,
            36, 260, 288,
            17, 257, 272,
            20, 68, 80
    };
    uint16_t instantWinBoard[24] = {
            2, 1, 0,
            5, 4, 3,
            8, 7, 6,
            6, 3, 0,
            7, 4, 1,
            8, 5, 2,
            8, 4, 0,
            6, 4, 2
    };
    uint16_t result = 0;
    for (int i = 0; i < 24; i++) {
        uint16_t m1 = hasInstantWinBoardMasks[i];
        uint16_t m2 = lineAlreadyFilled[i/3];
        if ((bigBoard & m1) == m1 && (bigBoard & m2) != m2) {
            BIT_SET(result, instantWinBoard[i]);
        }
    }
    return result;
}


uint16_t calculateInstantWinBoards(uint16_t bigBoard, uint16_t otherPlayerBigBoard) {
    return precalculatedInstantWinBoards[bigBoard] & ~otherPlayerBigBoard;
}


bool smallBoardHasInstantWinMove(uint16_t smallBoard, uint16_t otherSmallBoard) {
    return (precalculatedInstantWinBoards[smallBoard] & ~otherSmallBoard) != 0;
}


void initializeLookupTable() {
    for (uint16_t bigBoard = 0; bigBoard < 512; bigBoard++) {
        precalculatedInstantWinBoards[bigBoard] = calculateInstantWinBoards_(bigBoard);
    }
}


void initializeRolloutState(RolloutState* RS) {
    memset(RS, 0, sizeof(RolloutState));
}


bool hasWinningMove(RolloutState* RS, uint8_t currentBoard, Player player) {
    uint16_t smallBoardsWithWinningMove = RS->instantWinBoards[player] & RS->instantWinSmallBoards[player];
    return (currentBoard == 9 && smallBoardsWithWinningMove) || BIT_CHECK(smallBoardsWithWinningMove, currentBoard);
}


void updateSmallBoardState(RolloutState* RS, uint8_t boardIndex, uint16_t smallBoard, uint16_t otherSmallBoard, Player player) {
    BIT_CHANGE(RS->instantWinSmallBoards[player], boardIndex, smallBoardHasInstantWinMove(smallBoard, otherSmallBoard));
    BIT_CHANGE(RS->instantWinSmallBoards[OTHER_PLAYER(player)], boardIndex, smallBoardHasInstantWinMove(otherSmallBoard, smallBoard));
}


void updateBigBoardState(RolloutState* RS, uint16_t bigBoard, uint16_t otherBigBoard, Player player) {
    RS->instantWinBoards[player] = calculateInstantWinBoards(bigBoard, otherBigBoard);
    RS->instantWinBoards[OTHER_PLAYER(player)] = calculateInstantWinBoards(otherBigBoard, bigBoard);
}
