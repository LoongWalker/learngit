#include "../../include/Nodes/UnlockNode.h"

UnlockNode::UnlockNode(const llvm::Instruction *instruction):Node(NodeType::UNLOCK, instruction)
{}
