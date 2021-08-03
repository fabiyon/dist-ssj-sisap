#include "def.h"

std::atomic<uint32_t> next_rid(0);
std::atomic<uint32_t> numberOfActiveThreads(0);

void generateAndVerifyCandidates()
{
  static const uint32_t kInvalidRid = ~uint32_t{0}; // largest number in 32 bit

  Record *rx, *ry;
  int k, ypos;
  InvertedIndex::iterator iterI;
  uint32_t rid = kInvalidRid;

  // uint32_t total_processed = 0;
  uint32_t batch_processed;

  uint64_t candidates = 0;
  uint64_t results = 0;
  thread_local std::unordered_map<Record *, int> overlaps;
  int * starting_position_local = new int[numKeywords]; // automatically nulled. We use the heap to not have trouble with too large token universes
  
  numberOfActiveThreads.fetch_add(1);

  while (true)
  {
    if (batch_processed == FLAGS_batch_size || rid == kInvalidRid)
    {
      rid = next_rid.fetch_add(FLAGS_batch_size); // get the next portion from the next_rid integer (=rid) and add 500 for the next thread 
      batch_processed = 0; // reset the counter
    }

    if (rid >= numRecords)
    {
      break;
    }
    
    ++batch_processed;

    // FILTERING:
    rx = &R[rid];

    // skip if length is not in the probe list:
    if (FLAGS_mod == 0 && std::find(probeLengths.begin(), probeLengths.end(), rx->length) == probeLengths.end()) {
      continue;
    }

    // Use multiple hashtabs to reduce possible search overhead, and make
    // them thread-local to avoid memory allocation pressure
    overlaps.clear();

    // Traverse probe prefix of the record.
    for (int xpos = 0; xpos < rx->probePrefixLength; xpos++)
    {
      k = rx->keywords[xpos];

      // Access inverted list of keyword k.
      int minLength = int(FLAGS_threshold * rx->length - EPS) + 1;
      if ((iterI = idxR.find(k)) != idxR.end())
      {
        //startingPosition[k]
        for (int i = starting_position_local[k]; i < iterI->second.size(); i++) // the index access is expensive. If the array is never incremented (all 0 values) we lose several seconds over just entering 0.
        {
          ry = (iterI->second)[i].rec;

          // length optimization: ignore larger records in the index:
          if (ry->length < minLength)
          {
            starting_position_local[k]++;
            continue;
          }
          else if (ry->length > rx->length)
          { // we can skip longer already indexed records. They will probe this record later anyway.
            break;
          }

          ypos = (iterI->second)[i].position;

          // To avoid duplicates, (x,y) and (y,x)
          if (rx->id <= ry->id)
            break;

          if (FLAGS_pos_filter == 1) {
            if (!QualifyPositionalFilter(rx, xpos, ry, ypos)) {
              continue;
            }
          }
          auto ret = overlaps.emplace(ry, 1);
          if (!ret.second)
          {
            // Insert didn't happen
            ++(ret.first->second);
          }

        }
      }
    }

    for (auto &x : overlaps)
    {
      uint64_t nc = 0, nr = 0;
      VerifyAllPairsThread(rx, x.first, x.second, nc, nr);
      candidates += nc;
      results += nr;
    }

    // Advance RID for the next record
    ++rid;
  }

  // Accumulate results to global counters
  __sync_fetch_and_add(&numResults, results); 
  __sync_fetch_and_add(&numCandidates, candidates);
  numberOfActiveThreads.fetch_sub(1);
  delete[] starting_position_local;
}

/**
 * allph
 * thread placement by OS
 * */
void AllPairsHyperThread() {
  std::vector<std::thread *> threads;
  for (uint32_t i = 0; i < FLAGS_threads; ++i) {
    std::thread * t = new std::thread(generateAndVerifyCandidates);
    threads.push_back(t);
  }

  for (uint32_t i = 0; i < FLAGS_threads; ++i) {
    threads[i]->join();
  }

  std::cerr << "completed.\n";
}

