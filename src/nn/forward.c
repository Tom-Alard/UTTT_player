#pragma GCC optimize("Ofast", "unroll-loops", "omit-frame-pointer")
#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
#pragma GCC target("avx2", "fma")


#include <stdalign.h>
#include "forward.h"
#include "clipped_relu.h"
#include "linear.h"

#define HIDDEN_NEURONS 256


void addHiddenWeights(int i, int16_t* restrict output) {
    for (int j = 0; j < HIDDEN_NEURONS; j++) {
        output[j] = (int16_t) (output[j] + hiddenWeights[i][j]);
    }
}


void handlePlayerInput(PlayerBitBoard* playerBitBoard, bool isCurrentPlayer, __m256i regs[16]) {
    uint16_t bigBoard = playerBitBoard->bigBoard;
    int bigBoardOffset = isCurrentPlayer? 0 : 90;
    while (bigBoard) {
        addFeature(__builtin_ffs(bigBoard) - 1 + bigBoardOffset, regs);
        bigBoard &= bigBoard - 1;
    }
    int smallBoardOffset = isCurrentPlayer? 9 : 99;
    int64_t lowBits = (int64_t) playerBitBoard->marks;
    int64_t highBits = (int64_t) (playerBitBoard->marks >> 64);
    while (lowBits) {
        addFeature(__builtin_ffsl(lowBits) - 1 + smallBoardOffset, regs);
        lowBits &= lowBits - 1;
    }
    while (highBits) {
        addFeature(__builtin_ffsl(highBits) - 1 + smallBoardOffset + 64, regs);
        highBits &= highBits - 1;
    }
}


void boardToInput(Board* board, int16_t* restrict output) {
    __m256i regs[16];
    for (int i = 0; i < 16; i++) {
        regs[i] = _mm256_load_si256((__m256i*) &hiddenBiases[i * 16]);
    }

    PlayerBitBoard* p1 = &board->state.player1;
    PlayerBitBoard* currentPlayer = p1 + board->state.currentPlayer;
    PlayerBitBoard* otherPlayer = p1 + !board->state.currentPlayer;
    handlePlayerInput(currentPlayer, true, regs);
    handlePlayerInput(otherPlayer, false, regs);

    for (int i = 0; i < 16; i++) {
        _mm256_store_si256((__m256i*) &output[i * 16], regs[i]);
    }
}


float neuralNetworkEvalFromHidden(__m256i regs[16]) {
    applyClippedReLU256(regs);
    float x = applyLinear256_1(regs) + outputBias + 0.5f;
    return x < 0? 0 : x > 1? 1 : x;
}


float neuralNetworkEval(Board* board) {
    alignas(32) int16_t input[HIDDEN_NEURONS];
    boardToInput(board, input);
    addHiddenWeights(board->state.currentBoard + 180, input);
    __m256i regs[16];
    for (int j = 0; j < 16; j++) {
        regs[j] = _mm256_load_si256((__m256i*) &input[j * 16]);
    }
    return neuralNetworkEvalFromHidden(regs);
}
