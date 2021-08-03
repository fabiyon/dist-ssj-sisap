#pragma once

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <sys/mman.h>
#include <thread>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <string>
#include <utility>

//#define __UNIX_LINUX__

#ifdef __UNIX_LINUX__
#include <limits.h>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#else
#include <limits>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#endif

#include <algorithm>
#include <list>
#include <vector>
#include "timer.h"
#include <iostream>
#include <sstream>
#include <sys/time.h>

#ifdef __UNIX_LINUX__
#define hash tr1::unordered_map
#define shash tr1::unordered_set
#else
#define hash unordered_map
#define shash unordered_set
#endif

#define EPS 1e-8
//#define EPS
// std::numeric_limits<double>::epsilon()

#define JACCARD
//#define COSINE
//#define DICE
//#define OVERLAP

//#define PRINT_CANDIDATES
//#define PRINT_RESULTS
//#define PRINT_GROUP_RECORD_ID
#define STATISTICS
//#define PROGRESS_BAR

//#define OVERLAP_SHEN
//#define OVERLAP_STL
#define OVERLAP_MYHASH

//#define BUILD_INVERTED_INDEX_ONLINE

#define INVERTED_LIST_CPP_VECTOR
//#define INVERTED_LIST_CPP_LIST
//#define INVERTED_LIST_MY_LIST

#define CACHELINE_SIZE 64
#define CACHE_ALIGNED __attribute__((aligned(CACHELINE_SIZE)))
#define COMPILER_MEMORY_FENCE asm volatile("" ::: "memory")

using namespace std;

class MyHash {
public:
  int *probs;
  vector<pair<int, int>> data;

  MyHash(int numKeys) {
    this->probs = new int[numKeys];
    memset(this->probs, 0, numKeys * sizeof(int)); // fills the block with zeros
  };

  void clear() { this->data.clear(); };

  bool exists(int key) { return (this->probs[key] != 0); };

  void add(int key) {
    int one = 1;
    this->data.push_back(make_pair<int, int>(int(key), int(one)));
    this->probs[key] = this->data.size();
  };

  void increase(int key) { this->data[this->probs[key] - 1].second++; };

  int &operator[](int key) { this->data[this->probs[key] - 1].second; };

  void erase(int key) { this->probs[key] = 0; };

  ~MyHash() { delete[] this->probs; };
};

struct Record {
  int id;
  int length;
  float sqrtLength;
  int *keywords;
  int indexPrefixLength;
  int probePrefixLength;
  int validIndexPrefixLength;
  int requiredOverlap;
  int hammingDistanceThreshold;
  int xpos_mp;
  int ypos_mp;
  int optimalPrefixSchemeDegree;
  int tids[0];
};

struct GroupRecord {
  int id;
  int length;
  float sqrtLength;
  int *keywords; // If size = 1, this contains the keywords of the actual
                 // record, otherwise only the probe prefix should be used.
  int indexPrefixLength;
  int probePrefixLength;
  int validIndexPrefixLength;
  int requiredOverlap;
  int hammingDistanceThreshold;
  int xpos_mp;
  int ypos_mp;
  int xpos;
  int ypos;
  int size;
  Record *rec;
  int optimalPrefixSchemeDegree;
};

struct index_entry {
  Record *rec;
  int position;
  // index_entry *next;
};

struct gindex_entry {
  GroupRecord *rec;
  int position;
  gindex_entry *next;
};

typedef Record *Relation;

#ifdef INVERTED_LIST_CPP_VECTOR
typedef vector<index_entry> InvertedList;
typedef vector<gindex_entry> InvertedGList;
typedef hash<int, InvertedList> InvertedIndex;
typedef hash<int, InvertedGList> InvertedGroupIndex;
#elif defined INVERTED_LIST_CPP_LIST
typedef list<index_entry> InvertedList;
typedef list<gindex_entry> InvertedGList;
typedef hash<int, InvertedList> InvertedIndex;
typedef hash<int, InvertedGList> InvertedGroupIndex;
#elif defined INVERTED_LIST_MY_LIST
class InvertedList {
public:
  index_entry *head, *tail;
  int size;

  InvertedList(index_entry &e) {
    head = new index_entry;
    head->rec = e.rec;
    head->position = e.position;
    head->next = NULL;
    tail = head;
    size = 1;
  };

  void print() {
    index_entry *ptr = head;

    while (ptr != NULL) {
      cout << " r" << ptr->rec->id;
      ptr = ptr->next;
    }
    cout << endl;
  };

  void append(index_entry &e) {
    tail->next = new index_entry;
    tail = tail->next;
    tail->rec = e.rec;
    tail->position = e.position;
    tail->next = NULL;
    size++;
  };

  void erase(index_entry *e) {}

  ~InvertedList() {
    index_entry *ptr = head;
    while (ptr != NULL) {
    }
  }
};

class InvertedGList {
public:
  gindex_entry *head, *tail;
  int size;

  InvertedGList(gindex_entry &e) {
    head = new gindex_entry;
    head->rec = e.rec;
    head->position = e.position;
    head->next = NULL;
    tail = head;
    size = 1;
  };

  void print() {
    gindex_entry *ptr = head;

    while (ptr != NULL) {
      cout << " r" << ptr->rec->id;
      ptr = ptr->next;
    }
    cout << endl;
  };

  void append(gindex_entry &e) {
    tail->next = new gindex_entry;
    tail = tail->next;
    tail->rec = e.rec;
    tail->position = e.position;
    tail->next = NULL;
  };

  void erase(index_entry *e) {}

  ~InvertedGList() {
    gindex_entry *ptr = head;
    while (ptr != NULL) {
    }
  }
};

typedef hash<int, InvertedList *> InvertedIndex;
typedef hash<int, InvertedGList *> InvertedGroupIndex;
#endif

class Statistics {
public:
  float avgRecordLength;
  float avgGroupRecordLength;
  float avgProbePrefixLength;
  float avgIndexPrefixLength;
  float avgNumIndexEntries;
  double grouping_duration;     // unsigned long long
  double indexing_duration;     // unsigned long long
  double filtering_duration;    // unsigned long long
  double verification_duration; // unsigned long long

  Statistics() { this->Init(); };

  void Init() {
    this->avgRecordLength = 0;
    this->avgGroupRecordLength = 0;
    this->avgProbePrefixLength = 0;
    this->avgIndexPrefixLength = 0;
    this->avgNumIndexEntries = 0;
    this->grouping_duration = 0;
    this->indexing_duration = 0;
    this->filtering_duration = 0;
    this->verification_duration = 0;
  };
};

class PseudoHash {
public:
  int range;
  int size;
  int *map_array;
  int *keys;

  PseudoHash(int key_range) : range(key_range), size(0) {
    map_array = new int[key_range];
    memset(map_array, 0xff, key_range * sizeof(int));
    keys = new int[key_range];
  }

  ~PseudoHash() {
    delete[] map_array;
    delete[] keys;
  }

  int &operator[](int key_id) {
    if (map_array[key_id] < 0)
      keys[size++] = key_id;

    return map_array[key_id];
  }

  bool find(int key_id) { return (map_array[key_id] >= 0); }

  void clear() {
    for (int i = 0; i < size; i++)
      map_array[keys[i]] = -1;
    size = 0;
  }

  int get_map(int key_id) {
    if (map_array[key_id] < 0) {
      keys[size++] = key_id;
      map_array[key_id] = size - 1;
    }

    return map_array[key_id];
  }
};

// pseudo hash (int, int) to non-negative int
// the key range should be fixed and predefined
class PseudoMatrixHash {
public:
  int row_range;
  int range;

  int size;
  int *map_array;
  int *row_keys;
  int *col_keys;

  PseudoMatrixHash(int key_range)
      : row_range(key_range), range(key_range * key_range), size(0) {
    map_array = new int[range];
    memset(map_array, 0xff, range * sizeof(int));
    row_keys = new int[range];
    col_keys = new int[range];
  }

  ~PseudoMatrixHash() {
    delete[] map_array;
    delete[] row_keys;
    delete[] col_keys;
  }

  int &operator()(int row, int col) {
    int addr = col * row_range + row;
    if (map_array[addr] < 0) {
      col_keys[size] = col;
      row_keys[size] = row;
      size++;
    }
    return map_array[addr];
  }

  bool find(int row, int col) {
    int addr = col * row_range + row;
    return (map_array[addr] >= 0);
  }

  void clear() {
    for (int i = 0; i < size; i++) {
      int addr = col_keys[i] * row_range + row_keys[i];
      map_array[addr] = -1;
    }
    size = 0;
  }

  void inc_count(int row, int col) {
    int addr = col * row_range + row;
    if (map_array[addr] < 0) {
      col_keys[size] = col;
      row_keys[size] = row;
      size++;

      map_array[addr] = 1;
    } else
      map_array[addr]++;
  }

  bool if_zero(int row, int col) {
    int addr = col * row_range + row;
    if (map_array[addr] == 0)
      return true;
    else
      return false;
  }
};

extern bool allp_with_posfilter;
extern Relation R;
extern Record * recs; // holds all records including all tids
extern Record ** recIx; // holds the pointers to the records inside recs
extern vector<GroupRecord> gR;
extern InvertedIndex idxR;
extern InvertedGroupIndex gidxR;

#ifdef INVERTED_LIST_CPP_VECTOR
extern int *startingPosition;
extern int *startingGPosition;
#elif defined INVERTED_LIST_CPP_LIST
extern list<index_entry>::iterator *startingPosition;
extern list<gindex_entry>::iterator *startingGPosition;
#elif defined INVERTED_LIST_MY_LIST
extern index_entry **startingPosition;
extern gindex_entry **startingGPosition;
#endif

extern InvertedIndex *didxR;
extern int numRecords, numGroupRecords, numKeywords, numGroupKeywords;
extern double sqrThreshold;
extern unsigned long long numCandidates, numResults;

DECLARE_int32(inline_records);
DECLARE_int32(batch_size);
DECLARE_double(threshold);
DECLARE_uint64(threads);
DECLARE_int32(pos_filter);
DECLARE_int32(scale);
DECLARE_string(indexLengths);
DECLARE_string(probeLengths);
DECLARE_int32(mod);
DECLARE_int32(modgroup);

extern std::vector<int> probeLengths;
extern std::vector<int> indexLengths;

#ifdef OVERLAP_SHEN
extern PseudoHash *row, *col;
extern PseudoMatrixHash *overlap_max;
extern PseudoHash *Overlap;
#elif defined OVERLAP_STL
extern hash<int, int> *Overlap;
#elif defined OVERLAP_MYHASH
extern MyHash *Overlap;
extern MyHash *DeltaOverlap;
#endif

#ifdef STATISTICS
extern Statistics *stats;
#endif

extern int maxDepth, maxPrefixSchemeDegree;

// extern hash<int, vector<Record*> > bagsR;
extern vector<vector<Record *>> bagsR;

// IO functions.
void PrintRecord(Record *r);
void PrintRecord(Record *r, int limit);
void PrintGroupRecord(GroupRecord *gr);
void LoadRelationOptimized(const char *filename);
// void ProcessRelation(const char *filename);
// void LoadOptimalPrefixSchemeDegreeOracle(const char *filename);

// Tools functions.
double GetSimilarity(Record *rx, Record *ry);
bool QualifyTextual(Record *rx, Record *ry, int xstart, int ystart, int o);
bool QualifyTextual(Record *rx, Record *ry);
int SuffixFilter(int *xArray, int *yArray, int xStart, int xEnd, int yStart,
                 int yEnd, int HD, int depth);
void CreateGroupRecords();
void CreateGroupRecordsEqualLengths();
void CountGroupKeywords();
void VerifyGroupSelf(GroupRecord *gr);
void VerifyGroupSelfEqualLengthsPVLDB12(GroupRecord *gr);
void VerifyGroupSelfEqualLengths(GroupRecord *gr);
void VerifyGroupNonSelf(GroupRecord *grx, GroupRecord *gry, int overlap);
void VerifyGroupNonSelfEqualLengthsPVLDB12(GroupRecord *grx, GroupRecord *gry,
                                           int startx, int starty, int overlap);
void VerifyGroupNonSelfEqualLengths(GroupRecord *grx, GroupRecord *gry,
                                    int startx, int starty, int overlap);
void VerifyGroupNonSelfEqualLengths(GroupRecord *grx, GroupRecord *gry,
                                    int overlap);
// void CreateOracle();
void VerifyAllPairs(Record *rx, Record *ry, int overlap);
void VerifyAllPairsThread(Record *rx, Record *ry, int overlap,
                          uint64_t &n_candidates, uint64_t &n_results);
void Verify(Record *rx, Record *ry, int overlap);
void VerifyMPJoin(Record *rx, Record *ry, int overlap);
void Verify(GroupRecord *gr);
void VerifyPlus(GroupRecord *gr);
void VerifyMPJoin(GroupRecord *gr);
void Verify(GroupRecord *grx, GroupRecord *gry, int overlap);
void VerifyPlus(GroupRecord *grx, GroupRecord *gry, int overlap);
void VerifyMPJoin(GroupRecord *grx, GroupRecord *gry, int overlap);
void VerifyMPJoinPlus(GroupRecord *grx, GroupRecord *gry, int overlap);
bool CompareRecordsByLengthProbePrefixAndId(const Record *rx, const Record *ry);

// Filters.
bool QualifyLengthFilter(Record *rx, Record *ry);
bool QualifyLengthFilter(GroupRecord *grx, GroupRecord *gry);
bool QualifyPositionalFilter(Record *rx, int xpos, Record *ry, int ypos);
bool QualifyPositionalFilter(GroupRecord *grx, int xpos, GroupRecord *gry,
                             int ypos);
bool QualifySuffixFilter(Record *rx, int xpos, Record *ry, int ypos);
bool QualifySuffixFilter(GroupRecord *grx, int xpos, GroupRecord *gry,
                         int ypos);

// Indexing functions.
void BuildIndex_PPJOIN();

// Algorithms
void AllPairsHyperThread(); // allph - multithreaded, thread placement by OS


