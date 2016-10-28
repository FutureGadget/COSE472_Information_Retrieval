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

struct inverted_index{
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
		else if (is_counting && is_valid_char(s.at(i))){
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
		tf_idf_word[tokens[1]].first = stoi(tokens[2]);
		tf_idf_word[tokens[1]].second = (log(stod(tokens[2])) + 1) * log((double)doc_count / word_data_table[tokens[1]].df);
		denominator += pow(tf_idf_word[tokens[1]].second,2);
		// END OF FILE
		if (!getline(index_file, line)) {
			for (auto &word : tf_idf_word) {
				iif << utility::format_digit(6, word_data_table[word.first].id)
					<< utility::format_digit(6, doc_id)
					<< utility::format_digit(3, word.second.first)
					<< utility::format_weight(word.second.second / denominator)
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
					<< utility::format_weight(word.second.second / denominator)
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
				doc_data << doc_count << '\t' << doc->docno << '\t' << found_in_the_current_doc.size() << endl;
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
						}
					}
				}
				else if (is_parsing_text) {
					Porter2Stemmer::trim(word);
					if (word != "" && stopwords.find(word) == stopwords.end()) {
						Porter2Stemmer::stem(word);	// stem
						if (word != "" && stopwords.find(word) == stopwords.end()) {
							buffer.push_back(word);
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
		f3 = f1 + "_" + f2;
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
			local.push_back(inverted_index(stoi(line.substr(0,6)), line.substr(6,string::npos)));
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
	string final_file = merge(0, file_cnt-1);
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

double get_weight_for_word(ifstream& file, string word) {
	string line;
	int start = word_data_table[word].start;
	file.seekg(0, file.beg);
	file.seekg(start*(6 + 6 + 7 + 2), file.beg);
	getline(file, line);
	return stod(line.substr(12, string::npos));
}

int main(int argc, char* argv[]) {
	// options
	constructStopWordSet();
	Porter2Stemmer::construct_irr_verbs_dictionary("irregular_word.txt");

	string token;
	vector<string> files;
	ifstream file;
	ofstream data_file;
	ofstream output;

	// Time Stamp
	time_point<system_clock> start, end;
	start = system_clock::now();

	utility::get_file_paths(TEXT(".\\data"), files);	// Get file paths

	// Preprocessing
	if (!if_exists("Term.dat") && !if_exists("Doc.dat") && !if_exists("Index.dat")) {
		// Open all files, do parsing, stemming, and index creation
		data_file.open(doc_data_fname);
		output.open(stemmed_fname);

		cout << "�Ľ� & ���׹� & �ܾ� ���� ���� ����" << endl;
		cout << "Doc.dat(���� ���� ����) ���� ����" << endl;
		for (int i = 0; i < files.size(); ++i) {
			file.open(files[i]);
			cout << files[i] << "...";
			if (file.is_open()) {
				fast_parse(file, data_file, output);
				file.close();
			}
			cout << '\t' << "ó�� �Ϸ�" << endl;
		}
		cout << "���� ���� ���� ���� �Ϸ�" << endl << endl;
		/*
		Freeing memory.
		*/
		data_file.close();
		output.close();
		stopwords.clear();

		cout << "Term.dat(�ܾ� ���� ����) ���� ����";
		// Create Word dictionary.
		data_file.open(word_data_fname);
		create_word_data_file(data_file);
		data_file.close();
		cout << "\t���� �Ϸ�" << endl << endl;


		// Create Inverse index file.	
		data_file.open(inverse_data_fname);
		if (!data_file.is_open()) {
			cout << "Index.dat Open Error!" << endl;
			exit(-1);
		}

		cout << "Index.dat (�� ���� ����) ����...";
		create_inverted_index_file(data_file);
		cout << "\t" << "�����Ϸ�" << endl;
		data_file.close();

		//SORT Invert.dat
		cout << "���� ����" << endl;
		file.open(inverse_data_fname);
		external_merge_sort(file);
		file.close();
		cout << "���� ��" << endl;
	}
	else {
		cout << "Loading Term.dat...";
		// Load Term.dat
		file.open("Term.dat");
		load_term_file(file);
		file.close();
		cout << "Finished Loading Term.dat." << endl;
	}

	// Get weight
	/*file.open("Index.dat");
	cout << get_weight_for_word(file, "upgrad") << endl;
	file.close();*/

	// Query Parsing
	// Query structure
	struct query {
		map<string, int> qi;
	};
	vector<query> queries;
	file.open("topic25.txt");
	
	file.close();

	end = system_clock::now();
	duration<double> elapsed_seconds = end - start;
	time_t end_time = system_clock::to_time_t(end);
	cout << "finished computation at " << ctime(&end_time)
		<< "elapsed time: " << elapsed_seconds.count() << "s\n";

    return 0;
}