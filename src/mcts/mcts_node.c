#include <assert.h>
#include <string.h>
#include <immintrin.h>
#include <stdalign.h>
#include "mcts_node.h"
#include "../misc/util.h"
#include "../nn/forward.h"


int createMCTSRootNode(Board* board) {
    int rootIndex = allocateNodes(board, 1);
    MCTSNode* root = &board->nodes[rootIndex];
    root->parentIndex = -1;
    root->childrenIndex = -1;
    root->eval = 0.0f;
    root->sims = 0.0f;
    root->square.board = 9;
    root->square.position = 9;
    root->amountOfChildren = -1;
    return rootIndex;
}


void initializeMCTSNode(int parentIndex, Square square, float eval, MCTSNode* node) {
    node->parentIndex = parentIndex;
    node->childrenIndex = -1;
    node->eval = eval;
    node->sims = 0.0f;
    node->square = square;
    node->amountOfChildren = -1;
}


float getEvalOfMove(Board* board, Square square) {
    State temp = board->stateCheckpoint;
    updateCheckpoint(board);
    makeTemporaryMove(board, square);
    float eval = neuralNetworkEval(board);
    revertToCheckpoint(board);
    board->stateCheckpoint = temp;
    return eval;
}


void singleChild(int nodeIndex, Board* board, Square square) {
    MCTSNode* node = &board->nodes[nodeIndex];
    node->amountOfChildren = 1;
    node->childrenIndex = allocateNodes(board, 1);
    float eval = getEvalOfMove(board, square);
    initializeMCTSNode(nodeIndex, square, eval, &board->nodes[node->childrenIndex + 0]);
}


bool handleSpecialCases(int nodeIndex, Board* board) {
    if (nextBoardIsEmpty(board) && getPly(board) <= 20) {
        uint8_t currentBoard = getCurrentBoard(board);
        Square sameBoard = {currentBoard, currentBoard};
        singleChild(nodeIndex, board, sameBoard);
        return true;
    }
    if (currentPlayerIsMe(board) && getPly(board) == 0) {
        Square bestFirstMove = {4, 4};
        singleChild(nodeIndex, board, bestFirstMove);
        return true;
    }
    return false;
}


bool isBadMove(Board* board, Square square) {
    bool sendsToDecidedBoard = (board->state.player1.bigBoard | board->state.player2.bigBoard) & (1 << square.position);
    return sendsToDecidedBoard && getPly(board) <= 30;
}


void initializeChildNodes(int parentIndex, Board* board, Square* moves) {
    MCTSNode* parent = &board->nodes[parentIndex];
    alignas(32) int16_t NNInputs[256];
    board->state.currentPlayer ^= 1;
    setHidden(board, NNInputs);
    board->state.currentPlayer ^= 1;
    int8_t amountOfMoves = parent->amountOfChildren;
    PlayerBitBoard* p1 = &board->state.player1;
    PlayerBitBoard* currentPlayerBitBoard = p1 + board->state.currentPlayer;
    PlayerBitBoard* otherPlayerBitBoard = p1 + !board->state.currentPlayer;
    int childIndex = 0;
    __m256i regs[16];
    for (int i = 0; i < amountOfMoves; i++) {
        Square move = moves[i];
        if (isBadMove(board, move) && parent->amountOfChildren > 1) {
            parent->amountOfChildren--;
            continue;
        }
        for (int j = 0; j < 16; j++) {
            regs[j] = _mm256_load_si256((__m256i*) &NNInputs[j * 16]);
        }
        MCTSNode* child = &board->nodes[parent->childrenIndex + childIndex++];
        addFeature(move.position + 99 + 9*move.board, regs);
        uint16_t smallBoard = currentPlayerBitBoard->smallBoards[move.board];
        BIT_SET(smallBoard, move.position);
        bool smallBoardIsDecided;
        if (isWin(smallBoard)) {
            smallBoardIsDecided = BIT_CHECK(board->state.player1.bigBoard | board->state.player2.bigBoard
                                            | (1 << move.board), move.position);
            addFeature(move.board + 90, regs);
        } else if (isDraw(smallBoard, otherPlayerBitBoard->smallBoards[move.board])) {
            smallBoardIsDecided = BIT_CHECK(board->state.player1.bigBoard | board->state.player2.bigBoard
                                            | (1 << move.board), move.position);
            addFeature(move.board, regs);
            addFeature(move.board + 90, regs);
        } else {
            smallBoardIsDecided = BIT_CHECK(board->state.player1.bigBoard | board->state.player2.bigBoard, move.position);
        }
        addFeature((smallBoardIsDecided? ANY_BOARD : move.position) + 180, regs);
        alignas(32) int16_t result[256];
        for (int j = 0; j < 16; j++) {
            _mm256_store_si256((__m256i*) &result[j * 16], regs[j]);
        }
        float eval = neuralNetworkEvalFromHidden(result);
        initializeMCTSNode(parentIndex, move, eval, child);
    }
}


void discoverChildNodes(int nodeIndex, Board* board) {
    MCTSNode* node = &board->nodes[nodeIndex];
    if (node->amountOfChildren == -1 && !handleSpecialCases(nodeIndex, board)) {
        Square movesArray[TOTAL_SMALL_SQUARES];
        int8_t amountOfMoves;
        Square* moves = generateMoves(board, movesArray, &amountOfMoves);
        if (getPly(board) > 30) {
            Player player = getCurrentPlayer(board);
            for (int i = 0; i < amountOfMoves; i++) {
                if (getWinnerAfterMove(board, moves[i]) == player + 1) {
                    singleChild(nodeIndex, board, moves[i]);
                    return;
                }
            }
        }
        node->amountOfChildren = amountOfMoves;
        node->childrenIndex = allocateNodes(board, amountOfMoves);
        initializeChildNodes(nodeIndex, board, moves);
    }
}


bool isLeafNode(int nodeIndex, Board* board) {
    return (&board->nodes[nodeIndex])->sims == 0;
}


float fastSquareRoot(float x) {
    __m128 in = _mm_set_ss(x);
    return _mm_cvtss_f32(_mm_mul_ss(in, _mm_rsqrt_ss(in)));
}


#define EXPLORATION_PARAMETER 0.41f
#define FIRST_PLAY_URGENCY 0.40f
float getUCTValue(MCTSNode* node, float parentLogSims) {
    float exploitation = node->eval;
    float exploration = node->sims == 0? FIRST_PLAY_URGENCY : fastSquareRoot(parentLogSims / node->sims);
    return exploitation + exploration;
}


// From: https://github.com/etheory/fastapprox/blob/master/fastapprox/src/fastlog.h
float fastLog2(float x) {
    union { float f; uint32_t i; } vx = { x };
    float y = (float)vx.i;
    y *= 1.1920928955078125e-7f;
    return y - 126.94269504f;
}


int selectNextChild(Board* board, int nodeIndex) {
    MCTSNode* node = &board->nodes[nodeIndex];
    assert(node->amountOfChildren > 0);
    float logSims = EXPLORATION_PARAMETER*EXPLORATION_PARAMETER * fastLog2(node->sims);
    int highestUCTChildIndex = -1;
    float highestUCT = -1.0f;
    for (int i = 0; i < node->amountOfChildren; i++) {
        int childIndex = node->childrenIndex + i;
        float UCT = getUCTValue(&board->nodes[childIndex], logSims);
        if (UCT > highestUCT) {
            highestUCTChildIndex = childIndex;
            highestUCT = UCT;
        }
    }
    assert(highestUCTChildIndex != -1);
    return highestUCTChildIndex;
}


MCTSNode* expandLeaf(int leafIndex, Board* board) {
    assert(isLeafNode(leafIndex, board));
    discoverChildNodes(leafIndex, board);
    return &board->nodes[selectNextChild(board, leafIndex)];
}


int updateRoot(MCTSNode* root, Board* board, Square square) {
    MCTSNode* newRoot = NULL;
    for (int i = 0; i < root->amountOfChildren; i++) {
        MCTSNode* child = &board->nodes[root->childrenIndex + i];
        if (squaresAreEqual(square, child->square)) {
            newRoot = child;
            int newRootIndex = root->childrenIndex + i;
            newRoot->parentIndex = -1;
            return newRootIndex;
        }
    }

    Square movesArray[TOTAL_SMALL_SQUARES];
    int8_t amountOfMoves;
    Square* moves = generateMoves(board, movesArray, &amountOfMoves);
    for (int i = 0; i < amountOfMoves; i++) {
        if (squaresAreEqual(moves[i], square)) {
            int newRootIndex = allocateNodes(board, 1);
            newRoot = &board->nodes[newRootIndex];
            initializeMCTSNode(-1, square, 0.0f, newRoot);
            return newRootIndex;
        }
    }
    exit(123);
}


void backpropagate(Board* board, int nodeIndex, Winner winner, Player player) {
    assert(winner != NONE && "backpropagate: Can't backpropagate a NONE Winner");
    MCTSNode* node = &board->nodes[nodeIndex];
    node->eval = winner == DRAW ? 0.5f : player + 1 == winner ? 1.0f : 0.0f;
    backpropagateEval(board, node);
}


void backpropagateEval(Board* board, MCTSNode* node) {
    assert(node != NULL);
    MCTSNode* currentNode = node;
    if (currentNode->amountOfChildren <= 0) {
        currentNode = &board->nodes[currentNode->parentIndex];
    }
    while (currentNode != NULL) {
        float maxChildEval = 0.0f;
        for (int i = 0; i < currentNode->amountOfChildren; i++) {
            float eval = (&board->nodes[currentNode->childrenIndex + i])->eval;
            maxChildEval = maxChildEval >= eval? maxChildEval : eval;
        }
        currentNode->eval = 1 - maxChildEval;
        currentNode->sims++;
        currentNode = currentNode->parentIndex == -1? NULL : &board->nodes[currentNode->parentIndex];
    }
}


void visitNode(int nodeIndex, Board* board) {
    makeTemporaryMove(board, (&board->nodes[nodeIndex])->square);
}


Square getMostPromisingMove(Board* board, MCTSNode* node) {
    assert(node->amountOfChildren > 0 && "getMostPromisingMove: node has no children");
    MCTSNode* highestScoreChild = &board->nodes[node->childrenIndex + 0];
    float highestScore = getEval(highestScoreChild) + fastLog2(highestScoreChild->sims);
    for (int i = 1; i < node->amountOfChildren; i++) {
        MCTSNode* child = &board->nodes[node->childrenIndex + i];
        float score = getEval(child) + fastLog2(child->sims);
        if (score > highestScore) {
            highestScoreChild = child;
            highestScore = score;
        }
    }
    return highestScoreChild->square;
}


float getEval(MCTSNode* node) {
    return node->eval;
}
