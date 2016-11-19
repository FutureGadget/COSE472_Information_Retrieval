#pragma once
#include <string>
#include <map>

using namespace std;
map<int, map<string, int>> read_query_file(string queryFile);
void process_query_word(std::string& word, std::map<std::string, int>& Q, int weight);