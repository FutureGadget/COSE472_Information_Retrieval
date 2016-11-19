#include "TextTransform.h"
#include "utility.h"
#include "Entities.h"
#include "Parser.h"
#include "porter2_stemmer.h"
#include "FileIO.h"
#include "Constants.h"

#include <map>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

using namespace std;
using namespace utility;
/**
Word index table can be located in the memory. (Heaps' law)
*/
unordered_map<string, word_info> word_data_table;

/*
Store stop words in the unordered_set for a fast comparison.
*/
unordered_set<string> stopwords;

unsigned int doc_count = 0;	// document id (Which means the number of documents)

static void constructStopWordSet() {
	ifstream file;
	file.open(stopword_fname);
	if (file.is_open()) {
		string line;
		while (getline(file, line)) {
			stopwords.insert(line);
		}
		file.close();
	}
}

void preprocessing() {
	constructStopWordSet();
	Porter2Stemmer::construct_irr_verbs_dictionary(irregular_dictionary_fname);
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

void text_transform_indexing(LPCWSTR path) {
	ofstream data_file;
	ofstream output;
	ifstream file;
	vector<string> files;
	if (!if_exists("Term.dat") && !if_exists("Doc.dat")) {
		// Get file paths
		utility::get_file_paths(path, files);

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
		data_file.close();
		output.close();

		cout << "Term.dat(단어 정보 파일) 생성 시작";
		// Create Word dictionary.
		data_file.open(word_data_fname);
		create_word_data_file(data_file);
		data_file.close();
		cout << "\t생성 완료" << endl << endl;
	}

	// Invert indexing
	if (!if_exists("Index.dat")) {
		if (!if_exists("TF.txt") && if_exists("Term.dat")) {
			// Load Term.dat
			cout << "Term.dat already exists. Now Loading...";
			file.open("Term.dat");
			load_term_file(word_data_fname);
			file.close();
			cout << "\tLoading Finished." << endl;
		}
		// Create Inverse index file.
		if (if_exists("TF.txt")) {
			if (word_data_table.empty()) {
				// Load Term.dat
				cout << "Term.dat already exists. Now Loading...";
				file.open("Term.dat");
				load_term_file(word_data_fname);
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
		load_term_file(word_data_fname);
		file.close();
		cout << "Finished Loading Term.dat." << endl;
	}
}

void fast_parse(ifstream& file, ofstream& doc_file, ofstream& tf_file) {
	string line;
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
				}
				else if (line == "<HEADLINE>") {
					getline(file, line);
					while (line != "</HEADLINE>") {
						// Parse HEADLINE
						text_transform(line, tempTF, PARSE_headline_weight, doc_length);
						getline(file, line);
					}

				}
				else if ((fidx = line.find("<HEADLINE>")) != string::npos) {
					tmp = line.substr(fidx + 11, line.size() - 10 - 11);
					utility::trim(tmp);
					text_transform(tmp, tempTF, PARSE_headline_weight, doc_length);
					getline(file, line);
				}
				else if (line == "<TEXT>") {
					getline(file, line);
					while (line != "</TEXT>") {
						if (line == "<P>") {
							getline(file, line);
							while (line != "</P>") {
								// Parse Paragraph
								text_transform(line, tempTF, PARSE_normal_text_weight, doc_length);
								getline(file, line);
							}
						}
						else if (line == "<SUBHEAD>") {
							getline(file, line);
							while (line == "</SUBHEAD>") {
								// Parse SUBHEAD
								text_transform(line, tempTF, PARSE_subhead_weight, doc_length);
								getline(file, line);
							}
						}
						else if (line[0] != '<') { // contents without a <P> tag
							text_transform(line, tempTF, PARSE_normal_text_weight, doc_length);
							getline(file, line);
						}
						else {
							getline(file, line);
						}
					}
				}
				else if (line[0] != '<') {
					text_transform(line, tempTF, PARSE_headline_weight, doc_length);
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