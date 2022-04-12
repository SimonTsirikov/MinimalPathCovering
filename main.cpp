#include "extract_cfg.hpp"

using namespace std;

int main(int argc, char** argv) {

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <IR file>\n";
        return 1;
    }

    map<string, vector<tuple<int, string>>> cfg = make_cfg(argv[1]);

    for (auto const& [from, to_s]: cfg) {
        cout << from << " -> ";
        for (auto const& [weight, to]: to_s) {
            cout << weight << ":" << to << ", ";
        }
        cout << endl;
    }

    return 0;
}

