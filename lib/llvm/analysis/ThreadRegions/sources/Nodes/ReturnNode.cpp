#include "../../include/Nodes/ReturnNode.h"

ReturnNode::ReturnNode(const llvm::Instruction *instruction):Node(NodeType::RETURN, instruction)
{}
