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
const char* stopword_fname = "stopwords";
const char* stemmed_fname = "TF.txt";
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
	string data;
	bool operator<(inverted_index& t) {
		return (this->word_id < t.word_id);
	}
	inverted_index() {}
	inverted_index(int word_id, string data) {
		this->word_id = word_id;
		this->data = data;
	}
};

int doc_count = 0;	// document id (Which means the number of documents)
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
	string two("aa");
	for (int i = 'a'; i <= 'z'; ++i) {
		for (int j = 'a'; j <= 'z'; ++j) {
			two[0] = i;
			two[1] = j;
			stopwords.insert(two);
		}
	}
}

bool is_valid_char(char c) {
	if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '<' || c == '>'
		|| c >= '0' && c <= '9' || c == '.' || c == '/') return true;
	return false;
}

void tokenize_only_alpha(string s, vector<string>& tokens) {
	string tmp;
	bool is_counting = false;
	tokens.clear();
	for (int i = 0; i < s.size(); ++i) {
		if (!is_counting && is_valid_char(s.at(i))) {
			is_counting = true;
			tmp = s.at(i);
		}
		else if (is_counting && is_valid_char(s.at(i))) {
			tmp += s.at(i);
		}
		else if (is_counting && !is_valid_char(s.at(i))) {
			is_counting = false;
			tokens.push_back(tmp);
		}
	}
	if (is_counting) {
		tokens.push_back(tmp);
	}
}

void create_index_file(vector<string>& buffer, ofstream& stem_output, int doc_id) {
	map<string, int> tf;
	for (auto word : buffer) {
		if (word_data_table.find(word) == word_data_table.end()) {
			word_data_table[word] = word_info();
		}
		word_data_table[word].cf++;
		if (tf.find(word) == tf.end()) {
			tf[word] = 1;
			word_data_table[word].df++;
		}
		else {
			tf[word]++;
		}
	}
	for (string word : buffer) {
		stem_output << doc_id << '\t' << word << '\t' << tf[word] << endl;
	}
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


void fast_parse(ifstream& file, ofstream& doc_data, ofstream& stem_output) {
	string line;
	vector<string> tokens;
	document* doc = NULL;
	multiset<string> found_in_the_current_doc; // To count the document length. Should be cleared per docs.
	vector<string> buffer;

	bool docno, head, text, is_parsing_docno, is_parsing_head, is_parsing_text;
	while (getline(file, line)) {
		tokenize_only_alpha(line, tokens);
		for (string word : tokens) {
			if (word == "<DOC>") {
				/*
					Initialization code
				*/
				doc = new document();
				doc_count = doc_count + 1;		  // increase doc_count
				docno = head = text = false;
				is_parsing_docno = is_parsing_head = is_parsing_text = false;
			}
			else if (word == "</DOC>") {
				// Write to the document data file
				doc_data << utility::format_digit(6,doc_count) << doc->docno << utility::format_digit(4,found_in_the_current_doc.size()) << endl;
				//write_stemmed_document(buffer, stem_output);
				create_index_file(buffer, stem_output, doc_count);
				/*
					initialize
				*/
				buffer.clear();
				found_in_the_current_doc.clear(); // Clear the multi set to store words for the new documents.
				delete(doc);
			}
			else {
				if (!docno && word == TAG_DOCNO) {
					is_parsing_docno = true;
				}
				else if (!head && word == TAG_HEADLINE) {
					is_parsing_head = true;
				}
				else if (!text && word == TAG_TEXT) {
					is_parsing_text = true;
				}
				else if (!docno && is_parsing_docno && word == TAG_DOCNO_END) {
					docno = true;
					is_parsing_docno = false;
				}
				else if (!head && is_parsing_head && word == TAG_HEADLINE_END) {
					head = true;
					is_parsing_head = false;
				}
				else if (!text && is_parsing_text && word == TAG_TEXT_END) {
					text = true;
					is_parsing_text = false;
				}
				else if (is_parsing_docno) {
					doc->docno = word;
				}
				else if (is_parsing_head) {
					Porter2Stemmer::trim(word);
					if (word != "" && stopwords.find(word) == stopwords.end()) {
						Porter2Stemmer::stem(word);	// stem
						if (word != "" && stopwords.find(word) == stopwords.end()) {
							buffer.push_back(word);
							found_in_the_current_doc.insert(word);
						}
					}
				}
				else if (is_parsing_text) {
					Porter2Stemmer::trim(word);
					if (word != "" && stopwords.find(word) == stopwords.end()) {
						Porter2Stemmer::stem(word);	// stem
						if (word != "" && stopwords.find(word) == stopwords.end()) {
							buffer.push_back(word);
							found_in_the_current_doc.insert(word);
						}
					}
				}
			}
		}
	}
}

string merge(int first, int last) {
	ifstream file1, file2;
	ofstream res_file;
	string f1, f2, f3, line1, line2;
	int a, b;
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
				getline(file2, line2);
			}
			else {
				a = stoi(line1.substr(0, 6));
				b = stoi(line2.substr(0, 6));
				if (a < b) {
					res_file << line1 << endl;
					getline(file1, line1);
				}
				else {
					res_file << line2 << endl;
					getline(file2, line2);
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
			local.push_back(inverted_index(stoi(line.substr(0, 6)), line.substr(6, string::npos)));
			++cnt;
		}
		else {
			sort(local.begin(), local.end());
			out.open(to_string(file_cnt));
			for (int i = 0; i < local.size(); ++i) {
				out << utility::format_digit(6, local[i].word_id) << local[i].data << endl;
			}
			out.close();
			local.clear();
			++file_cnt;
			cnt = 1;
			local.push_back(inverted_index(stoi(line.substr(0, 6)), line.substr(6, string::npos)));
		}
	}
	out.open(to_string(file_cnt));
	sort(local.begin(), local.end());
	for (int i = 0; i < local.size(); ++i) {
		out << utility::format_digit(6, local[i].word_id) << local[i].data << endl;
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

void process_query_word(string& word, map<string, int>& Q) {
	Porter2Stemmer::trim(word);
	if (stopwords.find(word) != stopwords.end()) return;
	else {
		Porter2Stemmer::stem(word);
		if (stopwords.find(word) != stopwords.end() || word == "") return;
		else {
			// Query weight is calculated by its term frequency.
			if (Q.find(word) == Q.end()) {
				Q[word] = 1;
			}
			else {
				Q[word]++;
			}
		}
	}
}

/*
	1. Read query file and trim & stemming the query terms.
	2. Count query term frequency to calculate qi in the vector space model.
	@returns queries
*/
map< int, map<string, int> > read_query_file(ifstream& file) {
	string line;
	map<int, map<string, int> > queries;
	map<string, int> query;
	vector<string> tokens;
	int query_num;
	while (getline(file, line)) {
		tokens = utility::tokenizer(line, ' ');
		if (tokens.size() == 0) continue;
		else if (tokens[0] == "<top>" || tokens[0] == "</top>") continue;
		else if (tokens[0] == "<num>") {
			query_num = stoi(tokens[2]);
			queries.insert(make_pair(query_num, query));
		}
		else if (tokens[0] == "<title>") { for (int i = 1; i < tokens.size(); ++i) process_query_word(tokens[i], queries[query_num]); }
		else if (tokens[0] == "<desc>") { continue; }
		else if (tokens[0] == "<narr>") { continue; }
		else
			for (auto s : tokens) { process_query_word(s, queries[query_num]); };
	}
	return queries;
}

/*
	Relevant documents should contain at least n query terms of which the TF in that document is same or greater than k and TF in the query >= qk.
*/
set<int> get_relevant_documents(ifstream& iindex, map<string, int>& Q, int n, int k, int qk) {
	int id, df, start, tf;
	map<int, int> doc_freq_map;
	set<int> rel_docs;
	string line;
	for (auto q : Q) {
		if (q.second < qk) continue;
		if (word_data_table.find(q.first) == word_data_table.end()) continue;
		df = word_data_table[q.first].df;
		start = word_data_table[q.first].start;
		iindex.seekg((unsigned long long)start*(6 + 6 + 3 + 7 + 2), iindex.beg);
		for (int i = 0; i < df; ++i) {
			getline(iindex, line);
			id = stoi(line.substr(6, 6));
			tf = stoi(line.substr(12, 3));
			if (tf < k) continue;
			if (doc_freq_map.find(id) == doc_freq_map.end()) {
				doc_freq_map[id] = 1;
			}
			else {
				doc_freq_map[id]++;
			}
		}
	}

	for (auto x : doc_freq_map) {
		if (x.second >= n) {
			rel_docs.insert(x.first);
		}
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

	bool test = false;
	for (auto &word : query) {
		if (word_data_table.find(word.first) == word_data_table.end()) continue;
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

int get_doc_length(ifstream& doc_file, int doc_id) {
	string line;
	doc_file.seekg((unsigned long long)(doc_id - 1)*(6 + 16 + 4 + 2));
	getline(doc_file, line);

	return stoi(line.substr(22, 4));
}

string get_doc_name(ifstream& doc_file, int doc_id) {
	string line;
	doc_file.seekg((unsigned long long)(doc_id - 1)*(6 + 16 + 4 + 2));
	getline(doc_file, line);

	return line.substr(6, 16);
}

vector< pair<double, int> > language_model(ifstream& doc_file, ifstream& iindex, set<int>& rel_docs, map<string, int>& query) {
	map<int, double> dirichlet;
	double mu = 1500.0;
	int start, df, doc_id, dqtf, cqtf, D; // D : document length, dqtf : document query term frequency, cqtf : collection query term frequency
	string line;
	for (auto& word : query) {
		if (word_data_table.find(word.first) == word_data_table.end()) continue; // (?)
		df = word_data_table[word.first].df;
		start = word_data_table[word.first].start;
		iindex.seekg((unsigned long long)start*(6 + 6 + 3 + 7 + 2));
		for (int i = 0; i < df; ++i) {
			getline(iindex, line);
			doc_id = stoi(line.substr(6, 6));
			if (rel_docs.find(doc_id) == rel_docs.end()) continue;
			dqtf = stoi(line.substr(12, 3));
			cqtf = word_data_table[word.first].cf;
			D = get_doc_length(doc_file, doc_id);

			if (dirichlet.find(doc_id) == dirichlet.end()) {
				dirichlet.insert(make_pair(doc_id,
					log((dqtf + mu*(cqtf / C)) / (D + mu))));
			}
			else {
				dirichlet[doc_id] +=
					log((dqtf + mu*(cqtf / C)) / (D + mu));
			}
		}
	}
	vector<pair<double, int> > result_set;
	for (auto& res : dirichlet) {
		result_set.push_back(make_pair(res.second, res.first));
	}
	sort(result_set.begin(), result_set.end(), greater<pair<double, int> >());
	return result_set;
}

void evaluation() {
	map<int, set<string>> answer_set;
	map<int, set<string>> my_answer;
	ifstream answer("relevant_document.txt");
	ifstream myanswer("result.txt");
	string line;
	int id;
	
	while (getline(answer, line)) {
		vector<string> tokens = utility::tokenizer(line, '\t');
		id = stoi(tokens[0]);
		if (answer_set.find(id) == answer_set.end()) {
			set<string> docs;
			answer_set.insert(make_pair(id, docs));
		} else {
			answer_set[id].insert(tokens[1]);
		}
	}

	answer.close();

	while (getline(myanswer, line)) {
		vector<string> tokens = utility::tokenizer(line, '\t');
		id = stoi(tokens[0]);
		if (my_answer.find(id) == my_answer.end()) {
			set<string> docs;
			my_answer.insert(make_pair(id, docs));
		}
		else {
			my_answer[id].insert(tokens[1]);
		}
	}
	
	myanswer.close();

	// Compare
	int TP;
	int total;
	for (auto &myans : my_answer) {
		TP = 0;
		total = answer_set[myans.first].size();
		for (auto &mydoc : myans.second) {
			if (answer_set[myans.first].find(mydoc) != answer_set[myans.first].end()) TP++;
		}
		cout << "For query num : " << myans.first << endl;
		cout << "recall = " << TP / total << endl;
		cout << "precision = " << TP / myans.second.size() << endl << endl;
	}
}

void print_final_result(ifstream& docu_dat_file, vector<pair<double, int>>& result, 
	map<string, int>& query, int queryNum, ofstream& outfile) {
	outfile << "topicnum : " << queryNum << endl;
	int cnt = 5;
	for (auto& text : query) {
		if (cnt-- == 0) break;
		outfile << text.first << " ";
	}
	if (query.size() > 5) outfile << "..." << endl;

	for (auto &res : result) {
		outfile << get_doc_name(docu_dat_file, res.second) << "\t" << res.first << endl;
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
	
	// Time stamp
	time_point<system_clock> start, end;
	start = system_clock::now();

	// Preprocessing
	if (!if_exists("Term.dat") && !if_exists("Doc.dat")) {
		// Open all files, do parsing, stemming, and index creation
		data_file.open(doc_data_fname);
		output.open(stemmed_fname);

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
		// Create Inverse index file.
		data_file.open(inverse_data_fname);

		if (!data_file.is_open()) {
			cout << "Index.dat Open Error!" << endl;
			exit(-1);
		}

		cout << "Index.dat (역 색인 파일) 생성...";
		create_inverted_index_file(data_file);
		cout << "\t" << "생성완료" << endl;
		data_file.close();

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

	// Query Processing
	map< int, map<string, int> > queries;
	file.open("topics25.txt");
	queries = read_query_file(file);
	file.close();

	
	ifstream docu_dat_file(doc_data_fname);
	ifstream iindex("Index.dat");
	// Vector Space Model
	for (auto &query : queries) {
		set<int> rel_docs = get_relevant_documents(iindex, query.second, 2, 4, 3);
		vector< pair<double, int> > result = vector_space_model(iindex, rel_docs, query.second);

		// Print Vector Space Model Results in a relevance order.
		ofstream outfile("result_vsm.txt");
		print_final_result(docu_dat_file, result, query.second, query.first, outfile);
		outfile.close();
		//print_test_result(query, result);
	}

	// Language Model (Dirichlet Smoothing)
	for (auto &query : queries) {
		set<int> rel_docs = get_relevant_documents(iindex, query.second, 2, 4, 3);
		vector< pair<double, int> > result = language_model(docu_dat_file, iindex, rel_docs, query.second);

		// Print Vector Space Model Results in a relevance order.
		ofstream outfile("result_lm.txt");
		print_final_result(docu_dat_file, result, query.second, query.first, outfile);
		outfile.close();
		//print_test_result(query, result);
	}

	// TEST CODE
	/*for (auto &query : queries) {
		cout << query.first << endl;
		for (auto &test : query.second) {
			cout << test.first << " " << test.second << endl;
		}
	}*/

	iindex.close();
	docu_dat_file.close();

	end = system_clock::now();
	duration<double> elapsed_seconds = end - start;
	time_t end_time = system_clock::to_time_t(end);
	cout << "finished computation at " << ctime(&end_time)
		<< "elapsed time: " << elapsed_seconds.count() << "s\n";
	return 0;
}