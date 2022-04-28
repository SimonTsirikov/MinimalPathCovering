#include <vector>
#include <cassert>
#include "extract_cfg.hpp"

using namespace std;
map<int, string> rev_vertex_ind;

struct graph
{
	int vertex_num;
	int row_num;
	float inf = 1;
	vector<vector<float>> adj_matrix;

	graph(int row_num, int vertex_num, float max_weight)
	{
		this->vertex_num = vertex_num;
		this->row_num = row_num;
		this->inf = max_weight*vertex_num;
		vector<float> column(vertex_num + 1, inf);
		adj_matrix = vector<vector<float>>(row_num + 1, column);
	}
};

void print_matrix(const vector<vector<float>>& matrix)
{
	for (auto const& row: matrix)
	{
		for (auto const& v: row)
			cout << v << " ";
		cout << endl;
	}
}

map<int, int> get_matching(graph input_graph)
{
	int vertex_num = input_graph.vertex_num;
	int row_num = input_graph.row_num;

	vector<float> row_sub(row_num + 1, 0), column_sub(vertex_num + 1, 0);
	vector<int> matching(vertex_num + 1, 0), way(vertex_num + 1, 0);
	for (int i = 1; i <= row_num; ++i)
	{
		matching[0] = i;
		int cur_j = 0, min_j;
		vector<float> mins(vertex_num + 1, input_graph.inf);
		vector<bool> visited(vertex_num + 1, false);
		do
		{
			visited[cur_j] = true;
			int cur_i = matching[cur_j];
			float delta = input_graph.inf;
			for (int j = 1; j <= vertex_num; ++j)
			{
				if (!visited[j])
				{
					float cur_value = input_graph.adj_matrix[cur_i][j] - row_sub[cur_i] - column_sub[j];
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

	map<int, int> result;
	for (int j = 1; j <= vertex_num; j++)
	{
		if (matching[j] > 0)
		{
			result[matching[j]] = j;
			if (j != matching[j])
				result[-j] = matching[j];
		}
	}
	return result;
}

vector<vector<int>> get_min_path_covery(map<int, int> matching, const graph& input_graph)
{
	vector<vector<int>> result;
	vector<bool> visited(input_graph.vertex_num, false);
	int state_num = 0;
	for (int i = 1; i <= input_graph.row_num; i++)
	{
		int from = i;
		int to = matching[i];
		if (!visited[from] && input_graph.adj_matrix[from][to] != input_graph.inf)
		{
			while (matching[-from] != 0 && matching[-from] != to)
			{
				int pre_from = matching[-from];
				if (input_graph.adj_matrix[pre_from][from] == input_graph.inf)
					break;
				from = pre_from;
			}
			to = matching[from];
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
				{
					from = to;
					to = matching[to];
					if (input_graph.adj_matrix[from][to] == input_graph.inf)
						break;
				}
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
		cerr << "Usage: " << argv[0] << " <IR file> [<profiling report>]\n";
		return 1;
	}
	char* profiling_file = argc == 3 ? argv[2] : NULL;
	map<string, vector<tuple<float, string>>> cfg = make_cfg(argv[1], profiling_file);
	map<string, int> vertex_ind;

	int from_ind = 0, to_ind = cfg.size();
	float max_weight = 0;
	cout << "\nСontrol flow graph:\n";
	for (auto const&[from, to_s]: cfg)
	{
		vector<string> vertex_queue;
		vertex_queue.push_back(from);
		while (!vertex_queue.empty())
		{
			string cur_vertex = vertex_queue[0];
			vertex_queue.erase(vertex_queue.begin());
			if (vertex_ind[cur_vertex] == 0)
			{
				int cur_ind = cfg.find(cur_vertex) == cfg.end() ? ++to_ind : ++from_ind;
				vertex_ind[cur_vertex] = cur_ind;
				rev_vertex_ind[cur_ind] = cur_vertex;
			}
			for (auto const&[weight, to]: to_s)
			{
				if (vertex_ind[to] == 0)
					vertex_queue.push_back(to);
			}
		}
		for (auto const&[weight, to]: to_s)
		{
			cout << from << " -> " << to << " [ label=\"" << weight << "\" ];" << endl;
			if (weight > max_weight)
				max_weight = weight;
		}
	}
	auto cur_graph = graph(from_ind, to_ind, max_weight);
	for (auto const&[from, to_s]: cfg)
	{
		from_ind = vertex_ind[from];
		for (auto const&[weight, to]: to_s)
		{
			to_ind = vertex_ind[to];
			cur_graph.adj_matrix[from_ind][to_ind] = -weight;
		}
	}
	auto matching = get_matching(cur_graph);
	auto min_path_covery = get_min_path_covery(matching, cur_graph);

	cout << "\nMinimal path covery:\n";
	for (auto const &path: min_path_covery)
	{
		bool first = true;
		for (auto const &ind: path)
		{
			if (!first)
				cout << " -> ";
			first = false;
			cout << rev_vertex_ind[ind];
		}
		cout << ";" << endl;
	}
	cout << endl;
	return 0;
}
