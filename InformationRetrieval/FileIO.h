#pragma once
#include <fstream>
#include <string>
#include <map>

void create_word_data_file(std::ofstream& word_data_file);
void create_index_file(std::ofstream& doc_file, std::ofstream& tf_file, std::map<std::string, int>& tempTF, std::string docno, int doc_id, int doc_length);
void create_inverted_index_file(std::ofstream& iif);
void load_term_file(std::string filename);