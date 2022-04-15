#include <vector>
#include <cassert>
#include "extract_cfg.hpp"

using namespace std;

struct graph
{
	int vertex_num;
	int row_num;
	int inf;
	vector<vector<int>> adj_matrix;

	graph(int row_num, int vertex_num, int max_weight)
	{
		this->vertex_num = vertex_num;
		this->row_num = row_num;
		inf = max_weight*vertex_num;
		vector<int> column(vertex_num + 1, inf);
		adj_matrix = vector<vector<int>>(row_num + 1, column);
	}
};

void print_matrix(const vector<vector<int>>& matrix)
{
	for (auto const& row: matrix)
	{
		for (auto const& v: row)
			cout << v << " ";
		cout << endl;
	}
}

vector<int> get_matching(const graph& input_graph)
{
	int vertex_num = input_graph.vertex_num;
	int row_num = input_graph.row_num;

	vector<int> row_sub(row_num + 1, 0), column_sub(vertex_num + 1, 0);
	vector<int> matching(vertex_num + 1, 0), way(vertex_num + 1, 0);
	for (int i = 1; i <= row_num; ++i)
	{
		matching[0] = i;
		int cur_j = 0, min_j;
		vector<int> mins(vertex_num + 1, input_graph.inf);
		vector<bool> visited(vertex_num + 1, false);
		do
		{
			visited[cur_j] = true;
			int cur_i = matching[cur_j];
			int delta = input_graph.inf;
			for (int j = 1; j <= vertex_num; ++j)
			{
				if (!visited[j])
				{
					int cur_value = input_graph.adj_matrix[cur_i][j] - row_sub[cur_i] - column_sub[j];
					if (cur_value < mins[j])
					{
						mins[j] = cur_value;
						way[j] = cur_j;
					}
					if (mins[j] < delta)
					{
						delta = mins[j];
						min_j = j;
					}
				}
			}
			for (int j = 0; j <= vertex_num; ++j)
			{
				if (visited[j])
				{
					row_sub[matching[j]] += delta;
					column_sub[j] -= delta;
				}
				else
					mins[j] -= delta;
			}
			cur_j = min_j;
		}
		while (matching[cur_j] != 0);
		do
		{
			min_j = way[cur_j];
			matching[cur_j] = matching[min_j];
			cur_j = min_j;
		}
		while (cur_j);
	}

	vector<int> result(row_num + 1, 0);
	for (int j = 1; j <= vertex_num; j++)
	{
		if (matching[j] > 0)
		{
			result[matching[j]] = j;
			int cur_weight = input_graph.adj_matrix[matching[j]][j];
			if (cur_weight == input_graph.inf)
				result[j] = j;
		}
	}
	return result;
}

vector<vector<int>> get_min_path_covery(vector<int> matching, const graph& input_graph)
{
	vector<vector<int>> result;
	vector<bool> visited(input_graph.vertex_num, false);
	int state_num = 0;
	for (int from = 1; from <= input_graph.row_num; from++)
	{
		int to = matching[from];
		if (!visited[from])
		{
			vector<int> cur_vector;
			cur_vector.push_back(from);
			++state_num;
			visited[from] = true;
			while (!visited[to])
			{
				cur_vector.push_back(to);
				++state_num;
				visited[to] = true;
				if (to <= input_graph.row_num)
					to = matching[to];
			}
			result.push_back(cur_vector);
		}
	}
	for (int from = 1; from <= input_graph.vertex_num; from++)
	{
		if (!visited[from])
		{
			vector<int> cur_vector(1, from);
			++state_num;
			result.push_back(cur_vector);
		}
	}
	assert (("Not all vertices are covered", state_num >= input_graph.vertex_num));
	assert (("Some vertices are covered more than once", state_num <= input_graph.vertex_num));
	return result;
}


int main(int argc, char** argv)
{

	if (argc < 2)
	{
		cerr << "Usage: " << argv[0] << " <IR file>\n";
		return 1;
	}

	map<string, vector<tuple<int, string>>> cfg = make_cfg(argv[1]);
	map<string, int> vertex_ind;
	map<int, string> rev_vertex_ind;

	int from_ind = 0, to_ind = cfg.size();
	int max_weight = 0;
	cout << "\nСontrol flow graph:\n";
	for (auto const&[from, to_s]: cfg)
	{
		cout << from << " -> ";
		if (vertex_ind.find(from) == vertex_ind.end())
		{
			vertex_ind[from] = ++from_ind;
			rev_vertex_ind[from_ind] = from;
		}
		bool first = true;
		for (auto const&[weight, to]: to_s)
		{
			if (!first)
				cout << ", ";
			first = false;
			cout << weight << ":" << to;
			if (cfg.find(to) == cfg.end() && vertex_ind.find(to) == vertex_ind.end())
			{
				vertex_ind[to] = ++to_ind;
				rev_vertex_ind[to_ind] = to;
			}
			if (1/weight > max_weight)
				max_weight = 1/weight;
		}
		cout << endl;
	}
	auto cur_graph = graph(from_ind, to_ind, max_weight);
	for (auto const&[from, to_s]: cfg)
	{
		from_ind = vertex_ind[from];
		//cur_graph.adj_matrix[from_ind][from_ind] = max_weight + 1;
		for (auto const&[weight, to]: to_s)
		{
			to_ind = vertex_ind[to];
			cur_graph.adj_matrix[from_ind][to_ind] = weight == 0 ? 0 : 1/weight;
		}
	}

	auto matching = get_matching(cur_graph);
	auto min_path_covery = get_min_path_covery(matching, cur_graph);

	cout << "\nMinimal path covery:\n";
	for (auto const &path: min_path_covery)
	{
		bool first = true;
		for (auto const &column_sub: path)
		{
			if (!first)
				cout << " -> ";
			first = false;
			cout << rev_vertex_ind[column_sub];
		}
		cout << endl;
	}
	cout << endl;
	return 0;
}
