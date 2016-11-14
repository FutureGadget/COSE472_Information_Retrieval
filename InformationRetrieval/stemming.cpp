#include <fstream>
#include <vector>
#include <unordered_set>
#include <string>
#include <sstream>
#include <utility>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <functional>
#include <queue>

#include <chrono>
#include <ctime>

#include "DocumentEntity.h"
#include "utility.h"
#include "porter2_stemmer.h"

using namespace std;
using namespace std::chrono;

/*
	Constant String Variables.
*/
// contant TAG strings
const string TAG_HEADLINE = "<HEADLINE>";
const string TAG_HEADLINE_END = "</HEADLINE>";
const string TAG_DOCNO = "<DOCNO>";
const string TAG_DOCNO_END = "</DOCNO>";
const string TAG_TEXT = "<TEXT>";
const string TAG_TEXT_END = "</TEXT>";

// File names

// Tuning part 1. Stopwords
//const char* stopword_fname = "stopwords";
const char* stopword_fname = "stopwordsV4";
const char* tf_filename = "TF.txt";
const char* doc_data_fname = "Doc.dat";
const char* word_data_fname = "Term.dat";
const char* inverse_data_fname = "index_tmp.dat";

struct word_info {
	int id;
	int cf;
	int df;
	int start;
	word_info() {
		this->cf = 0;
		this->df = 0;
	}
};

struct inverted_index {
	int word_id;
	double weight;
	string data;
	bool operator<(inverted_index& t) {
		if (this->word_id == t.word_id) {
			return this->weight > t.weight;
		}
		else {
			return this->word_id < t.word_id;
		}
	}
	inverted_index() {}
	inverted_index(int word_id, double weight, string data) {
		this->word_id = word_id;
		this->weight = weight;
		this->data = data;
	}
};

int doc_count = 0;	// document id (Which means the number of documents)
int num_total_docs = 0;   // total number of documents. (Collection)
unsigned long long C; // total number of words in the collection.

/**
	Word index table can be located in the memory. (Heaps' law)
*/
unordered_map<string, word_info> word_data_table;

/*
	Store stop words in the unordered_set for a fast comparison.
*/
unordered_set<string> stopwords;

/*
	Function declarations.
*/
void stem_doc_porter2(document&);

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

void constructStopWordSet() {
	ifstream file;
	file.open(stopword_fname);
	if (file.is_open()) {
		string line;
		while (getline(file, line)) {
			stopwords.insert(line);
		}
		file.close();
	}

	// add two letter words to the stopword set
	/*string two("aa");
	for (int i = 'a'; i <= 'z'; ++i) {
		for (int j = 'a'; j <= 'z'; ++j) {
			two[0] = i;
			two[1] = j;
			stopwords.insert(two);
		}
		stopwords.insert(to_string(i));
	}*/
}

inline void create_index_file(ofstream& doc_file, ofstream& tf_file, map<string, int>& tempTF, string docno, int doc_id, int doc_length) {
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

void updateWordInfo(string& word, map<string, int>& tempTF, int weight) {
	if (word_data_table.find(word) == word_data_table.end()) {
		word_data_table[word] = word_info();
	}
	if (tempTF.find(word) == tempTF.end()) {
		tempTF[word] = weight;
		word_data_table[word].df++;
	}
	else {
		tempTF[word] += weight;
	}
	word_data_table[word].cf++;
}

void text_transform(string& line, map<string, int>& tempTF, int weight, int &doc_length) {
	vector<string> tokens;
	utility::tokenize_only_alpha(line, tokens);
	for (auto& token : tokens) {
		Porter2Stemmer::trim(token);
		if (stopwords.find(token) != stopwords.end()) continue;
		Porter2Stemmer::stem(token);
		if (token != "" && stopwords.find(token) == stopwords.end()) {
			doc_length++;
			updateWordInfo(token, tempTF, weight);
		}
	}
}

void fast_parse(ifstream& file, ofstream& doc_file, ofstream& tf_file) {
	string line;
	vector<string> only_alpha_tokens;
	map<string, int> tempTF;
	int fidx, doc_length = 0;
	string docno, tmp;
	while (getline(file, line)) {
		if (line == "<DOC>") {
			doc_count = doc_count + 1;
			getline(file, line);
			while (line != "</DOC>") {
				if ((fidx = line.find("<DOCNO>") != string::npos)) {
					docno = line.substr(fidx + 6, line.size() - 7 - 8 - 1);
					utility::trim(docno);
					getline(file, line);
				} else if (line == "<HEADLINE>") {
					getline(file, line);
					while (line != "</HEADLINE>") {
						// Parse HEADLINE
						text_transform(line, tempTF, 5, doc_length);
						getline(file, line);
					}

				} else if ((fidx = line.find("<HEADLINE>")) != string::npos) {
					tmp = line.substr(fidx + 11, line.size() - 10 - 11);
					utility::trim(tmp);
					text_transform(tmp, tempTF, 5, doc_length);
					getline(file, line);
				} else if (line == "<TEXT>") {
					getline(file, line);
					while (line != "</TEXT>") {
						if (line == "<P>") {
							getline(file, line);
							while (line != "</P>") {
								// Parse Paragraph
								text_transform(line, tempTF, 1, doc_length);
								getline(file, line);
							}
						}
						else if (line == "<SUBHEAD>") {
							getline(file, line);
							while (line == "</SUBHEAD>") {
								// Parse SUBHEAD
								text_transform(line, tempTF, 3, doc_length);
								getline(file, line);
							}
						}
						else if (line[0] != '<') { // contents without a <P> tag
							text_transform(line, tempTF, 1, doc_length);
							getline(file, line);
						}
						else {
							getline(file, line);
						}
					}
				}
				else if (line[0] != '<') {
					text_transform(line, tempTF, 3, doc_length);
					getline(file, line);
				}
				else {
					getline(file, line);
				}
			} // END OF DOC
			// Write TF.txt
			create_index_file(doc_file, tf_file, tempTF, docno, doc_count, doc_length);
			doc_length = 0;
			tempTF.clear();
		}
	}
}

string merge(int first, int last) {
	ifstream file1, file2;
	ofstream res_file;
	string f1, f2, f3, line1, line2;
	int a, b;
	double w1, w2;
	TCHAR t1[MAX_PATH], t2[MAX_PATH];
	if (first < last) {
		int mid = (first + last) / 2;
		f1 = merge(first, mid);
		f2 = merge(mid + 1, last);
		file1.open(f1);
		file2.open(f2);
		if (f1.size() + f2.size() >= 50) {
			f3 = f1.substr(0, f1.size() / 2) + "_" + f2.substr(0, f2.size() / 2);
		}
		else {
			f3 = f1 + "_" + f2;
		}
		res_file.open(f3);

		getline(file1, line1);
		getline(file2, line2);
		while (!file1.eof() || !file2.eof()) {
			if (file1.eof()) {
				res_file << line2 << endl;
				getline(file2, line2);
			}
			else if (file2.eof()) {
				res_file << line1 << endl;
				getline(file1, line1);
			}
			else {
				a = stoi(line1.substr(0, 6));
				w1 = stod(line1.substr(15, 7));
				b = stoi(line2.substr(0, 6));
				w2 = stod(line2.substr(15, 7));
				if (a < b) {
					res_file << line1 << endl;
					getline(file1, line1);
				}
				else if (a > b){
					res_file << line2 << endl;
					getline(file2, line2);
				}
				else if (a == b) {
					if (w1 > w2) {
						res_file << line1 << endl;
						getline(file1, line1);
					}
					else {
						res_file << line2 << endl;
						getline(file2, line2);
					}
				}
			}
		}
		wstring tmp1(f1.begin(), f1.end());
		wstring tmp2(f2.begin(), f2.end());
		GetFullPathName(tmp1.c_str(), MAX_PATH, t1, NULL);
		GetFullPathName(tmp2.c_str(), MAX_PATH, t2, NULL);
		file1.close();
		file2.close();

		DeleteFile(t1);
		DeleteFile(t2);
		res_file.close();
		return f3;
	}
	else {
		return to_string(first);
	}
}

void external_merge_sort(ifstream& pre_file) {
	string line;
	vector<inverted_index> local;
	vector<string> tokens;
	TCHAR temp[MAX_PATH];
	int file_cnt = 0;
	ofstream out;
	int cnt = 0;

	// PASS 1
	while (getline(pre_file, line)) {
		if (cnt < 1500000) {
			local.push_back(inverted_index(stoi(line.substr(0, 6)), stod(line.substr(15, 7)), line.substr(6, 9)));
			++cnt;
		}
		else {
			sort(local.begin(), local.end());
			out.open(to_string(file_cnt));
			for (int i = 0; i < local.size(); ++i) {
				out << utility::format_digit(6, local[i].word_id) << local[i].data << utility::format_weight(local[i].weight) << endl;
			}
			out.close();
			local.clear();
			++file_cnt;
			cnt = 1;
			local.push_back(inverted_index(stoi(line.substr(0, 6)), stod(line.substr(15, 7)), line.substr(6, 9)));
		}
	}
	out.open(to_string(file_cnt));
	sort(local.begin(), local.end());
	for (int i = 0; i < local.size(); ++i) {
		out << utility::format_digit(6, local[i].word_id) << local[i].data << utility::format_weight(local[i].weight) << endl;
	}
	++file_cnt;
	out.close();

	// PASS 2
	string final_file = merge(0, file_cnt - 1);
	ifstream from(final_file);
	out.open("Index.dat");
	while (getline(from, line)) {
		out << line << endl;
	}
	out.close();
	from.close();
	GetFullPathName(wstring(final_file.begin(), final_file.end()).c_str(), MAX_PATH, temp, NULL);
	DeleteFile(temp);
}

bool if_exists(string name) {
	ifstream f(name.c_str());
	return f.good();
}

void load_term_file(ifstream& file) {
	string line, token;
	vector<string> v;
	while (getline(file, line)) {
		v = utility::tokenizer(line, '\t');
		word_data_table[v[1]].id = stoi(v[0]);
		word_data_table[v[1]].df = stoi(v[2]);
		word_data_table[v[1]].cf = stoi(v[3]);
		word_data_table[v[1]].start = stoi(v[4]);
	}
}

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
map<int, map<string, int>> read_query_file(ifstream& file) {
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
					process_query_word(word, queries[query_num], 5);
				}
			}
		}
		else if (tokens[0] == "<desc>") { continue; }
		else if (tokens[0] == "<narr>") { continue; }
		else {
			for (auto s : tokens) {
				utility::tokenize_only_alpha(s, sp_char_removed);
				for (auto& word : sp_char_removed) {
					process_query_word(word, queries[query_num], 1);
				}
			}
		}
	}
	return queries;
}

int get_doc_length(ifstream& doc_file, int doc_id) {
	string line;
	doc_file.seekg((unsigned long long)(doc_id - 1)*(6 + 16 + 4 + 2));
	getline(doc_file, line);

	return stoi(line.substr(22, 4));
}

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
		if (df > 30000) df = 10000;
		for (int i = 0; i < df; ++i) {
			getline(iindex, line);
			id = stoi(line.substr(6, 6));
			tf = stoi(line.substr(12, 3));
			weight = stod(line.substr(15, 7));
			if (iiinfo.find(q.first) == iiinfo.end()) {
				map<int, pair<int,double>> doc_weight;
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

vector< pair<double, int> > vector_space_model(ifstream& iindex, set<int>& rel_docs, map<string, int>& query) {
	map<int, pair<double, double> > doc_cos_dist;
	map<int, pair<double, double> >::iterator it;
	vector < pair<double, int > > result_vector;
	string line;
	int df, doc_id, start;
	double weight;

	for (auto &word : query) {
		df = word_data_table[word.first].df;
		start = word_data_table[word.first].start;
		iindex.seekg((unsigned long long)start*(6 + 6 + 3 + 7 + 2), iindex.beg);
		for (int i = 0; i < df; ++i) {
			getline(iindex, line);
			doc_id = stoi(line.substr(6, 6));
			// If it's not in the relevant documents set, do not count in.
			if (rel_docs.find(doc_id) == rel_docs.end()) continue;
			weight = stod(line.substr(15, 7));
			if ((it = doc_cos_dist.find(doc_id)) == doc_cos_dist.end()) {
				doc_cos_dist.insert(make_pair(doc_id, make_pair((word.second * weight), weight * weight)));
			}
			else {
				it->second.first += word.second * weight;
				it->second.second += weight * weight;
			}
		}
	}

	for (auto res : doc_cos_dist) {
		result_vector.push_back(make_pair((res.second.first / sqrt(res.second.second)), res.first));
	}
	if (result_vector.size() > 0)
		sort(result_vector.begin(), result_vector.end(), greater<pair<double, int>>());
	return result_vector;
}

string get_doc_name(ifstream& doc_file, int doc_id) {
	string line;
	doc_file.seekg((unsigned long long)(doc_id - 1)*(6 + 16 + 4 + 2));
	getline(doc_file, line);

	return line.substr(6, 16);
}

vector<pair<double, int>> language_model(ifstream& doc_file, set<int>& rel_docs, map<string, int>& query, map<string, map<int, pair<int, double>>>& iindex_mem) {
	map<int, double> dirichlet;
	map<int, int> doc_length;
	long double mu = 2000;

	for (auto& doc_id : rel_docs) {
		// Get document's length from the Doc.dat file and store in on the memory.
		doc_length[doc_id] = get_doc_length(doc_file, doc_id);
		for (auto& qword : query) {
			// If the query word is not in the document,
			if (iindex_mem[qword.first].find(doc_id) == iindex_mem[qword.first].end()) {
				dirichlet[doc_id] += log((mu * word_data_table[qword.first].cf / C) / (mu + doc_length[doc_id]));
			}
			else {
				// If the query word is in the document,
				dirichlet[doc_id] += log((iindex_mem[qword.first][doc_id].first + mu * word_data_table[qword.first].cf / C) / (mu + doc_length[doc_id]));
			}
		}
	}

	vector<pair<double, int>> result_set;
	for (auto& res : dirichlet) {
		result_set.push_back(make_pair(res.second, res.first));
	}
	sort(result_set.begin(), result_set.end(), greater<pair<double, int>>());
	return result_set;
}

void print_result(ifstream& docu_dat_file, vector<pair<double, int>>& result,
	map<string, int>& query, int queryNum, ofstream& outfile) {
	outfile << queryNum << endl;
	for (auto &res : result) {
		outfile << get_doc_name(docu_dat_file, res.second) << "\t";
	}
	outfile << endl;
}

int main(int argc, char* argv[]) {
	// initialize irregular word dictionary which will be used in stemming.
	Porter2Stemmer::construct_irr_verbs_dictionary("irregular_word.txt");
	// construct stopword set
	constructStopWordSet();

	string token;
	vector<string> files;
	ifstream file;
	ofstream data_file;
	ofstream output;

	utility::get_file_paths(TEXT(".\\data"), files);	// Get file paths

	// Preprocessing
	if (!if_exists("Term.dat") && !if_exists("Doc.dat")) {
		// Open all files, do parsing, stemming, and index creation
		data_file.open(doc_data_fname);
		output.open(tf_filename);

		cout << "파싱 & 스테밍 & 단어 정보 생성 시작" << endl;
		cout << "Doc.dat(문서 정보 파일) 생성 시작" << endl;
		for (int i = 0; i < files.size(); ++i) {
			file.open(files[i]);
			cout << files[i] << "...";
			if (file.is_open()) {
				fast_parse(file, data_file, output);
				file.close();
			}
			cout << '\t' << "처리 완료" << endl;
		}
		cout << "문서 정보 파일 생성 완료" << endl << endl;
		/*
			Freeing memory.
		*/
		data_file.close();
		output.close();
		stopwords.clear();

		cout << "Term.dat(단어 정보 파일) 생성 시작";
		// Create Word dictionary.
		data_file.open(word_data_fname);
		create_word_data_file(data_file);
		data_file.close();
		cout << "\t생성 완료" << endl << endl;
	}
	if (!if_exists("Index.dat")) {
		if (!if_exists("TF.txt") && if_exists("Term.dat")) {
			// Load Term.dat
			cout << "Term.dat already exists. Now Loading...";
			file.open("Term.dat");
			load_term_file(file);
			file.close();
			cout << "\tLoading Finished." << endl;
		}
		// Create Inverse index file.
		if (if_exists("TF.txt")) {
			if (word_data_table.empty()) {
				// Load Term.dat
				cout << "Term.dat already exists. Now Loading...";
				file.open("Term.dat");
				load_term_file(file);
				file.close();
				cout << "\tLoading Finished." << endl;
				doc_count = 554028;
			}
			if (!if_exists(inverse_data_fname)) {
				data_file.open(inverse_data_fname);
				cout << "Index.dat (역 색인 파일) 생성...";
				create_inverted_index_file(data_file);
				cout << "\t" << "생성완료" << endl;
				data_file.close();
			}
		}
		//SORT Invert.dat
		cout << "정렬 시작" << endl;
		file.open(inverse_data_fname);
		external_merge_sort(file);
		file.close();
		cout << "정렬 끝" << endl;
	}
	else {
		cout << "Loading Term.dat...";
		// Load Term.dat
		file.open("Term.dat");
		load_term_file(file);
		file.close();
		cout << "Finished Loading Term.dat." << endl;
	}

	/*
		Count words in the collection. Used in smoothing. (f(qi, C) = C(qi) / |C|)
	*/
	for (auto word : word_data_table) {
		C += word.second.cf;
	}

	cout << "Total number of words in the collection (allow duplicates) : " << C << endl << endl;

	cout << "Processing Queries...";
	// Query Processing
	file.open("topics25.txt");
	// Query NO., word, TF(query), weight(query)
	map< int, map<string, int>> queries = read_query_file(file);
	file.close();

	// Print out queries
	/*for (auto& query : queries) {
		cout << query.first << endl;
		for (auto& word : query.second) {
			cout << word.first << " " << word.second << endl;
		}
		cout << endl;
	}*/

	cout << '\t' << "Done. Total : " << queries.size() << endl;

	cout << endl << endl << "Clock start..." << endl;
	// Time stamp
	time_point<system_clock> start, end;
	start = system_clock::now();

	ifstream docu_dat_file(doc_data_fname);
	ifstream iindex("Index.dat");
	ofstream outfile("result.txt");

	// Vector Space Model
	//for (auto &query : queries) {
	//	set<int> rel_docs = get_relevant_documents(iindex, query.second);
	//	vector< pair<double, int> > result = vector_space_model(iindex, rel_docs, query.second);
	//	// Print Vector Space Model Results in a relevance order.
	//	print_result(docu_dat_file, result, query.second, query.first, outfile);
	//}
	//outfile.close();

	//cout << "Vector Space Model 완료." << endl;

	// Language Model (Dirichlet Smoothing)
	for (auto &query : queries) {
		map<string, map<int, pair<int, double>>> iindex_memory;
		set<int> rel_docs = get_relevant_documents(iindex, query.second, iindex_memory);
		//vector<pair<long double, int>> result = language_model(docu_dat_file, rel_docs, query.second, iindex_memory);
		vector<pair<double, int>> result = language_model(docu_dat_file, rel_docs, query.second, iindex_memory);
		//vector<pair<double, int>> result = language_model(docu_dat_file, iindex, rel_docs, query.second);
		// Print Language Model Results in a relevance order.
		print_result(docu_dat_file, result, query.second, query.first, outfile);
	}
	outfile.close();

	cout << "Language Model 완료." << endl;

	docu_dat_file.close();

	end = system_clock::now();
	duration<double> elapsed_seconds = end - start;
	time_t end_time = system_clock::to_time_t(end);
	cout << "finished computation at " << ctime(&end_time)
		<< "elapsed time: " << elapsed_seconds.count() << "s\n";
	return 0;
}