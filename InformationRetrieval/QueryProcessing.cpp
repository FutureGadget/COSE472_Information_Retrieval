#include "QueryProcessing.h"
#include "porter2_stemmer.h"
#include "Entities.h"
#include "utility.h"
#include "Parameters.h"
#include <string>
#include <fstream>
#include <unordered_set>

extern unordered_set<string> stopwords;
extern unordered_map<string, word_info> word_data_table;

void process_query_word(string& word, map<string, int>& Q, int weight) {
	Porter2Stemmer::trim(word);
	if (stopwords.find(word) != stopwords.end()) return;
	else {
		Porter2Stemmer::stem(word);
		if (word_data_table.find(word) == word_data_table.end()) return;
		if (stopwords.find(word) != stopwords.end() || word == "") return;
		else {
			// Query weight is calculated by its term frequency.
			if (Q.find(word) == Q.end()) {
				Q[word] = weight;
			}
			else {
				Q[word] += weight;
			}
		}
	}
}

/*
1. Read query file and trim & stemming the query terms.
2. Count query term frequency to calculate qi in the vector space model.
@returns queries
*/
map<int, map<string, int>> read_query_file(string queryFile) {
	ifstream file(queryFile);
	string line;
	map<int, map<string, int>> queries;
	map<string, int> query;
	vector<string> tokens;
	vector<string> sp_char_removed;
	int query_num;
	while (getline(file, line)) {
		tokens = utility::tokenizer(line, ' ');
		if (tokens.size() == 0) continue;
		else if (tokens[0] == "<top>") continue;
		else if (tokens[0] == "</top>") continue;
		else if (tokens[0] == "<num>") {
			query_num = stoi(tokens[2]);
			queries.insert(make_pair(query_num, query));
		}
		else if (tokens[0] == "<title>") {
			for (int i = 1; i < tokens.size(); ++i) {
				utility::tokenize_only_alpha(tokens[i], sp_char_removed);
				for (auto& word : sp_char_removed) {
					process_query_word(word, queries[query_num], QUERY_title_weight);
				}
			}
		}
		else if (tokens[0] == "<desc>") { continue; }
		else if (tokens[0] == "<narr>") { continue; }
		else {
			for (auto s : tokens) {
				utility::tokenize_only_alpha(s, sp_char_removed);
				for (auto& word : sp_char_removed) {
					process_query_word(word, queries[query_num], QUERY_others_weight);
				}
			}
		}
	}
	file.close();
	return queries;
}