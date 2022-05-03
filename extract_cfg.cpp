#include "llvm_req.hpp"
#include "extract_cfg.hpp"

using namespace llvm;
using namespace std;


struct MyVisitor : public InstVisitor<MyVisitor> {

    map<string, vector<tuple<float, string> > >& cfg;
    map<int, tuple<long, long> >& weights;
    map<string, BasicBlock*>& mapping;

    MyVisitor(map<string, vector<tuple<float, string> > >& _cfg, map<int, tuple<long, long> >& _weights, map<string, BasicBlock*>& _mapping): cfg(_cfg), weights(_weights), mapping(_mapping) {};

    string tmp_function_name = "";

    void visitFunction(Function &F) {
        tmp_function_name = F.getName().str();
    }

    void visitBranchInst(BranchInst &BI) {
        BasicBlock* from  = BI.getParent();
        string from_label = tmp_function_name + "–" + from->getName().str();
        int index;
        while ((index = from_label.find(".")) != string::npos) {
            from_label.replace(index, 1, "_");
        }
        if (mapping.count(from_label) == 0) {
            mapping.insert(make_pair(from_label, from));
        }
        int weight = 1;
        float true_weight  = 0;
        float false_weight = 0;

        if (BI.getNumSuccessors() == 2 && BI.getDebugLoc()) {
            int line = BI.getDebugLoc().getLine();
            if (weights.count(line) != 0) {
                long true_count  = get<0>(weights[line]);
                long false_count = get<1>(weights[line]);
                weight = 0;
                if (true_count + false_count != 0) {
                    true_weight  = (float) true_count  / (float) (true_count + false_count);
                    false_weight = (float) false_count / (float) (true_count + false_count);
                }
            }
        }

        for (unsigned i = 0; i < BI.getNumSuccessors(); i++) {
            BasicBlock* to  = BI.getSuccessor(i);
            string to_label = tmp_function_name + "–" + to->getName().str();
            while ((index = to_label.find(".")) != string::npos) {
                to_label.replace(index, 1, "_");
            }
            if (mapping.count(to_label) == 0) {
                mapping.insert(make_pair(to_label, to));
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
        if (F != NULL) {    
            BasicBlock* to   = &F->getEntryBlock();
            string from_label = tmp_function_name  + "–" + from->getName().str();
            int index;
            while ((index = from_label.find(".")) != string::npos) {
                from_label.replace(index, 1, "_");
            }
            if (mapping.count(from_label) == 0) {
                mapping.insert(make_pair(from_label, from));
            }
            string to_label   = F->getName().str() + "–" + to->getName().str();
            while ((index = to_label.find(".")) != string::npos) {
                to_label.replace(index, 1, "_");
            }
            if (mapping.count(to_label) == 0) {
                mapping.insert(make_pair(to_label, to));
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
    }
};


map<int, tuple<long, long> > make_weights(char* profiling_file_name) {
    map<int, tuple<long, long> > weights;
    
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
                        float true_mult = 1;
                        string true_last = line.substr(true_count_end-1, 1);
                        switch (true_last[0]) {
                            case 'k':
                                true_mult = 1e3;
                                break;
                            case 'M':
                                true_mult = 1e6;
                                break;                                
                            case 'G':
                                true_mult = 1e9;
                                break;
                        }
                        float false_mult = 1;
                        string false_last = line.substr(false_count_end-1, 1);
                        switch (false_last[0]) {
                            case 'k':
                                false_mult = 1e3;
                                break;
                            case 'M':
                                false_mult = 1e6;
                                break;                                
                            case 'G':
                                false_mult = 1e9;
                                break;
                        }

                        long true_count  = (long) (true_mult * stof(line.substr(col_index_end+10, true_count_end-col_index_end-10 - (true_mult == 1 ? 0 : 1))));
                        long false_count = (long) (false_mult * stof(line.substr(true_count_end+9, false_count_end-true_count_end-9 - (false_mult == 1 ? 0 : 1))));

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

    map<int, tuple<long, long> > weights = make_weights(profiling_file_name);
    map<string, vector<tuple<float, string> > > cfg;
    map<string, BasicBlock*> mapping;
    MyVisitor visitor(cfg, weights, mapping);
    visitor.visit(*Mod);
    return cfg;
}

void arrange_ir(char* ir_file_name, vector<vector<int>> covery, map<int, string> index) {
    SMDiagnostic Err;
    LLVMContext Context;
    unique_ptr<Module> Mod(parseIRFile(ir_file_name, Err, Context));

    map<int, tuple<long, long> > weights = make_weights(NULL);
    map<string, vector<tuple<float, string> > > cfg;
    map<string, BasicBlock*> mapping;
    MyVisitor visitor(cfg, weights, mapping);
    visitor.visit(*Mod);

    for (auto const &path: covery) {
		string prev = "";
        string prev_fun = "";
		for (auto const &ind: path) {
            string curr = index[ind];
            size_t l_ind;
            if ((l_ind = curr.find("–")) != string::npos) {
                string curr_fun = curr.substr(0, l_ind);
                if (prev != "" && prev_fun == curr_fun && mapping.count(curr) != 0 && mapping.count(prev) != 0) {
                    mapping[curr]->moveAfter(mapping[prev]);
                }
                prev_fun = curr_fun;
            }
            prev = curr;
		}
	}

	error_code EC;
    raw_ostream *out = new raw_fd_ostream(string(ir_file_name) + "_arr.ll", EC, sys::fs::OpenFlags());
	Mod->print(*out, nullptr);
}
