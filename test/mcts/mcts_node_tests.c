#include <stdio.h>
#include <math.h>
#include "mcts_node_tests.h"
#include "../../src/mcts/mcts_node.h"
#include "../test_util.h"
#include "../../src/nn/forward.h"


void rootIsLeafNode() {
    Board* board = createBoard();
    int rootIndex = createMCTSRootNode(board);
    myAssert(isLeafNode(rootIndex, board));
    freeBoard(board);
}


void between17And81MovesInOneGame() {
    Board* board = createBoard();
    int nodeIndex = createMCTSRootNode(board);
    discoverChildNodes(nodeIndex, board);
    int count = 0;
    MCTSNode* node = &board->nodes[nodeIndex];
    while (node->numChildren > 0) {
        nodeIndex = selectNextChild(board, nodeIndex);
        node = &board->nodes[nodeIndex];
        visitNode(nodeIndex, board);
        discoverChildNodes(nodeIndex, board);
        count++;
    }
    myAssert(count >= 17 && count <= 81);
    freeBoard(board);
}


void updateRootTest() {
    Board* board = createBoard();
    int rootIndex = createMCTSRootNode(board);
    MCTSNode* root = &board->nodes[rootIndex];
    discoverChildNodes(rootIndex, board);
    int parentIndices[1] = {-1};
    backpropagate(board, root->childrenIndex + 0, DRAW, board->state.currentPlayer, parentIndices);
    Square square = getMostPromisingMove(board, root);
    makePermanentMove(board, square);
    int newRootIndex = updateRoot(root, board, square);
    MCTSNode* newRoot = &board->nodes[newRootIndex];
    discoverChildNodes(newRootIndex, board);
    backpropagate(board, newRoot->childrenIndex + 0, DRAW, board->state.currentPlayer, parentIndices);
    myAssert(getMostPromisingMove(board, newRoot).board == square.position);
    freeBoard(board);
}


void updateRootStillWorksWhenPlayedMoveWasPruned() {
    Board* board = createBoard();
    int rootIndex = createMCTSRootNode(board);
    MCTSNode* root = &board->nodes[rootIndex];
    Square firstMove = {4, 4};
    discoverChildNodes(rootIndex, board);
    rootIndex = updateRoot(root, board, firstMove);
    root = &board->nodes[rootIndex];
    makePermanentMove(board, firstMove);
    discoverChildNodes(rootIndex, board);
    root->numChildren = 1;  // 'prune' the other 8 moves
    Square prunedMove = {4, 5};
    myAssert(!squaresAreEqual(board->nodes[root->childrenIndex].square, prunedMove));
    updateRoot(root, board, prunedMove);
    freeBoard(board);
}


void alwaysPlays44WhenGoingFirst() {
    Board* board = createBoard();
    int rootIndex = createMCTSRootNode(board);
    board->me = PLAYER1;
    discoverChildNodes(rootIndex, board);
    Square expected = {4, 4};
    myAssert(squaresAreEqual(getMostPromisingMove(board, &board->nodes[rootIndex]), expected));
    freeBoard(board);
}


void optimizedNNEvalTest() {
    Board* board = createBoard();
    int rootIndex = createMCTSRootNode(board);
    MCTSNode* root = &board->nodes[rootIndex];
    discoverChildNodes(rootIndex, board);
    for (int i = 0; i < root->numChildren; i++) {
        MCTSNode* child = &board->nodes[root->childrenIndex + i];
        makeTemporaryMove(board, child->square);
        float expectedEval = neuralNetworkEval(board);
        float actualEval = child->eval;
        myAssert(fabsf(expectedEval - actualEval) < 1e-4);
        revertToCheckpoint(board);
    }
    freeBoard(board);
}


void runMCTSNodeTests() {
    printf("\trootIsLeafNode...\n");
    rootIsLeafNode();
    printf("\tbetween17And81MovesInOneGame...\n");
    between17And81MovesInOneGame();
    printf("\tupdateRootTest...\n");
    updateRootTest();
    printf("\tupdateRootStillWorksWhenPlayedMoveWasPruned...\n");
    updateRootStillWorksWhenPlayedMoveWasPruned();
    printf("\talwaysPlays44WhenGoingFirst...\n");
    alwaysPlays44WhenGoingFirst();
    printf("\toptimizedNNEvalTest...\n");
    optimizedNNEvalTest();
}