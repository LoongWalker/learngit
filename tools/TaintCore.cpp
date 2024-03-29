#include <set>
#include <string>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>

#ifndef HAVE_LLVM
#error "This code needs LLVM enabled"
#endif

#include <llvm/Config/llvm-config.h>

#if (LLVM_VERSION_MAJOR < 3)
#error "Unsupported version of LLVM"
#endif

#include "llvm-slicer.h"
#include "llvm-slicer-opts.h"
#include "llvm-slicer-utils.h"

// ignore unused parameters in LLVM libraries
#if (__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if LLVM_VERSION_MAJOR >= 4
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#else
#include <llvm/Bitcode/ReaderWriter.h>
///2 llvm3.4
#include <llvm/Support/InstIterator.h>
#endif

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/IR/IntrinsicInst.h>

#include <llvm/support/Casting.h>

#if (__clang__)
#pragma clang diagnostic pop // ignore -Wunused-parameter
#else
#pragma GCC diagnostic pop
#endif

#include <iostream>
#include <fstream>

#include "dg/ADT/Queue.h"
#include "dg/llvm/LLVMDG2Dot.h"
#include "llvm/LLVMDGAssemblyAnnotationWriter.h"

using namespace dg;

using llvm::errs;
using dg::analysis::LLVMPointerAnalysisOptions;
using dg::analysis::LLVMReachingDefinitionsAnalysisOptions;

using AnnotationOptsT
= dg::debug::LLVMDGAssemblyAnnotationWriter::AnnotationOptsT;


llvm::cl::opt<bool> should_verify_module("dont-verify",
                                         llvm::cl::desc("Verify sliced module (default=true)."),
                                         llvm::cl::init(true), llvm::cl::cat(SlicingOpts));

llvm::cl::opt<bool> remove_unused_only("remove-unused-only",
                                       llvm::cl::desc("Only remove unused parts of module (default=false)."),
                                       llvm::cl::init(false), llvm::cl::cat(SlicingOpts));

llvm::cl::opt<bool> statistics("statistics",
                               llvm::cl::desc("Print statistics about slicing (default=false)."),
                               llvm::cl::init(false), llvm::cl::cat(SlicingOpts));

llvm::cl::opt<bool> dump_dg("dump-dg",
                            llvm::cl::desc("Dump dependence graph to dot (default=false)."),
                            llvm::cl::init(false), llvm::cl::cat(SlicingOpts));

llvm::cl::opt<bool> dump_dg_only("dump-dg-only",
                                 llvm::cl::desc("Only dump dependence graph to dot,"
                                                        " do not slice the module (default=false)."),
                                 llvm::cl::init(false), llvm::cl::cat(SlicingOpts));

llvm::cl::opt<bool> dump_bb_only("dump-bb-only",
                                 llvm::cl::desc("Only dump basic blocks of dependence graph to dot"
                                                        " (default=false)."),
                                 llvm::cl::init(false), llvm::cl::cat(SlicingOpts));

llvm::cl::opt<std::string> annotationOpts("annotate",
                                          llvm::cl::desc("Save annotated version of module as a text (.ll).\n"
                                                                 "(dd: data dependencies, cd:control dependencies,\n"
                                                                 "rd: reaching definitions, pta: points-to information,\n"
                                                                 "slice: comment out what is going to be sliced away, etc.)\n"
                                                                 "for more options, use comma separated list"),
                                          llvm::cl::value_desc("val1,val2,..."), llvm::cl::init(""),
                                          llvm::cl::cat(SlicingOpts));


// mapping of AllocaInst to the names of C variables
std::map<const llvm::Value *, std::string> valuesToVariables;

class ModuleWriter {
    const SlicerOptions& options;
    llvm::Module *M;

public:
    ModuleWriter(const SlicerOptions& o,
                 llvm::Module *m)
            : options(o), M(m) {}

    int cleanAndSaveModule(bool should_verify_module = true) {
        // remove unneeded parts of the module
        removeUnusedFromModule();

        // fix linkage of declared functions (if needs to be fixed)
        makeDeclarationsExternal();

        return saveModule(should_verify_module);
    }

    int saveModule(bool should_verify_module = true)
    {
        if (should_verify_module)
            return verifyAndWriteModule();
        else
            return writeModule();
    }

    void removeUnusedFromModule()
    {
        bool fixpoint;

        do {
            fixpoint = _removeUnusedFromModule();
        } while (fixpoint);
    }

    // after we slice the LLVM, we somethimes have troubles
    // with function declarations:
    //
    //   Global is external, but doesn't have external or dllimport or weak linkage!
    //   i32 (%struct.usbnet*)* @always_connected
    //   invalid linkage type for function declaration
    //
    // This function makes the declarations external
    void makeDeclarationsExternal()
    {
        using namespace llvm;

        // iterate over all functions in module
        for (auto& F : *M) {
            if (F.size() == 0) {
                // this will make sure that the linkage has right type
                F.deleteBody();
            }
        }
    }

    llvm::Module* getModule(){return M;}

private:
    bool writeModule() {
        // compose name if not given
        std::string fl;
        if (!options.outputFile.empty()) {
            fl = options.outputFile;
        } else {
            fl = options.inputFile;
            replace_suffix(fl, ".sliced");
        }

        // open stream to write to
        std::ofstream ofs(fl);
        llvm::raw_os_ostream ostream(ofs);

        // write the module
        errs() << "INFO: saving sliced module to: " << fl.c_str() << "\n";

#if (LLVM_VERSION_MAJOR > 6)
        llvm::WriteBitcodeToFile(*M, ostream);
#else
        llvm::WriteBitcodeToFile(M, ostream);
#endif

        return true;
    }

    bool verifyModule()
    {
        // the verifyModule function returns false if there
        // are no errors

#if ((LLVM_VERSION_MAJOR >= 4) || (LLVM_VERSION_MINOR >= 5))
        return !llvm::verifyModule(*M, &llvm::errs());
#else
        return !llvm::verifyModule(*M, llvm::PrintMessageAction);
#endif
    }


    int verifyAndWriteModule()
    {
        if (!verifyModule()) {
            errs() << "ERR: Verifying module failed, the IR is not valid\n";
            errs() << "INFO: Saving anyway so that you can check it\n";
            return 1;
        }

        if (!writeModule()) {
            errs() << "Saving sliced module failed\n";
            return 1;
        }

        // exit code
        return 0;
    }

    bool _removeUnusedFromModule()
    {
        using namespace llvm;
        // do not slice away these functions no matter what
        // FIXME do it a vector and fill it dynamically according
        // to what is the setup (like for sv-comp or general..)
        const char *keep[] = {options.dgOptions.entryFunction.c_str(),
                              "klee_assume", nullptr};

        // when erasing while iterating the slicer crashes
        // so set the to be erased values into container
        // and then erase them
        std::set<Function *> funs;
        std::set<GlobalVariable *> globals;
        std::set<GlobalAlias *> aliases;

        for (auto I = M->begin(), E = M->end(); I != E; ++I) {
            Function *func = &*I;
            if (array_match(func->getName(), keep))
                continue;

            // if the function is unused or we haven't constructed it
            // at all in dependence graph, we can remove it
            // (it may have some uses though - like when one
            // unused func calls the other unused func
            if (func->hasNUses(0))
                funs.insert(func);
        }

        for (auto I = M->global_begin(), E = M->global_end(); I != E; ++I) {
            GlobalVariable *gv = &*I;
            if (gv->hasNUses(0))
                globals.insert(gv);
        }

        for (GlobalAlias& ga : M->getAliasList()) {
            if (ga.hasNUses(0))
                aliases.insert(&ga);
        }

        for (Function *f : funs)
            f->eraseFromParent();
        for (GlobalVariable *gv : globals)
            gv->eraseFromParent();
        for (GlobalAlias *ga : aliases)
            ga->eraseFromParent();

        return (!funs.empty() || !globals.empty() || !aliases.empty());
    }

};

static void maybe_print_statistics(llvm::Module *M, const char *prefix = nullptr)
{
    if (!statistics)
        return;

    using namespace llvm;
    uint64_t inum, bnum, fnum, gnum;
    inum = bnum = fnum = gnum = 0;

    for (auto I = M->begin(), E = M->end(); I != E; ++I) {
        // don't count in declarations
        if (I->size() == 0)
            continue;

        ++fnum;

        for (const BasicBlock& B : *I) {
            ++bnum;
            inum += B.size();
        }
    }

    for (auto I = M->global_begin(), E = M->global_end(); I != E; ++I)
        ++gnum;

    if (prefix)
        errs() << prefix;

    errs() << "Globals/Functions/Blocks/Instr.: "
           << gnum << " " << fnum << " " << bnum << " " << inum << "\n";
}

class DGDumper {
    const SlicerOptions& options;
    LLVMDependenceGraph *dg;
    bool bb_only{false};
    uint32_t dump_opts{debug::PRINT_DD | debug::PRINT_CD | debug::PRINT_USE | debug::PRINT_ID};

public:
    DGDumper(const SlicerOptions& opts,
             LLVMDependenceGraph *dg,
             bool bb_only = false,
             uint32_t dump_opts = debug::PRINT_DD | debug::PRINT_CD | debug::PRINT_USE | debug::PRINT_ID)
            : options(opts), dg(dg), bb_only(bb_only), dump_opts(dump_opts) {}

    void dumpToDot(const char *suffix = nullptr) {
        // compose new name
        std::string fl(options.inputFile);
        if (suffix)
            replace_suffix(fl, suffix);
        else
            replace_suffix(fl, ".dot");

        errs() << "INFO: Dumping DG to to " << fl << "\n";

        if (bb_only) {
            debug::LLVMDGDumpBlocks dumper(dg, dump_opts, fl.c_str());
            dumper.dump();
        } else {
            debug::LLVMDG2Dot dumper(dg, dump_opts, fl.c_str());
            dumper.dump();
        }
    }
};

class ModuleAnnotator {
    const SlicerOptions& options;
    LLVMDependenceGraph *dg;
    AnnotationOptsT annotationOptions;

public:
    ModuleAnnotator(const SlicerOptions& o,
                    LLVMDependenceGraph *dg,
                    AnnotationOptsT annotO)
            : options(o), dg(dg), annotationOptions(annotO) {}

    bool shouldAnnotate() const { return annotationOptions != 0; }

    void annotate(const std::set<LLVMNode *> *criteria = nullptr)
    {
        // compose name
        std::string fl(options.inputFile);
        fl.replace(fl.end() - 3, fl.end(), "-debug.ll");

        // open stream to write to
        std::ofstream ofs(fl);
        llvm::raw_os_ostream outputstream(ofs);

        std::string module_comment =
                "; -- Generated by llvm-slicer --\n"
                        ";   * slicing criteria: '" + options.slicingCriteria + "'\n" +
                ";   * secondary slicing criteria: '" + options.secondarySlicingCriteria + "'\n" +
                ";   * forward slice: '" + std::to_string(options.forwardSlicing) + "'\n" +
                ";   * remove slicing criteria: '"
                + std::to_string(options.removeSlicingCriteria) + "'\n" +
                ";   * undefined are pure: '"
                + std::to_string(options.dgOptions.RDAOptions.undefinedArePure) + "'\n" +
                ";   * pointer analysis: ";
        if (options.dgOptions.PTAOptions.analysisType
            == LLVMPointerAnalysisOptions::AnalysisType::fi)
            module_comment += "flow-insensitive\n";
        else if (options.dgOptions.PTAOptions.analysisType
                 == LLVMPointerAnalysisOptions::AnalysisType::fs)
            module_comment += "flow-sensitive\n";
        else if (options.dgOptions.PTAOptions.analysisType
                 == LLVMPointerAnalysisOptions::AnalysisType::inv)
            module_comment += "flow-sensitive with invalidate\n";

        module_comment+= ";   * PTA field sensitivity: ";
        if (options.dgOptions.PTAOptions.fieldSensitivity == Offset::UNKNOWN)
            module_comment += "full\n\n";
        else
            module_comment
                    += std::to_string(*options.dgOptions.PTAOptions.fieldSensitivity)
                       + "\n\n";

        errs() << "INFO: Saving IR with annotations to " << fl << "\n";
        auto annot
                = new dg::debug::LLVMDGAssemblyAnnotationWriter(annotationOptions,
                                                                dg->getPTA(),
                                                                dg->getRDA(),
                                                                criteria);
        annot->emitModuleComment(std::move(module_comment));
        llvm::Module *M = dg->getModule();
        M->print(outputstream, annot);

        delete annot;
    }
};


static bool usesTheVariable(LLVMDependenceGraph& dg,
                            const llvm::Value *v,
                            const std::string& var)
{
    auto ptrNode = dg.getPTA()->getPointsTo(v);
    if (!ptrNode)
        return true; // it may be a definition of the variable, we do not know

    for (const auto& ptr : ptrNode->pointsTo) {
        if (ptr.isUnknown())
            return true; // it may be a definition of the variable, we do not know

        auto alloca = ptr.target->getUserData<llvm::Value>();
        if (!alloca)
            continue;

        if (const llvm::AllocaInst *AI = llvm::dyn_cast<llvm::AllocaInst>(alloca)) {
            auto name = valuesToVariables.find(AI);
            if (name != valuesToVariables.end()) {
                if (name->second == var)
                    return true;
            }
        }
    }

    return false;
}

template <typename InstT>
static bool useOfTheVar(LLVMDependenceGraph& dg,
                        const llvm::Instruction& I,
                        const std::string& var)
{
    // check that we store to that variable
    const InstT *tmp = llvm::dyn_cast<InstT>(&I);
    if (!tmp)
        return false;

    return usesTheVariable(dg, tmp->getPointerOperand(), var);
}

static bool isStoreToTheVar(LLVMDependenceGraph& dg,
                            const llvm::Instruction& I,
                            const std::string& var)
{
    return useOfTheVar<llvm::StoreInst>(dg, I, var);
}

static bool isLoadOfTheVar(LLVMDependenceGraph& dg,
                           const llvm::Instruction& I,
                           const std::string& var)
{
    return useOfTheVar<llvm::LoadInst>(dg, I, var);
}

//ling:call 作切片标准

static bool isCallSite(LLVMDependenceGraph& dg,
                       const llvm::Instruction& I,
                       const std::pair<int, std::string> c)
{
    if(llvm::isa<llvm::CallInst>(I))
    {
        if(auto * i= llvm::dyn_cast<llvm::CallInst>(&I))
        {
            std::string strFuncName;
            llvm::Function* pFunction =  i->getCalledFunction();
            if(pFunction)
                strFuncName = pFunction->getName();
            if(strFuncName == c.second)
            {
                auto& Loc = I.getDebugLoc();
                unsigned int line = Loc.getLine();
                if (line == c.first)
                    return true;
            }
        }
    }

    return false;
    //return useOfTheVar<llvm::CallInst>(dg, I, var);
}

static bool instMatchesCrit(LLVMDependenceGraph& dg,
                            const llvm::Instruction& I,
                            const std::vector<std::pair<int, std::string>>& parsedCrit)
{
    for (const auto& c : parsedCrit) {
        auto& Loc = I.getDebugLoc();
        //if (!Loc)
        if(Loc.isUnknown())
            continue;
        if (static_cast<int>(Loc.getLine()) != c.first)
            continue;

        if (static_cast<int>(Loc.getLine()) != c.first){
            if (isStoreToTheVar(dg, I, c.second) ||
                isLoadOfTheVar(dg, I, c.second)){
                llvm::errs() << "Matched line " << c.first << " with variable "
                             << c.second << " to:\n" << I << "\n";
                return true;
            }
        }
    }

    return false;
}

static bool globalMatchesCrit(const llvm::GlobalVariable& G,
                              const std::vector<std::pair<int, std::string>>& parsedCrit)
{
    for (const auto& c : parsedCrit) {
        if (c.first != -1)
            continue;
        if (c.second == G.getName().str()) {
            llvm::errs() << "Matched global variable "
                         << c.second << " to:\n" << G << "\n";
            return true;
        }
    }

    return false;
}

static inline bool isNumber(const std::string& s) {
    assert(!s.empty());

    for (const auto c : s)
        if (!isdigit(c))
            return false;

    return true;
}

static void getLineCriteriaNodes(LLVMDependenceGraph& dg,
                                 std::vector<std::string>& criteria,
                                 std::set<LLVMNode *>& nodes)
{
    assert(!criteria.empty() && "No criteria given");

    std::vector<std::pair<int, std::string>> parsedCrit;
    for (auto& crit : criteria) {
        auto parts = splitList(crit, ':');
        assert(parts.size() == 2);

        // parse the line number
        if (parts[0].empty()) {
            // global variable
            parsedCrit.emplace_back(-1, parts[1]);
        } else if (isNumber(parts[0])) {
            int line = atoi(parts[0].c_str());
            if (line > 0)
                parsedCrit.emplace_back(line, parts[1]);
        } else {
            llvm::errs() << "Invalid line: '" << parts[0] << "'. "
                         << "Needs to be a number or empty for global variables.\n";
        }
    }

    assert(!parsedCrit.empty() && "Failed parsing criteria");

    // create the mapping from LLVM values to C variable names
    for (auto& it : getConstructedFunctions()) {
        ///for (auto& I : llvm::instructions(*llvm::cast<llvm::Function>(it.first))) {

        llvm::Function* F = llvm::cast<llvm::Function>(it.first);
        for(llvm::inst_iterator i = llvm::inst_begin(F), e = llvm::inst_end(F); i!=e; ++i)
        {
            llvm::Instruction& I = *i;
            ///
            if (const llvm::DbgDeclareInst *DD = llvm::dyn_cast<llvm::DbgDeclareInst>(&I)) {
                auto val = DD->getAddress();
                valuesToVariables[val] = DD->getVariable()->getName().str();
            } else if (const llvm::DbgValueInst *DV
                    = llvm::dyn_cast<llvm::DbgValueInst>(&I)) {
                auto val = DV->getValue();
                valuesToVariables[val] = DV->getVariable()->getName().str();
            }
        }
    }

    if (valuesToVariables.empty()) {
        llvm::errs() << "No debugging information found in program,\n"
                     << "slicing criteria with lines and variables will not work.\n"
                     << "You can still use the criteria based on call sites ;)\n";
        return;
    }

    // map line criteria to nodes
    for (auto& it : getConstructedFunctions()) {
        ///for (auto& I : llvm::instructions(*llvm::cast<llvm::Function>(it.first))) {
        llvm::Function* F = llvm::cast<llvm::Function>(it.first);
        for(llvm::inst_iterator i = llvm::inst_begin(F), e = llvm::inst_end(F); i!=e; ++i)
        {
            llvm::Instruction& I = *i;
            ///
            if (instMatchesCrit(dg, I, parsedCrit)) {
                LLVMNode *nd = dg.getNode(&I);
                assert(nd);
                nodes.insert(nd);
            }
        }
    }

    ///for (auto& G : dg.getModule()->globals()) {
    llvm::Module::global_iterator i = dg.getModule()->global_begin();
    llvm::Module::global_iterator e = dg.getModule()->global_end();
    for(; i!=e; ++i)
    {
        llvm::GlobalVariable& G = *i;
        ////
        if (globalMatchesCrit(G, parsedCrit)) {
            LLVMNode *nd = dg.getGlobalNode(&G);
            assert(nd);
            nodes.insert(nd);
        }
    }
}

static std::set<LLVMNode *> getSlicingCriteriaNodes(LLVMDependenceGraph& dg,
                                                    const std::string& slicingCriteria) {
    std::set<LLVMNode *> nodes;
    std::vector<std::string> criteria = splitList(slicingCriteria);
    assert(!criteria.empty() && "Did not get slicing criteria");

    std::vector<std::string> line_var_Criteria;
    std::vector<std::string> line_call_Criteria;

    std::tie(line_var_Criteria, line_call_Criteria)
            = splitStringVector(criteria, [](std::string &s) -> bool { return s.find("L&") != std::string::npos; }
    );

    //node_criteria是函数调用的callsite，本来是C&line:function, 处理后得到line:function
    //进一步处理，拆成两个vector
    std::vector<std::string> callNodes;
    std::vector<unsigned int> callSiteLines;
    for (auto &str : line_call_Criteria) {
        auto parts = splitList(str, ':');
        assert(parts.size() == 2);

        // parse the line number
        if (isNumber(parts[0])) {
            int line = atoi(parts[0].c_str());
            callSiteLines.push_back(std::move(line));
            callNodes.push_back((std::move(parts[1])));
        }
    }

    // if user wants to slice with respect to the return of main,
    // insert the ret instructions to the nodes.
    for (const auto &c : callNodes) {
        if (c == "ret") {
            LLVMNode *exit = dg.getExit();
            // We could insert just the exit node, but this way we will
            // get annotations to the functions.
            for (auto it = exit->rev_control_begin(), et = exit->rev_control_end();
                it != et; ++it) {
                nodes.insert(*it);
            }
        }
    }

    // map the criteria to nodes
    if (!callNodes.empty())
        dg.getCallSites(callNodes, callSiteLines, &nodes);
    if (!line_var_Criteria.empty())
        getLineCriteriaNodes(dg, line_var_Criteria, nodes);

    return nodes;
}

static std::pair<std::set<std::string>, std::set<std::string>>
parseSecondarySlicingCriteria(const std::string& slicingCriteria)
{
    std::vector<std::string> criteria = splitList(slicingCriteria);

    std::set<std::string> control_criteria;
    std::set<std::string> data_criteria;


    // if user wants to slice with respect to the return of main,
    // insert the ret instructions to the nodes.
    for (const auto& c : criteria) {
        auto s = c.size();
        if (s > 2 && c[s - 2] == '(' && c[s - 1] == ')')
            data_criteria.insert(c.substr(0, s - 2));
        else
            control_criteria.insert(c);
    }

    return {control_criteria, data_criteria};
}

// FIXME: copied from LLVMDependenceGraph.cpp, do not duplicate the code
static bool isCallTo(LLVMNode *callNode, const std::set<std::string>& names)
{
    using namespace llvm;

    if (!isa<llvm::CallInst>(callNode->getValue())) {
        return false;
    }

    // if the function is undefined, it has no subgraphs,
    // but is not called via function pointer
    if (!callNode->hasSubgraphs()) {
        const CallInst *callInst = cast<CallInst>(callNode->getValue());
        const Value *calledValue = callInst->getCalledValue();
        const Function *func = dyn_cast<Function>(calledValue->stripPointerCasts());
        // in the case we haven't run points-to analysis
        if (!func)
            return false;

        return array_match(func->getName(), names);
    } else {
        // simply iterate over the subgraphs, get the entry node
        // and check it
        for (LLVMDependenceGraph *dg : callNode->getSubgraphs()) {
            LLVMNode *entry = dg->getEntry();
            assert(entry && "No entry node in graph");

            const Function *func
                    = cast<Function>(entry->getValue()->stripPointerCasts());
            return array_match(func->getName(), names);
        }
    }

    return false;
}

// mark nodes that are going to be in the slice
static
bool findSecondarySlicingCriteria(std::set<LLVMNode *>& criteria_nodes,
                                  const std::set<std::string>& secondaryControlCriteria,
                                  const std::set<std::string>& secondaryDataCriteria)
{
    // FIXME: do this more efficiently (and use the new DFS class)
    std::set<LLVMBBlock *> visited;
    ADT::QueueLIFO<LLVMBBlock *> queue;
    auto tmp = criteria_nodes;
    for (auto&c : tmp) {
        queue.push(c->getBBlock());
        visited.insert(c->getBBlock());

        for (auto nd : c->getBBlock()->getNodes()) {
            if (nd == c)
                break;
            if (isCallTo(nd, secondaryControlCriteria))
                criteria_nodes.insert(nd);
            if (isCallTo(nd, secondaryDataCriteria)) {
                llvm::errs() << "WARNING: Found possible data secondary slicing criterion: "
                             << *nd->getValue() << "\n";
                llvm::errs() << "This is not fully supported, so adding to be sound\n";
                criteria_nodes.insert(nd);
            }
        }
    }

    // get basic blocks
    while (!queue.empty()) {
        auto cur = queue.pop();
        for (auto pred : cur->predecessors()) {
            for (auto nd : pred->getNodes()) {
                if (isCallTo(nd, secondaryControlCriteria))
                    criteria_nodes.insert(nd);
                if (isCallTo(nd, secondaryDataCriteria)) {
                    llvm::errs() << "WARNING: Found possible data secondary slicing criterion: "
                                 << *nd->getValue() << "\n";
                    llvm::errs() << "This is not fully supported, so adding to be sound\n";
                    criteria_nodes.insert(nd);
                }
            }
            if (visited.insert(pred).second)
                queue.push(pred);
        }
    }

    return true;
}

static AnnotationOptsT parseAnnotationOptions(const std::string& annot)
{
    if (annot.empty())
        return {};

    AnnotationOptsT opts{};
    std::vector<std::string> lst = splitList(annot);
    for (const std::string& opt : lst) {
        if (opt == "dd")
            opts |= AnnotationOptsT::ANNOTATE_DD;
        else if (opt == "cd")
            opts |= AnnotationOptsT::ANNOTATE_CD;
        else if (opt == "rd")
            opts |= AnnotationOptsT::ANNOTATE_RD;
        else if (opt == "pta")
            opts |= AnnotationOptsT::ANNOTATE_PTR;
        else if (opt == "slice" || opt == "sl" || opt == "slicer")
            opts |= AnnotationOptsT::ANNOTATE_SLICE;
    }

    return opts;
}

std::unique_ptr<llvm::Module> parseModule(llvm::LLVMContext& context,
                                          const SlicerOptions& options)
{
    llvm::SMDiagnostic SMD;

#if ((LLVM_VERSION_MAJOR == 3) && (LLVM_VERSION_MINOR <= 5))
    auto _M = llvm::ParseIRFile(options.inputFile, SMD, context);
    auto M = std::unique_ptr<llvm::Module>(_M);
#else
    auto M = llvm::parseIRFile(options.inputFile, SMD, context);
    // _M is unique pointer, we need to get Module *
#endif

    if (!M) {
        SMD.print("llvm-slicer", llvm::errs());
    }

    return M;
}

#ifndef USING_SANITIZERS
void setupStackTraceOnError(int argc, char *argv[])
{

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR < 9
    llvm::sys::PrintStackTraceOnErrorSignal();
#else
    llvm::sys::PrintStackTraceOnErrorSignal(llvm::StringRef());
#endif
    llvm::PrettyStackTraceProgram X(argc, argv);

}
#else
void setupStackTraceOnError(int, char **) {}
#endif // not USING_SANITIZERS

// lines with matching braces
std::vector<std::pair<unsigned, unsigned>> matching_braces;
// mapping line->index in matching_braces
std::map<unsigned, unsigned> nesting_structure;

static void get_lines_from_module(const llvm::Module *M, std::set<unsigned>& lines)
{
    // iterate over all instructions
    for (const llvm::Function& F : *M) {
        for (const llvm::BasicBlock& B : F) {
            for (const llvm::Instruction& I : B) {
                const llvm::DebugLoc& Loc = I.getDebugLoc();
                if (Loc.getLine() > 0)
                    lines.insert(Loc.getLine());
            }
        }
    }

    /*
    for (const GlobalVariable& G : M->globals()) {
        const DebugLoc& Loc = G.getDebugLoc();
        lines.insert(Loc.getLine());
    }
    */
}

static void get_nesting_structure(const char *source)
{
    std::ifstream ifs(source);
    if (!ifs.is_open() || ifs.bad()) {
        errs() << "Failed opening given source file: " << source << "\n";
        abort();
    }

    char ch;
    unsigned cur_line = 1;
    unsigned idx;
    std::stack<unsigned> nesting;
    while (ifs.get(ch)) {
        switch(ch) {
            case '\n':
                ++cur_line;
                if (!nesting.empty())
                    nesting_structure.emplace(cur_line, nesting.top());
                break;
            case '{':
                nesting.push(matching_braces.size());
                matching_braces.push_back({cur_line, 0});
                break;
            case '}':
                idx = nesting.top();
                assert(idx < matching_braces.size());
                assert(matching_braces[idx].second == 0);
                matching_braces[idx].second = cur_line;
                nesting.pop();
                break;
            default:
                break;
        }
    }

    ifs.close();
}

static void print_lines(std::ifstream& ifs, std::set<unsigned>& lines, std::ostream& os=std::cout)
{
    char buf[1024];
    unsigned cur_line = 1;
    while (!ifs.eof()) {
        ifs.getline(buf, sizeof buf);

        if (lines.count(cur_line) > 0) {
            os << cur_line << ": ";
            os << buf << "\n";
        }

        if (ifs.bad()) {
            errs() << "An error occured\n";
            break;
        }

        ++cur_line;
    }
}

static void print_lines_numbers(std::set<unsigned>& lines, std::ostream& os=std::cout)
{
    for (unsigned ln : lines)
        os << ln << "\n";
}


int IR2Source(llvm::Module *M, const char *source, std::ostream& os=std::cout)
{

    if (!M) {
        llvm::errs() << "Failed parsing '" << " module " << "' file:\n";
        return 1;
    }

    std::ofstream ofs("source_o.c");
    std::set<unsigned> lines;
    get_lines_from_module(M, lines);

    if (!source) {
        std::ofstream ofsLineNum("source_lineNum.c");
        print_lines_numbers(lines, ofsLineNum);
    }
    else {
        get_nesting_structure(source);
        size_t old_size;

        do {
            old_size = lines.size();
            std::set<unsigned> new_lines;

            for (unsigned i : lines) {
                new_lines.insert(i);
                auto it = nesting_structure.find(i);
                if (it != nesting_structure.end()) {
                    auto& pr = matching_braces[it->second];
                    new_lines.insert(pr.first);
                    new_lines.insert(pr.second);
                }
            }

            lines.swap(new_lines);
        } while(lines.size() > old_size);

        std::ifstream ifs(source);
        if (!ifs.is_open() || ifs.bad()) {
            errs() << "Failed opening given source file: " << source << "\n";
            return 1;
        }

        //print_lines_numbers(lines);
        print_lines(ifs, lines, ofs);
        ifs.close();
    }

    return 0;
}

int main(int argc, char *argv[])
{
    setupStackTraceOnError(argc, argv);

    SlicerOptions options = parseSlicerOptions(argc, argv);

    // dump_dg_only implies dumg_dg
    if (dump_dg_only)
        dump_dg = true;

    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> M = parseModule(context, options);
    if (!M) {
        llvm::errs() << "Failed parsing '" << options.inputFile << "' file:\n";
        return 1;
    }

    if (!M->getFunction(options.dgOptions.entryFunction)) {
        llvm::errs() << "The entry function not found: "
                     << options.dgOptions.entryFunction << "\n";
        return 1;
    }

    maybe_print_statistics(M.get(), "Statistics before ");

    // remove unused from module, we don't need that
    ModuleWriter writer(options, M.get());
    writer.removeUnusedFromModule();

    if (remove_unused_only) {
        errs() << "INFO: removed unused parts of module, exiting...\n";
        maybe_print_statistics(M.get(), "Statistics after ");
        return writer.saveModule(should_verify_module);
    }

    Slicer slicer(M.get(), options);
    if (!slicer.buildDG()) { //TODO 注意：此处只是创建了CFG，没有计算依赖关系
        errs() << "ERROR: Failed building DG\n";
        return 1;
    }

    ModuleAnnotator annotator(options, &slicer.getDG(), parseAnnotationOptions(annotationOpts));
    auto criteria_nodes = getSlicingCriteriaNodes(slicer.getDG(), options.slicingCriteria);
    if (criteria_nodes.empty()) {
        llvm::errs() << "Did not find slicing criteria: '"
                     << options.slicingCriteria << "'\n";
        if (annotator.shouldAnnotate()) {
            slicer.computeDependencies();
            annotator.annotate();
        }

        if (!slicer.createEmptyMain())
            return 1;

        maybe_print_statistics(M.get(), "Statistics after ");
        return writer.cleanAndSaveModule(should_verify_module);
    }

    auto secondaryCriteria = parseSecondarySlicingCriteria(options.secondarySlicingCriteria);
    const auto& secondaryControlCriteria = secondaryCriteria.first;
    const auto& secondaryDataCriteria = secondaryCriteria.second;

    // mark nodes that are going to be in the slice
    if (!findSecondarySlicingCriteria(criteria_nodes,
                                      secondaryControlCriteria,
                                      secondaryDataCriteria)) {
        llvm::errs() << "Finding dependent nodes failed\n";
        return 1;
    }

    // mark nodes that are going to be in the slice
    if (!slicer.mark(criteria_nodes)) {
        llvm::errs() << "Finding dependent nodes failed\n";
        return 1;
    }

    // print debugging llvm IR if user asked for it
    if (annotator.shouldAnnotate())
        annotator.annotate(&criteria_nodes);

    // slice the graph
    if (!slicer.slice()) {
        errs() << "ERROR: Slicing failed\n";
        return 1;
    }

    // remove unused from module again, since slicing
    // could and probably did make some other parts unused
    maybe_print_statistics(M.get(), "Statistics after ");
    writer.cleanAndSaveModule(should_verify_module);

    llvm::Module* pSlicedModule = writer.getModule();
    IR2Source(pSlicedModule, "qwik-smtpd.c");

    return 1;
}

