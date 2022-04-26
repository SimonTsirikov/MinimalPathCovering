#include "llvm_req.hpp"
#include "extract_cfg.hpp"

using namespace llvm;
using namespace std;


struct MyVisitor : public InstVisitor<MyVisitor> {

    map<string, vector<tuple<float, string> > >& cfg;
    map<int, tuple<int, int> >& weights;

    MyVisitor(map<string, vector<tuple<float, string> > >& _cfg, map<int, tuple<int, int> >&_weights): cfg(_cfg), weights(_weights) {};

    string tmp_function_name;

    void visitFunction(Function &F) {
        tmp_function_name = F.getName().str();
    }

    void visitBranchInst(BranchInst &BI) {
        BasicBlock* from  = BI.getParent();
        string from_label = tmp_function_name + "_" + from->getName().str();
        int index;
        while ((index = from_label.find(".")) != string::npos) {
            from_label.replace(index, 1, "_");
        }
        int weight = 1;
        float true_weight  = 0;
        float false_weight = 0;

        if (BI.getNumSuccessors() == 2 && BI.getDebugLoc()) {
            int line = BI.getDebugLoc().getLine();
            if (weights.count(line) != 0) {
                int true_count  = get<0>(weights[line]);
                int false_count = get<1>(weights[line]);
                weight = 0;
                true_weight  = (float) true_count  / (float) (true_count + false_count);
                false_weight = (float) false_count / (float) (true_count + false_count);
            }
        }

        for (unsigned i = 0; i < BI.getNumSuccessors(); i++) {
    
            BasicBlock* to  = BI.getSuccessor(i);
            string to_label = tmp_function_name + "_" + to->getName().str();
            while ((index = to_label.find(".")) != string::npos) {
                to_label.replace(index, 1, "_");
            }

            if (cfg.find(from_label) == cfg.end()) {
                vector<tuple<float, string>> adj = {{weight + (i == 0 ? true_weight : false_weight), to_label}};
                cfg.insert(make_pair(from_label, adj));
            } else {
                cfg[from_label].push_back({weight + (i == 0 ? true_weight : false_weight), to_label});
            }
        }
    }

    void visitCallInst(CallInst &CI) {
        BasicBlock* from = CI.getParent();
        Function*   F    = CI.getCalledFunction();
        BasicBlock* to   = &F->getEntryBlock();

        string from_label = tmp_function_name  + "_" + from->getName().str();
        int index;
        while ((index = from_label.find(".")) != string::npos) {
            from_label.replace(index, 1, "_");
        }
        string to_label   = F->getName().str() + "_" + to->getName().str();
        while ((index = to_label.find(".")) != string::npos) {
            to_label.replace(index, 1, "_");
        }
        if (to_label.substr(0, 4) != "llvm") {
            if (cfg.find(from_label) == cfg.end()) {
                vector<tuple<float, string>> adj = {{1, to_label}};
                cfg.insert(make_pair(from_label, adj));
            } else {
                cfg[from_label].push_back({1, to_label});
            }
        }
    }
};


map<int, tuple<int, int> > make_weights(char* profiling_file_name) {
    map<int, tuple<int, int> > weights;
    
    if (profiling_file_name != NULL) {
        fstream prof_data;
        prof_data.open(profiling_file_name, ios::in);
        if (prof_data.is_open()) {
            string line;
            while(getline(prof_data, line) && line.size() != 0) {
                size_t branch_start = line.find("|  Branch");
                if (branch_start != string::npos) {
                    size_t line_index_end  = line.find_first_of(":", branch_start+1);
                    size_t col_index_end   = line.find_first_of(")", line_index_end+1);
                    size_t true_count_end  = line.find_first_of(",", col_index_end+1);
                    if (true_count_end != string::npos) {
                        size_t false_count_end = line.find_first_of("]", true_count_end+1);

                        int line_index  = stoi(line.substr(branch_start+11, line_index_end-branch_start-11));
                        // int col_index   = stoi(line.substr(line_index_end+1, col_index_end-line_index_end-1));
                        int true_count  = stoi(line.substr(col_index_end+10, true_count_end-col_index_end-10));
                        int false_count = stoi(line.substr(true_count_end+9, false_count_end-true_count_end-9));

                        if (weights.count(line_index) == 0) {
                            weights.insert({line_index, {true_count, false_count}});
                        }
                    }
                }
            }
            prof_data.close();
        }
    }
    return weights;
}


map<string, vector<tuple<float, string>>> make_cfg(char* ir_file_name, char* profiling_file_name) {

    SMDiagnostic Err;
    LLVMContext Context;
    unique_ptr<Module> Mod(parseIRFile(ir_file_name, Err, Context));
    if (!Mod) {
        Err.print("File not found.", errs());
        return {};
    }

    map<int, tuple<int, int> > weights = make_weights(profiling_file_name);

    map<string, vector<tuple<float, string> > > cfg;
    MyVisitor visitor(cfg, weights);
    visitor.visit(*Mod);

    return cfg;
}
