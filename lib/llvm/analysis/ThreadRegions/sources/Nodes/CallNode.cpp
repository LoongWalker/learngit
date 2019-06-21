#include "../../include/Nodes/CallNode.h"

CallNode::CallNode(const llvm::Instruction *instruction):Node(NodeType::CALL, instruction)
{}
