#include "def.h"

DEFINE_string(input_file, "", "Input data file");
DEFINE_double(threshold, 0.99, "Threshold");
DEFINE_string(method, "allps", "Join algorithm");
DEFINE_int64(max_depth, -1, "Max depth for ppj+, mpj+, qj+, gmpj+ and aj+o");
DEFINE_uint64(threads, 1, "Number of worker threads");
DEFINE_int32(inline_records, 0, "Should TIDs be integrated into the record structs");
DEFINE_int32(batch_size, 100, "allps: Batch size");
DEFINE_int32(pos_filter, 0, "If AllPairs should use the positional filter (=PPJ)");
DEFINE_int32(scale, 1, "for process. scale up the datasets");
DEFINE_string(indexLengths, "indexLengths", "Index Lengths comma-separated");
DEFINE_string(probeLengths, "probeLengths", "Probe Lengths comma-separated");
DEFINE_int32(mod, 0, "modulo: For 2 groups choose 2");
DEFINE_int32(modgroup, 0, "modulo group");

Relation R;
vector<GroupRecord> gR;
InvertedIndex idxR;
InvertedGroupIndex gidxR;
InvertedIndex *didxR;

#ifdef INVERTED_LIST_CPP_VECTOR
int *startingPosition;
int *startingGPosition;
#elif defined INVERTED_LIST_CPP_LIST
list<index_entry>::iterator *startingPosition;
list<gindex_entry>::iterator *startingGPosition;
#elif defined INVERTED_LIST_MY_LIST
index_entry **startingPosition;
gindex_entry **startingGPosition;
#endif

int numRecords, numGroupRecords, numKeywords, numGroupKeywords;
double sqrThreshold;
unsigned long long numCandidates, numResults;

#ifdef STATISTICS
Statistics *stats;
#endif

#ifdef OVERLAP_SHEN
PseudoHash *row, *col;
PseudoMatrixHash *overlap_max;
PseudoHash *Overlap;
#elif defined OVERLAP_STL
hash<int, int> *Overlap;
#elif defined OVERLAP_MYHASH
MyHash *Overlap;
MyHash *DeltaOverlap;
#endif

int maxDepth, maxPrefixSchemeDegree;

vector<vector<Record *>> bagsR;
unsigned long long total_duration = 0;

void Reset() {
#ifdef OVERLAP_SHEN
  Overlap = new PseudoHash(numRecords);
#elif defined OVERLAP_STL
  Overlap = new hash<int, int>();
#elif defined OVERLAP_MYHASH
  Overlap = new MyHash(numRecords);
  DeltaOverlap = new MyHash(numRecords);
#endif

  if (FLAGS_inline_records == 0) {
    for (int rid = 0; rid < numRecords; rid++)
      R[rid].validIndexPrefixLength = R[rid].indexPrefixLength;
  }

  for (int grid = 0; grid < numGroupRecords; grid++) {
    delete[] gR[grid].keywords;
    bagsR[grid].clear();
  }
  gR.clear();
  bagsR.clear();

  idxR.clear();
  gidxR.clear();
  if (didxR) {
    for (int l = 0; l < maxPrefixSchemeDegree; l++)
      didxR[l].clear();
  }
}

void CleanUp() {
#ifdef STATISTICS
  delete stats;
#endif

  if (FLAGS_inline_records == 0) {
    for (int rid = 0; rid < numRecords; rid++)
      delete[] R[rid].keywords;
    delete[] R;
  } else { // TODO: free recs and recIx

  }


  for (int grid = 0; grid < numGroupRecords; grid++)
    delete[] gR[grid].keywords;

  delete Overlap;
  delete DeltaOverlap;

  if (didxR)
    delete[] didxR;

  delete[] startingPosition;
  delete[] startingGPosition;

#ifdef OVERLAP_SHEN
  if (row) {
    delete row;
    delete col;
    delete overlap_max;
  }
#endif
}

#ifdef STATISTICS
void PrintStatistics() {
  CountGroupKeywords();

  cout << endl;
  cout << "DATASET AND INDEX STATS" << endl;
  cout << "\tInput File.................: " << FLAGS_input_file << endl;
  cout << "\t# Records..................: " << numRecords << endl;
  cout << "\t# Keywords.................: " << numKeywords << endl;
  cout << "\tAverage Record Length......: " << stats->avgRecordLength << endl;
  cout << "\t# Group Records............: " << numGroupRecords << endl;
  cout << "\t# Keywords.................: " << numGroupKeywords << endl;
  cout << "\tAverage Group Record Length: " << stats->avgGroupRecordLength
       << endl;
  //	cout << "\tAverage Index Prefix Length : " <<
  //(float)stats->totalIndexPrefixLength/numRecords << endl; 	cout <<
  //"\tAverage Probe Prefix Length : " <<
  // (float)stats->totalProbePrefixLength/numRecords
  //<< endl;
  cout << "\tAverage Index List Length..: " << stats->avgNumIndexEntries
       << endl;
  cout << endl;
  cout << "PHASE TIME STATS" << endl;
  cout << "\tGrouping Time..............: " << (double)stats->grouping_duration
       << endl; // (CLOCKS_PER_SEC)
  cout << "\tIndexing Time..............: " << (double)stats->indexing_duration
       << endl; // (CLOCKS_PER_SEC)
  cout << "\tFiltering Time.............: " << (double)stats->filtering_duration
       << endl; // (CLOCKS_PER_SEC)
  cout << "\tVerification Time..........: "
       << (double)stats->verification_duration << endl; // (CLOCKS_PER_SEC)
  cout << endl;
  cout << "JOIN STATS" << endl;
  cout << "\tMethod.....................: " << FLAGS_method << endl;
  cout << "\tSuffixFilter Max Depth.....: " << maxDepth << endl;
  cout << "\tSimilarity Measure.........: Jaccard" << endl;
  cout << "\tSimilarity Threshold.......: " << FLAGS_threshold << endl;
  cout << "\t# Candidates...............: " << numCandidates << endl;
  cout << "\t# Results..................: " << numResults << endl;
  cout << "\tCPU Time (sec).............: "
       << (double)(stats->grouping_duration + stats->indexing_duration +
                   stats->filtering_duration + stats->verification_duration)
       << endl; // /(CLOCKS_PER_SEC)
  cout << "\tTotal Duration.............: " << total_duration / 1000.0 << endl;
  // cout << "\tPhysical Time (sec)........: " << double(timeEnd.tv_sec -
  // timeStart.tv_sec) + double(timeEnd.tv_usec - timeStart.tv_usec) / 1e6 <<
  // endl;
  cout << endl;
}
#endif

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  stringstream ss;

// Initialization.
#ifdef STATISTICS
  stats = new Statistics();
#endif

  sqrThreshold = FLAGS_threshold * FLAGS_threshold;
  numCandidates = numResults = 0;

  std::cout << "NumThreads=" << FLAGS_threads << std::endl;

  // Load records: we do not count this time:
  LoadRelationOptimized(FLAGS_input_file.c_str());

  // Perform join.
  FLAGS_method = "AllPairsHyperThread";
  Reset();
  Timer t;
  t.Reset();

  BuildIndex_PPJOIN();

  AllPairsHyperThread();
  total_duration += t.GetElapsedTimeMs();


#ifdef STATISTICS
  PrintStatistics();
#endif

  CleanUp();

  return 0;
}
