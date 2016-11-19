#pragma once
#include <set>
#include <map>
#include <fstream>
#include <vector>
#include <string>
#include "Parameters.h"

using namespace std;

set<int> get_relevant_documents(ifstream& iindex, map<string, int>& Q, map<string, map<int, pair<int, double>>>& iiinfo);
vector< pair<double, int> > vector_space_model(ifstream& iindex, set<int>& rel_docs, map<string, int>& query);
void language_model(ifstream& doc_file, set<int>& rel_docs, map<string, int>& query, map<string, map<int, pair<int, double>>>& iindex_mem, vector<pair<double, int>>& result);