#include <llvm/IR/Instructions.h>

#include "../../include/Nodes/ForkNode.h"
#include "../../include/Nodes/EntryNode.h"
#include "../../include/Nodes/JoinNode.h"
#include "dg/llvm/analysis/ThreadRegions/ControlFlowGraph.h"

#include <iostream>

using namespace std;

ForkNode::ForkNode(const llvm::Instruction *instruction):Node(NodeType::FORK, instruction) {}

bool ForkNode::addCorrespondingJoin(JoinNode *joinNode){
    if (!joinNode) {
        return false;
    }
    correspondingJoins_.insert(joinNode);
    return joinNode->correspondingForks_.insert(this).second;
}

bool ForkNode::addForkSuccessor(EntryNode *entryNode) {
    if (!entryNode) {
        return false;
    }
    forkSuccessors_.insert(entryNode);
    return entryNode->forkPredecessors_.insert(this).second;
}

bool ForkNode::removeForkSuccessor(EntryNode *entryNode) {
    if (!entryNode) {
        return false;
    }
    forkSuccessors_.erase(entryNode);
    return entryNode->forkPredecessors_.erase(this);
}

const std::set<EntryNode *> &ForkNode::forkSuccessors() const {
    return forkSuccessors_;
}

std::set<EntryNode *> ForkNode::forkSuccessors() {
    return forkSuccessors_;
}

size_t ForkNode::successorsNumber() const {
    return successors().size() + forkSuccessors_.size();
}

const std::set<JoinNode *> &ForkNode::correspondingJoins() const {
    return correspondingJoins_;
}

std::set<JoinNode *> ForkNode::correspondingJoins() {
    return correspondingJoins_;
}


void ForkNode::printOutcomingEdges(ostream &ostream) const {
    Node::printOutcomingEdges(ostream);
    for (const auto &forkSuccessor : forkSuccessors_) {
        ostream << this->dotName() << " -> " << forkSuccessor->dotName() << " [style=dashed]\n";
    }
}
