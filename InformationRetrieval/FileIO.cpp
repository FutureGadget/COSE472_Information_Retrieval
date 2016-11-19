#include <vector>
#include <utility>
#include "Entities.h"
#include <unordered_map>
#include "utility.h"
#include "FileIO.h"

using namespace std;

unsigned long long C = 0;  // total number of words in the collection.
extern unordered_map<string, word_info> word_data_table;
extern unsigned int doc_count;// number of documents

void create_word_data_file(ofstream& word_data_file) {
	int word_id = 1;
	int position = 0;
	for (auto word : word_data_table) {
		word_data_table[word.first].start = position;
		word_data_table[word.first].id = word_id++;
		word_data_file << word_id - 1 << '\t' << word.first << '\t' << word.second.df << '\t' << word.second.cf << '\t' << position << endl;
		position += word.second.df;
	}
}

void create_index_file(ofstream& doc_file, ofstream& tf_file, map<string, int>& tempTF, string docno, int doc_id, int doc_length) {
	for (auto word : tempTF) {
		tf_file << doc_id << '\t' << word.first << '\t' << word.second << endl;
	}
	doc_file << utility::format_digit(6, doc_id) << docno << utility::format_digit(4, doc_length) << endl;
}

void create_inverted_index_file(ofstream& iif) {
	ifstream index_file("TF.txt");
	vector<string> tokens;
	map< string, pair<int, double> > tf_idf_word; // TF, weight
	double denominator = 0.0f;
	string line;
	int doc_id;
	int doc_id_2;
	getline(index_file, line);
	tokens = utility::tokenizer(line, '\t');
	doc_id = stoi(tokens[0]);
	while (1) {
		tf_idf_word.insert(make_pair(tokens[1],
			make_pair(stoi(tokens[2]), (log(stod(tokens[2])) + 1) * log((double)doc_count / word_data_table[tokens[1]].df))));
		denominator += tf_idf_word[tokens[1]].second * tf_idf_word[tokens[1]].second;
		// END OF FILE
		if (!getline(index_file, line)) {
			for (auto &word : tf_idf_word) {
				iif << utility::format_digit(6, word_data_table[word.first].id)
					<< utility::format_digit(6, doc_id)
					<< utility::format_digit(3, word.second.first)
					<< utility::format_weight(word.second.second / sqrt(denominator))
					<< endl;
			}
			tf_idf_word.clear();
			break;
		}
		tokens = utility::tokenizer(line, '\t');
		doc_id_2 = stoi(tokens[0]);
		// NEW DOCUMENT ID
		if (doc_id_2 != doc_id) {
			for (auto &word : tf_idf_word) {
				iif << utility::format_digit(6, word_data_table[word.first].id)
					<< utility::format_digit(6, doc_id)
					<< utility::format_digit(3, word.second.first)
					<< utility::format_weight(word.second.second / sqrt(denominator))
					<< endl;
			}
			// INITIALIZATION
			doc_id = doc_id_2;
			tf_idf_word.clear();
			denominator = 0.0f;
		}
	}
	index_file.close();
}

void load_term_file(string infile) {
	ifstream file(infile);
	string line, token;
	vector<string> v;
	while (getline(file, line)) {
		v = utility::tokenizer(line, '\t');
		word_data_table[v[1]].id = stoi(v[0]);
		word_data_table[v[1]].df = stoi(v[2]);
		word_data_table[v[1]].cf = stoi(v[3]);
		word_data_table[v[1]].start = stoi(v[4]);
	}

	/*
	Count words in the collection. Used in smoothing. (f(qi, C) = C(qi) / |C|)
	*/
	for (auto& word : word_data_table) {
		C += word.second.cf;
	}

	file.close();
}