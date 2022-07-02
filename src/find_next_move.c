#include <time.h>
#include <stdio.h>
#include "find_next_move.h"
#include "util.h"


MCTSNode* selectLeaf(BitBoard* board, MCTSNode* root) {
    MCTSNode* currentNode = root;
    while (!isLeafNode(currentNode) && hasChildren(currentNode, board)) {
        currentNode = selectNextChild(currentNode, board);
        assertMsg(currentNode != NULL, "selectLeaf: currentNode is NULL!");
        visitNode(currentNode, board);
    }
    return currentNode;
}


MCTSNode* expandLeaf(BitBoard* board, MCTSNode* leaf) {
    MCTSNode* nextChild = selectNextChild(leaf, board);
    visitNode(nextChild, board);
    return nextChild;
}


Winner simulate(BitBoard* board, pcg32_random_t* rng) {
    while (getWinner(board) == NONE) {
        Square validMoves[TOTAL_SMALL_SQUARES];
        int amountOfMoves = generateMoves(board, validMoves);
        Square move = validMoves[pcg32_boundedrand_r(rng, amountOfMoves)];
        makeMove(board, move);
    }
    Winner simulationWinner = getWinner(board);
    revertToCheckpoint(board);
    return simulationWinner;
}


clock_t getDeadline(double time) {
    return clock() + (clock_t)(time*CLOCKS_PER_SEC);
}


bool hasTimeRemaining(clock_t deadline) {
    return clock() < deadline;
}


Square findNextMove(BitBoard* board, MCTSNode* root, double allocatedTime, pcg32_random_t* rng) {
    clock_t deadline = getDeadline(allocatedTime);
    while (hasTimeRemaining(deadline)) {
        MCTSNode* leaf = selectLeaf(board, root);
        MCTSNode* playoutNode;
        Winner simulationWinner;
        Winner winner = getWinner(board);
        if (winner == NONE) {
            playoutNode = expandLeaf(board, leaf);
            simulationWinner = simulate(board, rng);
        } else {
            playoutNode = leaf;
            simulationWinner = winner;
            setNodeWinner(playoutNode, winner);
            revertToCheckpoint(board);
        }
        backpropagate(playoutNode, simulationWinner);
    }
    return getMostSimulatedChildSquare(root);
}
