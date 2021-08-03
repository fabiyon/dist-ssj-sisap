#include "def.h"

void BuildIndex_PPJOIN() {
  // std::cout << "==Start Index==" << std::endl;
  Record *r;
  int k, indexPrefixLength;
  index_entry entry;
  unsigned long numEntries = 0;
  InvertedIndex::iterator iterI;
  int currentLengthIndex = 0;
  int currentLength = indexLengths[0];

#ifdef STATISTICS
  // ctim.startTimer();
  struct timespec tstart, tfinish;
  clock_gettime(CLOCK_MONOTONIC, &tstart);
#endif

  for (int x = 0; x < numRecords; x++) {
    r = &R[x];

    bool doBreak = false;
    while (currentLength < r->length) {
      currentLengthIndex++;
      if (currentLengthIndex > indexLengths.size()) {
        doBreak = true;
        break;
      } else {
        currentLength = indexLengths[currentLengthIndex];
      }
    }
    if (doBreak) {
      break;
    }

    if (r->length != currentLength) {
      continue;
    }

    indexPrefixLength = r->indexPrefixLength;

    for (int i = 0; i < indexPrefixLength; i++) {
      // std::cout << ";" << std::endl;
      k = r->keywords[i];
      
      entry.rec = r;
      entry.position = i;

      idxR[k].push_back(entry); // Never initialized, but works. Adds element at
                                // the end. Problem: k assumes that the tokens
                                // start with 0 and are dense
      // startingPosition[k] = 0;  // Never initialized, but works. Optimizes index
                                // probes by ignoring shorter records.

      numEntries++;
    }
  }
  // std::cout << "==End Index==" << std::endl;

#ifdef STATISTICS
  // stats->indexing_duration += ctim.stopTimer();
  stats->avgNumIndexEntries = (float)numEntries / numKeywords;
  clock_gettime(CLOCK_MONOTONIC, &tfinish);
  stats->indexing_duration += (tfinish.tv_sec - tstart.tv_sec);
  stats->indexing_duration += (tfinish.tv_nsec - tstart.tv_nsec) / 1000000000.0;

#endif
}
