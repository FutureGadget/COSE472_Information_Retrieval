#include <chrono>
#include <ctime>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

#include "utility.h"
#include "QueryProcessing.h"
#include "TextTransform.h"
#include "RankingModule.h"
#include "Entities.h"
#include "FileIO.h"
#include "Constants.h"
#include "Parameters.h"

using namespace std;
using namespace std::chrono;

extern unsigned long long C; // total number of words in the collection (allow duplicates)
extern int doc_count;
extern unordered_map<string, word_info> word_data_table;
extern unordered_set<string> stopwords;

int main(int argc, char* argv[]) {
	// Transform text
	preprocessing();
	text_transform_indexing(TEXT(".\\data"));
	cout << "Indexing Finished.." << endl;

	// Time stamp
	time_point<system_clock> start, end;
	start = system_clock::now();

	// Load term file
	// load_term_file(word_data_fname);
	cout << "Total number of words in the collection (allow duplicates) : " << C << endl << endl;
	cout << endl << endl << "Clock start..." << endl;

	// Query Processing
	cout << "Processing Queries...";
	map< int, map<string, int>> queries = read_query_file("topics25.txt");

	// Print out queries
	/*for (auto& query : queries) {
		cout << query.first << endl;
		for (auto& word : query.second) {
			cout << word.first << " " << word.second << endl;
		}
		cout << endl;
	}*/

	cout << '\t' << "Done. Total : " << queries.size() << " queries" << endl;

	ifstream docu_dat_file(doc_data_fname);
	ifstream iindex("Index.dat");
	ofstream outfile("result.txt");

	// Vector Space Model
	//for (auto &query : queries) {
	//	map<string, map<int, pair<int, double>>> iiindex;
	//	set<int> rel_docs = get_relevant_documents(iindex, query.second, iiindex);
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
		vector<pair<double, int>> result;
		language_model(docu_dat_file, rel_docs, query.second, iindex_memory, result);
		// Print Language Model Results in a relevance order.
		utility::print_result(docu_dat_file, result, query.second, query.first, outfile);
	}
	outfile.close();
	docu_dat_file.close();

	cout << "Language Model 완료." << endl;

	end = system_clock::now();

	duration<double> elapsed_seconds = end - start;
	time_t end_time = system_clock::to_time_t(end);
	cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
	return 0;
}