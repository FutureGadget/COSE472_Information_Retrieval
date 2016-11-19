#include "RankingModule.h"
#include <unordered_map>
#include "Entities.h"
#include <functional>
#include <algorithm>
#include <iostream>
#include "utility.h"

extern unordered_map<string, word_info> word_data_table;
extern unsigned long long C; // total number of words in the collection.

set<int> get_relevant_documents(ifstream& iindex,
	map<string, int>& Q,
	map<string, map<int, pair<int, double>>>& iiinfo)
{
	int id, df, tf, start;
	map<int, long double> doc_freq_map;
	set<int> rel_docs;
	string line;
	double weight;
	for (auto& q : Q) {
		df = word_data_table[q.first].df;
		start = word_data_table[q.first].start;
		iindex.seekg((unsigned long long)start*(6 + 6 + 3 + 7 + 2), iindex.beg);
		for (int i = 0; i < df; ++i) {
			getline(iindex, line);
			id = stoi(line.substr(6, 6));
			tf = stoi(line.substr(12, 3));
			weight = stod(line.substr(15, 7));
			if (iiinfo.find(q.first) == iiinfo.end()) {
				map<int, pair<int, double>> doc_weight;
				iiinfo.insert(make_pair(q.first, doc_weight));
			}
			iiinfo[q.first][id] = make_pair(tf, weight);
			doc_freq_map[id] += (double)q.second * q.second / Q.size() * weight;
		}
	}
	vector<pair<double, int>> result_sorted;
	for (auto x : doc_freq_map) {
		result_sorted.push_back(make_pair(x.second, x.first));
	}
	sort(result_sorted.begin(), result_sorted.end(), greater<pair<double, int>>());
	vector<pair<double, int>>::const_iterator it = result_sorted.begin();
	for (int i = 0; i < 1000 && it != result_sorted.end(); ++i, ++it) {
		rel_docs.insert(it->second);
	}
	return rel_docs;
}

//vector< pair<double, int> > vector_space_model(ifstream& iindex, set<int>& rel_docs, map<string, int>& query) {
//	map<int, pair<double, double> > doc_cos_dist;
//	map<int, pair<double, double> >::iterator it;
//	vector < pair<double, int > > result_vector;
//	string line;
//	int df, doc_id, start;
//	double weight;
//
//	for (auto &word : query) {
//		df = word_data_table[word.first].df;
//		start = word_data_table[word.first].start;
//		iindex.seekg((unsigned long long)start*(6 + 6 + 3 + 7 + 2), iindex.beg);
//		for (int i = 0; i < df; ++i) {
//			getline(iindex, line);
//			doc_id = stoi(line.substr(6, 6));
//			// If it's not in the relevant documents set, do not count in.
//			if (rel_docs.find(doc_id) == rel_docs.end()) continue;
//			weight = stod(line.substr(15, 7));
//			if ((it = doc_cos_dist.find(doc_id)) == doc_cos_dist.end()) {
//				doc_cos_dist.insert(make_pair(doc_id, make_pair((word.second * weight), weight * weight)));
//			}
//			else {
//				it->second.first += word.second * weight;
//				it->second.second += weight * weight;
//			}
//		}
//	}
//
//	for (auto res : doc_cos_dist) {
//		result_vector.push_back(make_pair((res.second.first / sqrt(res.second.second)), res.first));
//	}
//	if (result_vector.size() > 0)
//		sort(result_vector.begin(), result_vector.end(), greater<pair<double, int>>());
//	return result_vector;
//}

void language_model(ifstream& doc_file, set<int>& rel_docs, map<string, int>& query, map<string, map<int, pair<int, double>>>& iindex_mem, vector<pair<double, int>>& result) {
	map<int, double> dirichlet;
	map<int, int> doc_length;

	for (auto& doc_id : rel_docs) {
		// Get document's length from the Doc.dat file and store in on the memory.
		doc_length[doc_id] = utility::get_doc_length(doc_file, doc_id);
		for (auto& qword : query) {
			// If the query word is not in the document,
			if (iindex_mem[qword.first].find(doc_id) == iindex_mem[qword.first].end()) {
				dirichlet[doc_id] += log((RANKING_smoothing_mu * word_data_table[qword.first].cf / C) / (RANKING_smoothing_mu + doc_length[doc_id]));
			}
			else {
				// If the query word is in the document,
				dirichlet[doc_id] += log((iindex_mem[qword.first][doc_id].first 
					+ RANKING_smoothing_mu * word_data_table[qword.first].cf / C) / (RANKING_smoothing_mu + doc_length[doc_id]));
			}
		}
	}

	for (auto& res : dirichlet) {
		result.push_back(make_pair(res.second, res.first));
	}
	sort(result.begin(), result.end(), greater<pair<double, int>>());
}