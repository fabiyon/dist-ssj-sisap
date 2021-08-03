#include "def.h"

std::vector<int> probeLengths;
std::vector<int> indexLengths;

void LoadRelationOptimized(const char *filename) {
  char c;
  stringstream ss;
  FILE *fp = 0;
  int recid = 0, keywordIdx = 0;
  unsigned long long totalLength = 0;
  std::set<int> allLengths;
  int maxLength;

  std::istringstream iss(FLAGS_probeLengths);
  for (std::string token; std::getline(iss, token, ','); )
  {
      int tmp = std::stoi(std::move(token));
      probeLengths.push_back(tmp);
      allLengths.insert(tmp);
      maxLength = std::max(maxLength, tmp);
  }

  std::istringstream issIx(FLAGS_indexLengths);
  for (std::string token; std::getline(issIx, token, ','); )
  {
      int tmp = std::stoi(std::move(token));
      indexLengths.push_back(tmp);
      allLengths.insert(tmp);
      maxLength = std::max(maxLength, tmp);
  }


  std::string line;

  std::cout << "Loading statistics for: " << filename << std::endl;
  std::string statfilename = filename; // const char* wird hier gecastet
  statfilename += "-stat";
  std::ifstream statfile(statfilename);
  numRecords = 0;
  while (std::getline(statfile, line)) {
    stringstream linestream(line);
    std::string tmpString;
    std::getline(linestream, tmpString, ',');
    int length = std::stoi(tmpString);
    std::getline(linestream, tmpString, ',');
    int count = std::stoi(tmpString);
    if (allLengths.find(length) != allLengths.end()) {
      numRecords += count;
    }
  }

  std::cout << "Loading relation: " << filename << std::endl;
  std::ifstream file(filename);
  // first line is special:
  std::getline(file, line);
  int unused2;
  int unused1;
  sscanf(line.c_str(), "%d %d %d\n", &unused1, &numKeywords, &unused2); // numKeywords is the number of distinct keywords, numTids is the total number of keywords
  R = new Record[numRecords];
  int tidCnt = 0;
  maxPrefixSchemeDegree = 0;
  int modCount = 0; // we use this to prune records in our mod filter
  while (std::getline(file, line)) {
    stringstream linestream(line);

    // 1. Zerteile mit \t:
    std::string tmpString;
    std::getline(linestream, tmpString, '\t');
    int length = std::stoi(tmpString);
    if (length > maxLength) {
      break;
    }
    if (allLengths.find(length) == allLengths.end()) {
      continue;
    }

    bool isProbeRecord = false;
    if (FLAGS_mod != 0) { 
      if (std::find(indexLengths.begin(), indexLengths.end(), length) == indexLengths.end()) {
        if (modCount++ % FLAGS_mod != FLAGS_modgroup) {
          continue;
        }
        isProbeRecord = true;
      } else if (std::find(probeLengths.begin(), probeLengths.end(), length) != probeLengths.end()) {
        if (modCount++ % FLAGS_mod == FLAGS_modgroup) {
          isProbeRecord = true;
        }
      }
    } else {
      isProbeRecord = true;
    }

    R[recid].length = length;
#ifdef STATISTICS
    totalLength += R[recid].length;
#endif
    R[recid].sqrtLength = sqrt(R[recid].length);
    R[recid].keywords = new int[R[recid].length];
    if (isProbeRecord) {
      R[recid].probePrefixLength =
        min(R[recid].length,
          (int)(R[recid].length - R[recid].length * FLAGS_threshold + EPS) + 1);
    } else {
      R[recid].probePrefixLength = 0; // aborts the probe for this record
    }
    R[recid].indexPrefixLength =
      min(R[recid].length,
        (int)(R[recid].length -
          R[recid].length * 2.0 * FLAGS_threshold / (1.0 + FLAGS_threshold) + EPS) +
          1);
    maxPrefixSchemeDegree = max(maxPrefixSchemeDegree, int(R[recid].length * FLAGS_threshold + EPS) + 1);

    while (std::getline(linestream, tmpString, ',')) {
      R[recid].keywords[keywordIdx] = std::stoi(tmpString);
      keywordIdx++;
    }
    R[recid].id = recid; // If you just have a pointer to this record object it is
          // useful to be able to determine it's record ID.
    recid++;
    keywordIdx = 0;
  } // end while

  numRecords = recid;

#ifdef STATISTICS
  stats->avgRecordLength = (float)totalLength / numRecords;
#endif
  std::cout << "Loaded " << numRecords << " records" << std::endl;
}

void PrintRecord(Record *r, int limit) {
  cout << "r" << r->id << ", length = " << r->length << ":";
  for (int j = 0; j < limit; j++)
    cout << " " << r->keywords[j];
  cout << endl;
}

void PrintRecord(Record *r) {
  cout << "r" << r->id << ":";
  for (int j = 0; j < r->length; j++)
    cout << " " << r->keywords[j];
  cout << endl;
}


