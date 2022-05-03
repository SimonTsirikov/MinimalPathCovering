#include <vector>
#include <tuple>
#include <map>
#include <iostream>
#include <fstream>
#include <string>

std::map<std::string, std::vector<std::tuple<float, std::string> > > make_cfg(char* ir_file_name, char* profiling_file_name);
void arrange_ir(char* ir_file_name,std::vector<std::vector<int>> covery, std::map<int, std::string> index);
