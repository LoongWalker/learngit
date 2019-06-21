include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(DGAnalysis SHARED
	${CMAKE_SOURCE_DIR}/include/dg/analysis/Offset.h
	${CMAKE_SOURCE_DIR}/include/dg/ADT/DGContainer.h
	${CMAKE_SOURCE_DIR}/include/dg/ADT/Bitvector.h
	${CMAKE_SOURCE_DIR}/include/dg/ADT/Bits.h
	${CMAKE_SOURCE_DIR}/include/dg/ADT/NumberSet.h

	analysis/Offset.cpp
)

add_library(PTA SHARED
	${CMAKE_SOURCE_DIR}/include/dg/analysis/SubgraphNode.h
	${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/Pointer.h
	${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointsToSet.h
	${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/MemoryObject.h
	${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointerSubgraph.h
	${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointerAnalysis.h
	${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointerAnalysisFI.h
	${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointerAnalysisFS.h
	${CMAKE_SOURCE_DIR}/include/dg/analysis/PointsTo/PointerSubgraphValidator.h

	analysis/PointsTo/Pointer.cpp
	analysis/PointsTo/PointerAnalysis.cpp
	analysis/PointsTo/PointerSubgraphValidator.cpp
)
target_link_libraries(PTA PUBLIC DGAnalysis)

add_library(RD SHARED
	${CMAKE_SOURCE_DIR}/include/dg/analysis/ReachingDefinitions/ReachingDefinitions.h
	${CMAKE_SOURCE_DIR}/include/dg/analysis/ReachingDefinitions/RDMap.h

	analysis/ReachingDefinitions/Srg/MarkerSRGBuilderFI.h
	analysis/ReachingDefinitions/Srg/MarkerSRGBuilderFS.h

	analysis/ReachingDefinitions/BasicRDMap.cpp
	analysis/ReachingDefinitions/ReachingDefinitions.cpp
	analysis/ReachingDefinitions/Srg/SemisparseRda.cpp
	analysis/ReachingDefinitions/Srg/MarkerSRGBuilderFI.cpp
	analysis/ReachingDefinitions/Srg/MarkerSRGBuilderFS.cpp
)
target_link_libraries(RD PUBLIC DGAnalysis)


if (LLVM_DG)

add_library(LLVMpta STATIC #SHARED
	${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/PointsTo/PointerAnalysis.h
	${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/PointsTo/LLVMPointerAnalysisOptions.h
	${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/PointsTo/PointerSubgraph.h

	${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/PointerSubgraphValidator.h
	${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/PointerSubgraph.cpp
	${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/PointerSubgraphValidator.cpp
	${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/Structure.cpp
	${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/Globals.cpp
	${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/Constants.cpp
	${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/Instructions.cpp
	${CMAKE_SOURCE_DIR}/lib/llvm/analysis/PointsTo/Calls.cpp
)

target_link_libraries(LLVMpta
        PUBLIC PTA
#        PRIVATE LLVMCore
		)

add_library(LLVMrd STATIC #SHARED
	llvm/analysis/ReachingDefinitions/LLVMRDBuilderSemisparse.cpp
	llvm/analysis/ReachingDefinitions/LLVMRDBuilderDense.cpp
	llvm/analysis/ReachingDefinitions/LLVMReachingDefinitions.cpp

	${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/ReachingDefinitions/ReachingDefinitions.h
	llvm/analysis/ReachingDefinitions/LLVMRDBuilder.h
	llvm/analysis/ReachingDefinitions/LLVMRDBuilderSemisparse.h
	llvm/analysis/ReachingDefinitions/LLVMRDBuilderDense.h
)
target_link_libraries(LLVMrd
        PUBLIC LLVMpta
        PUBLIC RD
        )

add_library(ThreadRegions STATIC "") #SHARED "")
include(${CMAKE_CURRENT_SOURCE_DIR}/llvm/analysis/ThreadRegions/CMakeLists.txt)
target_include_directories(ThreadRegions
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/ThreadRegions)

target_link_libraries(ThreadRegions
        INTERFACE LLVMpta
        PRIVATE LLVMrd
        )

add_library(LLVMdg STATIC #SHARED
	${CMAKE_SOURCE_DIR}/include/dg/BBlock.h
	${CMAKE_SOURCE_DIR}/include/dg/Node.h
	${CMAKE_SOURCE_DIR}/include/dg/DependenceGraph.h
	${CMAKE_SOURCE_DIR}/include/dg/llvm/LLVMNode.h
	${CMAKE_SOURCE_DIR}/include/dg/llvm/LLVMDependenceGraph.h
	${CMAKE_SOURCE_DIR}/include/dg/llvm/LLVMDependenceGraphBuilder.h
	${CMAKE_SOURCE_DIR}/include/dg/llvm/LLVMSlicer.h
	${CMAKE_SOURCE_DIR}/include/dg/llvm/analysis/DefUse/DefUse.h

	llvm/LLVMDGVerifier.h
	llvm/llvm-utils.h

	llvm/LLVMNode.cpp
	llvm/LLVMDependenceGraph.cpp
	llvm/LLVMDGVerifier.cpp
	llvm/analysis/Dominators/PostDominators.cpp
	llvm/analysis/DefUse/DefUse.cpp
)

target_link_libraries(LLVMdg
#        PRIVATE LLVMAnalysis
        PRIVATE LLVMpta
        PRIVATE LLVMrd
        PRIVATE ThreadRegions
        )

install(TARGETS
        #LLVMdg
        #ThreadRegions
        #LLVMpta
        #LLVMrd
        PTA RD DGAnalysis
		RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
#	LIBRARY DESTINATION

endif(LLVM_DG)
