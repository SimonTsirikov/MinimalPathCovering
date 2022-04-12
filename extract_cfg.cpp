#include "llvm_req.hpp"
#include "extract_cfg.hpp"

using namespace llvm;
using namespace std;


struct MyVisitor : public InstVisitor<MyVisitor> {

    map<string, vector<tuple<int, string> > >& cfg;

    MyVisitor(map<string, vector<tuple<int, string> > >& _cfg): cfg(_cfg) {};

    string tmp_function_name;

    void visitFunction(Function &F) {
        tmp_function_name = F.getName().str();
    }

    void visitBranchInst(BranchInst &BI) {
        BasicBlock* from  = BI.getParent();
        string from_label = tmp_function_name + "_" + from->getName().str();
    
        for (unsigned i = 0; i < BI.getNumSuccessors(); i++) {
    
            BasicBlock* to  = BI.getSuccessor(i);
            string to_label = tmp_function_name + "_" + to->getName().str();

            if (cfg.find(from_label) == cfg.end()) {
                vector<tuple<int, string>> adj = {{1, to_label}};
                cfg.insert(make_pair(from_label, adj));
            } else {
                cfg[from_label].push_back({1, to_label});
            }
        }
    }

    void visitCallInst(CallInst &CI) {
        BasicBlock* from = CI.getParent();
        Function*   F    = CI.getCalledFunction();
        BasicBlock* to   = &F->getEntryBlock();

        string from_label = tmp_function_name  + "_" + from->getName().str();
        string to_label   = F->getName().str() + "_" + to->getName().str();

        if (cfg.find(from_label) == cfg.end()) {
            vector<tuple<int, string>> adj = {{1, to_label}};
            cfg.insert(make_pair(from_label, adj));
        } else {
            cfg[from_label].push_back({1, to_label});
        }
    }
};


map<string, vector<tuple<int, string>>> make_cfg(char* file_name) {

    SMDiagnostic Err;
    LLVMContext Context;
    unique_ptr<Module> Mod(parseIRFile(file_name, Err, Context));
    if (!Mod) {
        Err.print("File not found.", errs());
        return {};
    }

    map<string, vector<tuple<int, string> > > cfg;
    MyVisitor visitor(cfg);
    visitor.visit(*Mod);

    return cfg;
}
