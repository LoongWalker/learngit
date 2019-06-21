include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/include)

if (LLVM_DG)
	# generate a git-version.h with a HEAD commit hash tag
	# (if it changed)
#	add_custom_target(
#		gitversion ALL sh ${CMAKE_CURRENT_SOURCE_DIR}/git-version.sh
#	)
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	add_executable(llvm-dg-dump llvm-dg-dump.cpp)
	target_link_libraries(llvm-dg-dump
				PRIVATE LLVMdg
				PRIVATE ${llvm_support}
				PRIVATE ${llvm_analysis}
				PRIVATE ${llvm_irreader}
				PRIVATE ${llvm_bitwriter}
				PRIVATE ${llvm_core})

	add_executable(llvm-slicer llvm-slicer.cpp
			llvm-slicer-opts.cpp llvm-slicer-opts.h
			llvm-slicer-utils.cpp llvm-slicer-utils.h)
	target_link_libraries(llvm-slicer PRIVATE LLVMdg)
	target_link_libraries(llvm-slicer
			PRIVATE ${llvm_irreader}
			PRIVATE ${llvm_bitwriter}
			PRIVATE ${llvm_analysis}
			PRIVATE ${llvm_support}
			PRIVATE ${llvm_core})

############# TaintCore
	add_executable(TaintCore TaintCore.cpp
			llvm-slicer-opts.cpp llvm-slicer-opts.h
			llvm-slicer-utils.cpp llvm-slicer-utils.h)
	#target_link_libraries(TaintCore PRIVATE LLVMdg)
	target_link_libraries(TaintCore
            PRIVATE LLVMdg
			PRIVATE ${llvm_irreader}
			PRIVATE ${llvm_bitwriter}
			PRIVATE ${llvm_analysis}
			PRIVATE ${llvm_support}
			PRIVATE ${llvm_core})
#############

#	add_executable(llvm-slicer llvm-slicer.cpp
#				   llvm-slicer-opts.cpp llvm-slicer-opts.h
#				   llvm-slicer-utils.cpp llvm-slicer-utils.h

#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/Offset.h
#            ${CMAKE_SOURCE_DIR}/include/dg/ADT/DGContainer.h
#            ${CMAKE_SOURCE_DIR}/include/dg/ADT/Bitvector.h
#            ${CMAKE_SOURCE_DIR}/include/dg/ADT/Bits.h
#            ${CMAKE_SOURCE_DIR}/include/dg/ADT/NumberSet.h

#            ${CMAKE_SOURCE_DIR}/lib/analysis/Offset.cpp
##########################
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/SubgraphNode.h
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/Pointer.h
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointsToSet.h
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/MemoryObject.h
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointerSubgraph.h
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointerAnalysis.h
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointerAnalysisFI.h
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointerAnalysisFS.h
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointerSubgraphValidator.h

#            ${CMAKE_SOURCE_DIR}/lib/analysis/PointsTo/Pointer.cpp
#            ${CMAKE_SOURCE_DIR}/lib/analysis/PointsTo/PointerAnalysis.cpp
#            ${CMAKE_SOURCE_DIR}/lib/analysis/PointsTo/PointerSubgraphValidator.cpp
##########################
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/ReachingDefinitions/ReachingDefinitions.h
#            ${CMAKE_SOURCE_DIR}/include/dg/analysis/ReachingDefinitions/RDMap.h

#            ${CMAKE_SOURCE_DIR}/lib/analysis/ReachingDefinitions/Srg/MarkerSRGBuilderFI.h
#            ${CMAKE_SOURCE_DIR}/lib/analysis/ReachingDefinitions/Srg/MarkerSRGBuilderFS.h

#            ${CMAKE_SOURCE_DIR}/lib/analysis/ReachingDefinitions/BasicRDMap.cpp
#            ${CMAKE_SOURCE_DIR}/lib/analysis/ReachingDefinitions/ReachingDefinitions.cpp
#            ${CMAKE_SOURCE_DIR}/lib/analysis/ReachingDefinitions/Srg/SemisparseRda.cpp
#            ${CMAKE_SOURCE_DIR}/lib/analysis/ReachingDefinitions/Srg/MarkerSRGBuilderFI.cpp
#            ${CMAKE_SOURCE_DIR}/lib/analysis/ReachingDefinitions/Srg/MarkerSRGBuilderFS.cpp
##########################
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/PointsTo/PointerAnalysis.h
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/PointsTo/LLVMPointerAnalysisOptions.h
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/PointsTo/PointerSubgraph.h

#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/PointerSubgraphValidator.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/PointerSubgraph.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/PointerSubgraphValidator.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/Structure.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/Globals.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/Constants.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/Instructions.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/Calls.cpp
##########################
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ReachingDefinitions/LLVMRDBuilderSemisparse.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ReachingDefinitions/LLVMRDBuilderDense.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ReachingDefinitions/LLVMReachingDefinitions.cpp

#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/ReachingDefinitions/ReachingDefinitions.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ReachingDefinitions/LLVMRDBuilder.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ReachingDefinitions/LLVMRDBuilderSemisparse.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ReachingDefinitions/LLVMRDBuilderDense.h
##########################
#            ${CMAKE_SOURCE_DIR}/include/dg/BBlock.h
#            ${CMAKE_SOURCE_DIR}/include/dg/Node.h
#            ${CMAKE_SOURCE_DIR}/include/dg/DependenceGraph.h
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/LLVMNode.h
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/LLVMDependenceGraph.h
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/LLVMDependenceGraphBuilder.h
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/LLVMSlicer.h
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/DefUse/DefUse.h

#            ${CMAKE_SOURCE_DIR}/lib/llvm/LLVMDGVerifier.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/llvm-utils.h

#            ${CMAKE_SOURCE_DIR}/lib/llvm/LLVMNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/LLVMDependenceGraph.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/LLVMDGVerifier.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/Dominators/PostDominators.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/DefUse/DefUse.cpp
##########################
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/ThreadRegions/ControlFlowGraph.h
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/ThreadRegions/MayHappenInParallel.h
#            ${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/ThreadRegions/ThreadRegion.h
##########################
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Graphs/BlockGraph.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Graphs/CriticalSectionsBuilder.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Graphs/FunctionGraph.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Graphs/GraphBuilder.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Graphs/ThreadRegionsBuilder.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/CallFuncPtrNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/CallNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/CallReturnNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/EntryNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/ExitNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/ForkNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/GeneralNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/JoinNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/LockNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/Node.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/NodeIterator.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/Nodes.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/ReturnNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/include/Nodes/UnlockNode.h
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Graphs/BlockGraph.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Graphs/FunctionGraph.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Graphs/GraphBuilder.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Graphs/ThreadRegionsBuilder.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Graphs/CriticalSectionsBuilder.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Graphs/ControlFlowGraph.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/Node.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/GeneralNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/ForkNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/JoinNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/LockNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/UnlockNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/EntryNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/ExitNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/CallNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/CallFuncPtrNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/CallReturnNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/ReturnNode.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/Nodes/NodeIterator.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/MayHappenInParallel.cpp
#            ${CMAKE_SOURCE_DIR}/lib/llvm/analysis/ThreadRegions/sources/ThreadRegion.cpp
#            )

#########################################################
#	add_executable(llvm-ps-dump llvm-ps-dump.cpp)
#	target_link_libraries(llvm-ps-dump PRIVATE LLVMpta)
#	target_link_libraries(llvm-ps-dump
#				PRIVATE ${llvm_core}
#				PRIVATE ${llvm_irreader}
#				PRIVATE ${llvm_analysis}
#				PRIVATE ${llvm_support}
#            )
#########################################################

	add_executable(llvm-pta-ben llvm-pta-ben.cpp)
	target_link_libraries(llvm-pta-ben LLVMpta)
	target_link_libraries(llvm-pta-ben
				PRIVATE ${llvm_core}
				PRIVATE ${llvm_irreader}
				PRIVATE ${llvm_analysis}
				PRIVATE ${llvm_support})

	add_executable(llvm-pta-compare llvm-pta-compare.cpp)
	target_link_libraries(llvm-pta-compare PRIVATE LLVMpta)
	target_link_libraries(llvm-pta-compare
				PRIVATE ${llvm_core}
				PRIVATE ${llvm_irreader}
				PRIVATE ${llvm_analysis}
				PRIVATE ${llvm_support})

	add_executable(llvm-rd-dump llvm-rd-dump.cpp)
	target_link_libraries(llvm-rd-dump PRIVATE LLVMrd)
	target_link_libraries(llvm-rd-dump
				PRIVATE ${llvm_core}
				PRIVATE ${llvm_irreader}
				PRIVATE ${llvm_analysis}
				PRIVATE ${llvm_support})

#	add_executable(llvm-vr-dump llvm-vr-dump.cpp)
#	target_link_libraries(llvm-vr-dump
#				PRIVATE ${llvm_core}
#				PRIVATE ${llvm_irreader}
#				PRIVATE ${llvm_analysis}
#				PRIVATE ${llvm_support})

	add_executable(llvm-to-source llvm-to-source.cpp)
	target_link_libraries(llvm-to-source
				PRIVATE ${llvm_core}
				PRIVATE ${llvm_irreader}
				PRIVATE ${llvm_support})

#	add_executable(llvm-thread-regions-dump llvm-thread-regions-dump.cpp)
#    target_link_libraries(llvm-thread-regions-dump PRIVATE ThreadRegions
#				 PRIVATE ${llvm_core}
#                PRIVATE ${llvm_irreader}
#                PRIVATE ${llvm_support})

	install(TARGETS llvm-dg-dump llvm-slicer llvm-to-source
		RUNTIME DESTINATION bin)

	# copy also the wrapper scripts
	configure_file(ps-show ps-show COPYONLY)
	configure_file(rd-show rd-show COPYONLY)
	configure_file(llvmdg-show llvmdg-show COPYONLY)
endif (LLVM_DG)

include_directories(../src)

