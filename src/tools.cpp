#include "def.h"

double GetSimilarity(Record *rx, Record *ry)
{
  int xcounter = 0, ycounter = 0, overlap = 0;

  while ((xcounter < rx->length) && (ycounter < ry->length))
  {
    if (rx->keywords[xcounter] == ry->keywords[ycounter])
    {
      overlap++;
      xcounter++;
      ycounter++;
    }
    else if (rx->keywords[xcounter] > ry->keywords[ycounter])
      ycounter++;
    else
      xcounter++;
  }

#ifdef JACCARD
  return ((double)overlap) / (rx->length + ry->length - overlap);
#elif defined COSINE
  return ((double)overlap) / (rx->sqrtLength * ry->sqrtLength);
#elif defined DICE
  return ((double)overlap) * 2 / (rx->length + ry->length);
#elif defined OVERLAP
  return overlap;
#endif
}

bool QualifyTextual(Record *rx, Record *ry, int xstart, int ystart, int o)
{
  int left_th = rx->length + ry->length;
  int xcounter = xstart, ycounter = ystart, overlap = o;
  int q_intersect = ry->requiredOverlap;

  while ((xcounter < rx->length) && (ycounter < ry->length))
  {
    if (FLAGS_inline_records == 1)
    {
      if (rx->tids[xcounter] == ry->tids[ycounter])
      {
        overlap++;
        if (overlap >= q_intersect)
          return true;

        if (left_th-- < 0)
          return false;

        xcounter++;
        ycounter++;
      }
      else if (rx->tids[xcounter] > ry->tids[ycounter])
      {
        if (left_th-- < 0)
          return false;
        ycounter++;
      }
      else
      {
        if (left_th-- < 0)
          return false;
        xcounter++;
      }
    }
    else
    {
      if (rx->keywords[xcounter] == ry->keywords[ycounter])
      {
        overlap++;
        if (overlap >= q_intersect)
          return true;

        if (left_th-- < 0)
          return false;

        xcounter++;
        ycounter++;
      }
      else if (rx->keywords[xcounter] > ry->keywords[ycounter])
      {
        if (left_th-- < 0)
          return false;
        ycounter++;
      }
      else
      {
        if (left_th-- < 0)
          return false;
        xcounter++;
      }
    }
  }

  return (overlap >= q_intersect);
}

bool QualifyTextual(Record *rx, Record *ry)
{
  return QualifyTextual(rx, ry, 0, 0, 0);
}

bool QualifyLengthFilter(Record *rx, Record *ry)
{
#ifdef JACCARD
  return (ry->length >= int(FLAGS_threshold * rx->length - EPS) + 1);
#elif defined COSINE
  return (ry->length >= int(sqrThreshold * rx->length - EPS) + 1);
#elif defined DICE
  return (ry->length >=
          int(FLAGS_threshold * rx->length / (2.0 - FLAGS_threshold) - EPS) + 1);
#elif defined OVERLAP
  return (ry->length >= FLAGS_threshold);
#endif
}

bool QualifyLengthFilter(GroupRecord *grx, GroupRecord *gry)
{
#ifdef JACCARD
  return (gry->length >= int(FLAGS_threshold * grx->length - EPS) + 1);
#elif defined COSINE
  return (gry->length >= int(sqrThreshold * grx->length - EPS) + 1);
#elif defined DICE
  return (gry->length >=
          int(FLAGS_threshold * grx->length / (2.0 - FLAGS_threshold) - EPS) + 1);
#elif defined OVERLAP
  return (gry->length >= FLAGS_threshold);
#endif
}

bool QualifyPositionalFilter(Record *rx, int xpos, Record *ry, int ypos)
{
#ifdef JACCARD
  ry->requiredOverlap =
      int((rx->length + ry->length) * FLAGS_threshold / (1.0 + FLAGS_threshold) - EPS) + 1;
#elif defined COSINE
  ry->requiredOverlap =
      int(rx->sqrtLength * ry->sqrtLength * FLAGS_threshold - EPS) + 1;
#elif defined DICE
  ry->requiredOverlap =
      int((rx->length + ry->length) * FLAGS_threshold / 2.0 - EPS) + 1;
#elif defined OVERLAP
  ry->requiredOverlap = (int)FLAGS_threshold;
#endif

  return ((rx->length - xpos >= ry->requiredOverlap) &&
          (ry->length - ypos >= ry->requiredOverlap));
}

bool QualifyPositionalFilter(GroupRecord *grx, int xpos, GroupRecord *gry,
                             int ypos)
{
#ifdef JACCARD
  gry->requiredOverlap =
      int((grx->length + gry->length) * FLAGS_threshold / (1 + FLAGS_threshold) - EPS) + 1;
#elif defined COSINE
  gry->requiredOverlap =
      int(grx->sqrtLength * gry->sqrtLength * FLAGS_threshold - EPS) + 1;
#elif defined DICE
  gry->requiredOverlap =
      int((grx->length + gry->length) * FLAGS_threshold / 2.0 - EPS) + 1;
#elif defined OVERLAP
  gry->requiredOverlap = (int)FLAGS_threshold;
#endif

  return ((grx->length - xpos >= gry->requiredOverlap) &&
          (gry->length - ypos >= gry->requiredOverlap));
}

int BinarySearch(int *arr, int value, int start, int end)
{
  int mid = -1;

  while (start < end)
  {
    mid = (start + end) / 2;
    if (arr[mid] < value)
      start = mid + 1;
    else
      end = mid;
  }

  return start;
}

int SuffixFilter(int *xArray, int *yArray, int xStart, int xEnd, int yStart,
                 int yEnd, int HD, int depth)
{
  if (xEnd <= xStart || yEnd <= yStart)
    return abs((xEnd - xStart) - (yEnd - yStart));
  int xLen = xEnd - xStart, yLen = yEnd - yStart;
  int left, right, mid, pos, token, offset;
  int HDLeft, HDRight, HDLeftBound, HDRightBound;

  mid = xStart + xLen / 2, token = xArray[mid];

  if (xLen >= yLen)
  {
    offset = (HD - (xLen - yLen)) / 2 + (xLen - yLen),
    left = yStart + xLen / 2 - offset;
    offset = (HD - (xLen - yLen)) / 2, right = yStart + xLen / 2 + offset;
  }
  else
  {
    offset = (HD - (yLen - xLen)) / 2, left = yStart + xLen / 2 - offset;
    offset = (HD - (yLen - xLen)) / 2 + (yLen - xLen),
    right = yStart + xLen / 2 + offset;
  }

  if (left >= yStart && yArray[left] > token ||
      right < yEnd && yArray[right] < token)
    return HD + 1;

  pos = BinarySearch(yArray, token, left >= yStart ? left : yStart,
                     right + 1 < yEnd ? right + 1 : yEnd);
  if (pos < yEnd &&
      yArray[pos] == token)
  { // x:[Left][mid][Right] y:[Left][pos][Right]
    HDLeft = HDLeftBound = abs((mid - xStart) - (pos - yStart));
    HDRight = HDRightBound = abs((xEnd - mid - 1) - (yEnd - pos - 1));
    if (HDLeftBound + HDRightBound > HD)
      return HDLeftBound + HDRightBound;
    if (depth < maxDepth)
    {
      HDLeft = SuffixFilter(xArray, yArray, xStart, mid, yStart, pos,
                            HD - HDRightBound, depth + 1);
      if (HDLeft + HDRightBound > HD)
        return HDLeft + HDRightBound;
      HDRight = SuffixFilter(xArray, yArray, mid + 1, xEnd, pos + 1, yEnd,
                             HD - HDLeft, depth + 1);
    }
    if (HDLeft + HDRight > HD)
      return HDLeft + HDRight;
    return HDLeft + HDRight;
  }
  else
  { // x:[Left][mid][Right] y:[Left][Right]
    HDLeft = HDLeftBound = abs((mid - xStart) - (pos - yStart));
    HDRight = HDRightBound = abs((xEnd - mid - 1) - (yEnd - pos));
    if (HDLeftBound + HDRightBound + 1 > HD)
      return HDLeftBound + HDRightBound + 1;
    if (depth < maxDepth)
    {
      HDLeft = SuffixFilter(xArray, yArray, xStart, mid, yStart, pos,
                            HD - HDRightBound - 1, depth + 1);
      if (HDLeft + HDRightBound + 1 > HD)
        return HDLeft + HDRightBound + 1;
      HDRight = SuffixFilter(xArray, yArray, mid + 1, xEnd, pos, yEnd,
                             HD - HDLeft - 1, depth + 1);
    }
    if (HDLeft + HDRight + 1 > HD)
      return HDLeft + HDRight + 1;
    return HDLeft + HDRight + 1;
  }

  return 0;
}

bool QualifySuffixFilter(Record *rx, int xpos, Record *ry, int ypos)
{
  ry->hammingDistanceThreshold =
      rx->length + ry->length - 2 * ry->requiredOverlap - (xpos + ypos);

  return (SuffixFilter(ry->keywords, rx->keywords, ypos + 1, ry->length,
                       xpos + 1, rx->length, ry->hammingDistanceThreshold,
                       1) <= ry->hammingDistanceThreshold);
}

bool QualifySuffixFilter(GroupRecord *grx, int xpos, GroupRecord *gry,
                         int ypos)
{
  gry->hammingDistanceThreshold =
      grx->length + gry->length - 2 * gry->requiredOverlap - (xpos + ypos);
  gry->xpos = xpos;
  gry->ypos = ypos;

  if ((grx->size == 1) && (gry->size == 1))
    return (SuffixFilter(gry->keywords, grx->keywords, ypos + 1, gry->length,
                         xpos + 1, grx->length, gry->hammingDistanceThreshold,
                         1) <= gry->hammingDistanceThreshold);
  else
    return true;
}

int CompareRecordsLexicographicallyInt(const Record *rx, const Record *ry)
{
  int i = 0;
  int xPrefixLength =
      min(rx->length, (int)(rx->length - rx->length * FLAGS_threshold + EPS) + 1);
  int yPrefixLength =
      min(ry->length, (int)(ry->length - ry->length * FLAGS_threshold + EPS) + 1);
  int bound = min(xPrefixLength, yPrefixLength);

  while ((i < bound) && (rx->keywords[i] == ry->keywords[i]))
    i++;

  if (i == bound)
    return (xPrefixLength - yPrefixLength);
  else
    return (rx->keywords[i] - ry->keywords[i]);
}

bool CompareRecordsByProbePrefixAndId(const Record *rx, const Record *ry)
{
  int diff, pdiff;

  if ((diff = rx->probePrefixLength - ry->probePrefixLength) != 0)
    return (diff < 0);
  else if ((pdiff = CompareRecordsLexicographicallyInt(rx, ry)) != 0)
    return pdiff < 0;
  else
    return rx->id < ry->id;
}

bool CompareRecordsByLengthProbePrefixAndId(const Record *rx,
                                            const Record *ry)
{
  int diff, pdiff;

  if ((diff = rx->length - ry->length) != 0)
    return (diff < 0);
  else if ((pdiff = CompareRecordsLexicographicallyInt(rx, ry)) != 0)
    return pdiff < 0;
  else
    return rx->id < ry->id;
}

bool CompareGroupRecordsByLengthAndId(const GroupRecord &grx,
                                      const GroupRecord &gry)
{
  int diff;

  if ((diff = grx.length - gry.length) != 0)
    return (diff < 0);
  else
    return grx.id < gry.id;
}

int CompareGroupRecordsLegicographicallyInt(const GroupRecord &grx,
                                            const GroupRecord &gry)
{
  int i = 0;
  int bound = min(grx.probePrefixLength, gry.probePrefixLength);

  while ((i < bound) && (grx.keywords[i] == gry.keywords[i]))
    i++;

  // if (i == bound)
  //	return (grx.probePrefixLength - gry.probePrefixLength);
  // else
  return (grx.keywords[i] - gry.keywords[i]);
}

void CreateGroupRecords()
{
  vector<Record *> sR;
  Record *r, *prevr;
  GroupRecord gr;
  int grid = -1, prevProbePrefixLength = -1, probePrefixLength;
  unsigned long long totalLength = 0;
  vector<Record *> tmpbag;

  for (int rid = 0; rid < numRecords; rid++)
    sR.push_back(&R[rid]);
  sort(sR.begin(), sR.end(), CompareRecordsByProbePrefixAndId);

  for (vector<Record *>::iterator iter = sR.begin(); iter != sR.end(); ++iter)
  {
    r = (*iter);
    probePrefixLength = r->probePrefixLength;

#ifdef PRINT_GROUP_RECORD_ID
    cout << "r" << r->id << ":";
#endif

    if ((prevProbePrefixLength == -1) ||
        (probePrefixLength != prevProbePrefixLength))
    {
      if (prevProbePrefixLength != -1)
        bagsR.push_back(tmpbag);
      tmpbag.clear();

      grid++;
      prevProbePrefixLength = probePrefixLength;
      prevr = r;

#ifdef PRINT_GROUP_RECORD_ID
      cout << " ------> grid: " << grid << endl;
#endif

      gr.id = grid;
      gr.size = 0;
      gr.length = probePrefixLength;
      gr.probePrefixLength = probePrefixLength;
      gr.indexPrefixLength = probePrefixLength;
      // gr.keywords = new int[probePrefixLength];
      // memcpy(gr.keywords, r->keywords, probePrefixLength*sizeof(int));
      gR.push_back(gr);
      gR[grid].keywords = new int[probePrefixLength];
      memcpy(gR[grid].keywords, r->keywords, probePrefixLength * sizeof(int));
      // bagsR[grid].push_back(r);
      tmpbag.push_back(r);
    }
    else
    {
      for (int i = 0; i < probePrefixLength; i++)
      {
        if (r->keywords[i] != prevr->keywords[i])
        {
          bagsR.push_back(tmpbag);
          tmpbag.clear();

          prevr = r;

          grid++;
          gr.id = grid;
          gr.size = 0;
          gr.length = probePrefixLength;
          gr.probePrefixLength = probePrefixLength;
          gr.indexPrefixLength = probePrefixLength;
          // gr.keywords = new int[probePrefixLength];
          // memcpy(gr.keywords, r->keywords, probePrefixLength*sizeof(int));
          gR.push_back(gr);
          gR[grid].keywords = new int[probePrefixLength];
          memcpy(gR[grid].keywords, r->keywords,
                 probePrefixLength * sizeof(int));
          break;
        }
      }
      // bagsR[grid].push_back(r);
      tmpbag.push_back(r);

#ifdef PRINT_GROUP_RECORD_ID
      cout << " ------> grid: " << grid << endl;
#endif
    }
  }
  bagsR.push_back(tmpbag);

  numGroupRecords = gR.size();

  for (int grid = 0; grid < numGroupRecords; grid++)
  {
    gR[grid].size = bagsR[grid].size();

#ifdef STATISTICS
    totalLength += gR[grid].length;
#endif
  }

#ifdef STATISTICS
  stats->avgGroupRecordLength = (float)totalLength / numGroupRecords;
#endif
}


void CountGroupKeywords()
{
  GroupRecord *gr;
  shash<int> dict;

  for (vector<GroupRecord>::iterator iter = gR.begin(); iter != gR.end();
       ++iter)
  {
    gr = &(*iter);

    for (int i = 0; i < gr->probePrefixLength; i++)
      dict.insert(gr->keywords[i]);
  }

  numGroupKeywords = dict.size();
}

void VerifyGroupSelf(GroupRecord *gr)
{
  Record *rx, *ry;
  vector<Record *>::iterator iterX, iterY, beginB, endB;
  int overlap = gr->probePrefixLength;

  beginB = bagsR[gr->id].begin();
  endB = bagsR[gr->id].end();
  for (iterX = beginB; iterX != endB; ++iterX)
  {
    rx = (*iterX);
    for (iterY = beginB; iterY != endB; ++iterY)
    {
      ry = (*iterY);
      if (rx->id > ry->id)
      {
        numCandidates++;

        if (QualifyTextual(rx, ry))
        {
          numResults++;

#ifdef PRINT_RESULTS
          if (rx->id > ry->id)
            cout << rx->id << " " << ry->id << endl;
                //  << " ^ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
          else
            cout << ry->id << " " << rx->id << endl;
                //  << " ^ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
  }
}

void VerifyGroupSelfEqualLengthsPVLDB12(GroupRecord *gr)
{
  Record *rx, *ry;
  vector<Record *>::iterator iterX, iterY, beginB, endB;
  int overlap = gr->probePrefixLength, requiredOverlapE, yindexPrefixLength;

#ifdef JACCARD
  requiredOverlapE =
      int((gr->length + gr->length) * FLAGS_threshold / (1 + FLAGS_threshold) - EPS) + 1;
#elif defined COSINE
  requiredOverlapE = int(gr->sqrtLength * gr->sqrtLength * FLAGS_threshold - EPS) + 1;
#elif defined OVERLAP
  requiredOverlapE = (int)FLAGS_threshold;
#endif

  beginB = bagsR[gr->id].begin();
  endB = bagsR[gr->id].end();
  for (iterX = beginB; iterX != endB; ++iterX)
  {
    rx = (*iterX);
    for (iterY = beginB; iterY != endB; ++iterY)
    {
      ry = (*iterY);
      if (rx->id <= ry->id)
        break;
      // if (rx->id > ry->id)
      //{
      yindexPrefixLength = ry->indexPrefixLength;

      if (overlap + ry->length - yindexPrefixLength >= requiredOverlapE)
      {
        numCandidates++;

#ifdef PRINT_CANDIDATES
        cout << rx->id << " " << ry->id << endl;
#endif

        if (QualifyTextual(rx, ry, overlap, yindexPrefixLength, overlap))
        {
          numResults++;

#ifdef PRINT_RESULTS
          cout << rx->id << " " << ry->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
      //}
    }
  }
}

void VerifyGroupSelfEqualLengths(GroupRecord *gr)
{
  Record *rx, *ry;
  vector<Record *>::iterator iterX, iterY, beginB, endB;
  int overlap = gr->probePrefixLength, requiredOverlapE, yindexPrefixLength,
      hammingDistanceThreshold;

#ifdef JACCARD
  requiredOverlapE =
      int((gr->length + gr->length) * FLAGS_threshold / (1 + FLAGS_threshold) - EPS) + 1;
#elif defined COSINE
  requiredOverlapE = int(gr->sqrtLength * gr->sqrtLength * FLAGS_threshold - EPS) + 1;
#elif defined OVERLAP
  requiredOverlapE = (int)FLAGS_threshold;
#endif

  hammingDistanceThreshold = gr->length + gr->length - 2 * requiredOverlapE;

  beginB = bagsR[gr->id].begin();
  endB = bagsR[gr->id].end();
  for (iterX = beginB; iterX != endB; ++iterX)
  {
    rx = (*iterX);
    for (iterY = beginB; iterY != endB; ++iterY)
    {
      ry = (*iterY);
      if (rx->id <= ry->id)
        break;
      // if (rx->id > ry->id)
      //{
      yindexPrefixLength = ry->indexPrefixLength;

      if (overlap + ry->length - yindexPrefixLength >= requiredOverlapE)
      {
        if (SuffixFilter(ry->keywords, rx->keywords, 1, ry->length, 1,
                         rx->length, hammingDistanceThreshold,
                         1) > hammingDistanceThreshold)
          continue;

        numCandidates++;

#ifdef PRINT_CANDIDATES
        cout << rx->id << " " << ry->id << endl;
#endif

        if (QualifyTextual(rx, ry, overlap, yindexPrefixLength, overlap))
        {
          numResults++;

#ifdef PRINT_RESULTS
          cout << rx->id << " " << ry->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
      //}
    }
  }
}

void VerifyGroupNonSelf(GroupRecord *grx, GroupRecord *gry, int overlap)
{
  Record *rx, *ry;
  vector<Record *>::iterator iterX, iterY, beginX, beginY, endX, endY;

  beginX = bagsR[grx->id].begin();
  beginY = bagsR[gry->id].begin();
  if (grx->size == 1)
  {
    rx = (*beginX);
    if (gry->size == 1)
    {
      numCandidates++;

      ry = (*beginY);
      if (QualifyTextual(rx, ry))
      {
        numResults++;

#ifdef PRINT_RESULTS
        if (rx->id > ry->id)
          cout << rx->id << " " << ry->id << endl;
              //  << " | : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
        else
          cout << ry->id << " " << rx->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
      }
    }
    else
    {
      rx = (*beginX);
      endY = bagsR[gry->id].end();
      for (iterY = beginY; iterY != endY; ++iterY)
      {
        numCandidates++;

        ry = (*iterY);

        if  //(QualifyTextual(rx, ry, probePrefixLength, iterA->second,
            // iterA->second))
            (QualifyTextual(rx, ry))
        {
          numResults++;

#ifdef PRINT_RESULTS
          if (rx->id > ry->id)
            cout << rx->id << " " << ry->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
          else
            cout << ry->id << " " << rx->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
  }
  else
  {
    endX = bagsR[grx->id].end();
    endY = bagsR[gry->id].end();
    for (iterX = beginX; iterX != endX; ++iterX)
    {
      rx = (*iterX);
      for (iterY = beginY; iterY != endY; ++iterY)
      {
        numCandidates++;

        ry = (*iterY);
        if  //(QualifyTextual(rx, ry, probePrefixLength, iterA->second,
            // iterA->second))
            (QualifyTextual(rx, ry))
        {
          numResults++;

#ifdef PRINT_RESULTS
          if (rx->id > ry->id)
            cout << rx->id << " " << ry->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
          else
            cout << ry->id << " " << rx->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
  }
}

void VerifyNonSelfPlus(GroupRecord *grx, GroupRecord *gry, int startx,
                       int starty, int overlap)
{
  Record *rx, *ry;
  vector<Record *>::iterator iterX, iterY, beginX, beginY, endX, endY;
  int hammingDistanceThresholdE =
      grx->length + gry->length - 2 * gry->requiredOverlap;

  if (grx->size == 1)
  {
    rx = grx->rec;
    if (gry->size == 1)
    {
      ry = gry->rec;
      ry->requiredOverlap = gry->requiredOverlap;

      numCandidates++;

#ifdef PRINT_CANDIDATES
      if (rx->id > ry->id)
        cout << rx->id << " " << ry->id << endl;
      else
        cout << ry->id << " " << rx->id << endl;
#endif

      if (QualifyTextual(rx, ry, startx, starty, overlap))
      {
        numResults++;

#ifdef PRINT_RESULTS
        if (rx->id > ry->id)
          cout << rx->id << " " << ry->id << endl;
              //  << " | : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
        else
          cout << ry->id << " " << rx->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
      }
    }
    else
    {
      // tmp2++;
      beginY = bagsR[gry->id].begin();
      endY = bagsR[gry->id].end();
      for (iterY = beginY; iterY != endY; ++iterY)
      {
        ry = (*iterY);
        ry->requiredOverlap = gry->requiredOverlap;

        if (SuffixFilter(ry->keywords, rx->keywords, gry->ypos + 1, ry->length,
                         gry->xpos + 1, rx->length,
                         gry->hammingDistanceThreshold,
                         1) > gry->hammingDistanceThreshold)
          continue;

        numCandidates++;

#ifdef PRINT_CANDIDATES
        if (rx->id > ry->id)
          cout << rx->id << " " << ry->id << endl;
        else
          cout << ry->id << " " << rx->id << endl;
#endif

        if (QualifyTextual(rx, ry, startx, starty, overlap))
        {
          numResults++;

#ifdef PRINT_RESULTS
          if (rx->id > ry->id)
            cout << rx->id << " " << ry->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
          else
            cout << ry->id << " " << rx->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
  }
  else
  {
    // tmp3++;
    beginX = bagsR[grx->id].begin();
    endX = bagsR[grx->id].end();
    for (iterX = beginX; iterX != endX; ++iterX)
    {
      rx = (*iterX);

      if (gry->size == 1)
      {
        ry = gry->rec;
        ry->requiredOverlap = gry->requiredOverlap;

        if (SuffixFilter(ry->keywords, rx->keywords, gry->ypos + 1, ry->length,
                         gry->xpos + 1, rx->length,
                         gry->hammingDistanceThreshold,
                         1) > gry->hammingDistanceThreshold)
          continue;

        numCandidates++;

#ifdef PRINT_CANDIDATES
        if (rx->id > ry->id)
          cout << rx->id << " " << ry->id << endl;
        else
          cout << ry->id << " " << rx->id << endl;
#endif

        if (QualifyTextual(rx, ry, startx, starty, overlap))
        {
          numResults++;

#ifdef PRINT_RESULTS
          if (rx->id > ry->id)
            cout << rx->id << " " << ry->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
          else
            cout << ry->id << " " << rx->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
      else
      {
        beginY = bagsR[gry->id].begin();
        endY = bagsR[gry->id].end();
        for (iterY = beginY; iterY != endY; ++iterY)
        {
          ry = (*iterY);
          ry->requiredOverlap = gry->requiredOverlap;

          if (SuffixFilter(ry->keywords, rx->keywords, gry->ypos + 1,
                           ry->length, gry->xpos + 1, rx->length,
                           gry->hammingDistanceThreshold,
                           1) > gry->hammingDistanceThreshold)
            continue;

          numCandidates++;

#ifdef PRINT_CANDIDATES
          if (rx->id > ry->id)
            cout << rx->id << " " << ry->id << endl;
          else
            cout << ry->id << " " << rx->id << endl;
#endif

          if (QualifyTextual(rx, ry, startx, starty, overlap))
          {
            numResults++;

#ifdef PRINT_RESULTS
            if (rx->id > ry->id)
              cout << rx->id << " " << ry->id << endl;
                  //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
            else
              cout << ry->id << " " << rx->id << endl;
                  //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
          }
        }
      }
    }
  }
}

void VerifyNonSelf(GroupRecord *grx, GroupRecord *gry, int startx, int starty,
                   int overlap)
{
  Record *rx, *ry;
  vector<Record *>::iterator iterX, iterY, beginX, beginY, endX, endY;

  if (grx->size == 1)
  {
    rx = grx->rec;
    if (gry->size == 1)
    {
      ry = gry->rec;
      ry->requiredOverlap = gry->requiredOverlap;
      // tmp1++;

      numCandidates++;

#ifdef PRINT_CANDIDATES
      if (rx->id > ry->id)
        cout << rx->id << " " << ry->id << endl;
      else
        cout << ry->id << " " << rx->id << endl;
#endif

      if (QualifyTextual(rx, ry, startx, starty, overlap))
      {
        numResults++;

#ifdef PRINT_RESULTS
        if (rx->id > ry->id)
          cout << rx->id << " " << ry->id << endl;
              //  << " | : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
        else
          cout << ry->id << " " << rx->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
      }
    }
    else
    {
      // tmp2++;
      beginY = bagsR[gry->id].begin();
      endY = bagsR[gry->id].end();
      for (iterY = beginY; iterY != endY; ++iterY)
      {
        ry = (*iterY);
        ry->requiredOverlap = gry->requiredOverlap;

        numCandidates++;

#ifdef PRINT_CANDIDATES
        if (rx->id > ry->id)
          cout << rx->id << " " << ry->id << endl;
        else
          cout << ry->id << " " << rx->id << endl;
#endif

        if (QualifyTextual(rx, ry, startx, starty, overlap))
        {
          numResults++;

#ifdef PRINT_RESULTS
          if (rx->id > ry->id)
            cout << rx->id << " " << ry->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
          else
            cout << ry->id << " " << rx->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
  }
  else
  {
    // tmp3++;
    beginX = bagsR[grx->id].begin();
    endX = bagsR[grx->id].end();
    for (iterX = beginX; iterX != endX; ++iterX)
    {
      rx = (*iterX);

      if (gry->size == 1)
      {
        ry = gry->rec;
        ry->requiredOverlap = gry->requiredOverlap;

        numCandidates++;

#ifdef PRINT_CANDIDATES
        if (rx->id > ry->id)
          cout << rx->id << " " << ry->id << endl;
        else
          cout << ry->id << " " << rx->id << endl;
#endif

        if (QualifyTextual(rx, ry, startx, starty, overlap))
        {
          numResults++;

#ifdef PRINT_RESULTS
          if (rx->id > ry->id)
            cout << rx->id << " " << ry->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
          else
            cout << ry->id << " " << rx->id << endl;
                //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
      else
      {
        beginY = bagsR[gry->id].begin();
        endY = bagsR[gry->id].end();
        for (iterY = beginY; iterY != endY; ++iterY)
        {
          ry = (*iterY);
          ry->requiredOverlap = gry->requiredOverlap;

          numCandidates++;

#ifdef PRINT_CANDIDATES
          if (rx->id > ry->id)
            cout << rx->id << " " << ry->id << endl;
          else
            cout << ry->id << " " << rx->id << endl;
#endif

          if (QualifyTextual(rx, ry, startx, starty, overlap))
          {
            numResults++;

#ifdef PRINT_RESULTS
            if (rx->id > ry->id)
              cout << rx->id << " " << ry->id << endl;
                  //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
            else
              cout << ry->id << " " << rx->id << endl;
                  //  << " $ : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
          }
        }
      }
    }
  }
}

void VerifyGroupNonSelfEqualLengths(GroupRecord *grx, GroupRecord *gry,
                                    int overlap)
{
  Record *rx, *ry;
  vector<Record *>::iterator iterX, iterY, beginX, beginY, endX, endY;
  int requiredOverlapE, yindexPrefixLength;

  requiredOverlapE =
      int((grx->length + gry->length) * FLAGS_threshold / (1 + FLAGS_threshold) - EPS) + 1;

  beginX = bagsR[grx->id].begin();
  beginY = bagsR[gry->id].begin();
  if (grx->size == 1)
  {
    rx = (*beginX);
    if (gry->size == 1)
    {
      ry = (*beginY);
      yindexPrefixLength = ry->indexPrefixLength;

      if (rx->keywords[rx->probePrefixLength - 1] <
          ry->keywords[yindexPrefixLength - 1])
      {
        if (overlap + rx->length - rx->probePrefixLength >= requiredOverlapE)
        {
          numCandidates++;

          if (QualifyTextual(rx, ry, rx->probePrefixLength, overlap, overlap))
          {
            numResults++;

#ifdef PRINT_RESULTS
            if (rx->id > ry->id)
              cout << rx->id << " " << ry->id << endl;
                  //  << " | : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
            else
              cout << ry->id << " " << rx->id << endl;
                  //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
          }
        }
      }
      else
      {
        if (overlap + ry->length - yindexPrefixLength >= requiredOverlapE)
        {
          numCandidates++;

          if (QualifyTextual(rx, ry, overlap, yindexPrefixLength, overlap))
          {
            numResults++;

#ifdef PRINT_RESULTS
            if (rx->id > ry->id)
              cout << rx->id << " " << ry->id << endl;
                  //  << " | : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
            else
              cout << ry->id << " " << rx->id << endl;
                  //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
          }
        }
      }
    }
    else
    {
      rx = (*beginX);
      endY = bagsR[gry->id].end();
      for (iterY = beginY; iterY != endY; ++iterY)
      {
        ry = (*iterY);
        yindexPrefixLength = ry->indexPrefixLength;

        if (rx->keywords[rx->probePrefixLength - 1] <
            ry->keywords[yindexPrefixLength - 1])
        {
          if (overlap + rx->length - rx->probePrefixLength >=
              requiredOverlapE)
          {
            numCandidates++;

            if (QualifyTextual(rx, ry, rx->probePrefixLength, overlap,
                               overlap))
            {
              numResults++;

#ifdef PRINT_RESULTS
              if (rx->id > ry->id)
                cout << rx->id << " " << ry->id << endl;
                    //  << " | : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
              else
                cout << ry->id << " " << rx->id << endl;
                    //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
            }
          }
        }
        else
        {
          if (overlap + ry->length - yindexPrefixLength >= requiredOverlapE)
          {
            numCandidates++;

            if (QualifyTextual(rx, ry, overlap, yindexPrefixLength, overlap))
            {
              numResults++;

#ifdef PRINT_RESULTS
              if (rx->id > ry->id)
                cout << rx->id << " " << ry->id << endl;
                    //  << " | : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
              else
                cout << ry->id << " " << rx->id << endl;
                    //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
            }
          }
        }
      }
    }
  }
  else
  {
    endX = bagsR[grx->id].end();
    endY = bagsR[gry->id].end();
    for (iterX = beginX; iterX != endX; ++iterX)
    {
      rx = (*iterX);
      for (iterY = beginY; iterY != endY; ++iterY)
      {
        ry = (*iterY);
        yindexPrefixLength = ry->indexPrefixLength;

        if (rx->keywords[rx->probePrefixLength - 1] <
            ry->keywords[yindexPrefixLength - 1])
        {
          if (overlap + rx->length - rx->probePrefixLength >=
              requiredOverlapE)
          {
            numCandidates++;

            if (QualifyTextual(rx, ry, rx->probePrefixLength, overlap,
                               overlap))
            {
              numResults++;

#ifdef PRINT_RESULTS
              if (rx->id > ry->id)
                cout << rx->id << " " << ry->id << endl;
                    //  << " | : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
              else
                cout << ry->id << " " << rx->id << endl;
                    //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
            }
          }
        }
        else
        {
          if (overlap + ry->length - yindexPrefixLength >= requiredOverlapE)
          {
            numCandidates++;

            if (QualifyTextual(rx, ry, overlap, yindexPrefixLength, overlap))
            {
              numResults++;

#ifdef PRINT_RESULTS
              if (rx->id > ry->id)
                cout << rx->id << " " << ry->id << endl;
                    //  << " | : sim_t = " << GetTextualSimilarity(rx, ry) << endl;
              else
                cout << ry->id << " " << rx->id << endl;
                    //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
            }
          }
        }
      }
    }
  }
}

// Same as VerifyAllPairs but generates results locally
void VerifyAllPairsThread(Record *rx, Record *ry, int overlap,
                          uint64_t &n_candidates, uint64_t &n_results)
{
  n_candidates = 0;
  n_results = 0;

#ifdef JACCARD
  ry->requiredOverlap =
      int((rx->length + ry->length) * FLAGS_threshold / (1.0 + FLAGS_threshold) - EPS) + 1;
#elif defined COSINE
  ry->requiredOverlap =
      int(rx->sqrtLength * ry->sqrtLength * FLAGS_threshold - EPS) + 1;
#elif defined DICE
  ry->requiredOverlap =
      int((rx->length + ry->length) * FLAGS_threshold / 2.0 - EPS) + 1;
#elif defined OVERLAP
  ry->requiredOverlap = (int)FLAGS_threshold;
#endif

  if (FLAGS_inline_records == 1)
  {
    if (rx->tids[rx->probePrefixLength - 1] < ry->tids[ry->indexPrefixLength - 1])
    {
      if (overlap + rx->length - rx->probePrefixLength >= ry->requiredOverlap)
      {
        ++n_candidates;

#ifdef PRINT_CANDIDATES
        cout << rx->id << " " << ry->id << endl;
#endif

        if (QualifyTextual(rx, ry, rx->probePrefixLength, overlap, overlap))
        {
          ++n_results;

#ifdef PRINT_RESULTS
          cout << rx->id << " " << ry->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
    else
    {
      if (overlap + ry->length - ry->indexPrefixLength >= ry->requiredOverlap)
      {
        ++n_candidates;

#ifdef PRINT_CANDIDATES
        cout << rx->id << " " << ry->id << endl;
#endif

        if (QualifyTextual(rx, ry, overlap, ry->indexPrefixLength, overlap))
        {
          ++n_results;

#ifdef PRINT_RESULTS
          cout << rx->id << " " << ry->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
  }
  else
  {
    if (rx->keywords[rx->probePrefixLength - 1] <
        ry->keywords[ry->indexPrefixLength - 1])
    {
      if (overlap + rx->length - rx->probePrefixLength >= ry->requiredOverlap)
      {
        ++n_candidates;

#ifdef PRINT_CANDIDATES
        cout << rx->id << " " << ry->id << endl;
#endif

        if (QualifyTextual(rx, ry, rx->probePrefixLength, overlap, overlap))
        {
          ++n_results;

#ifdef PRINT_RESULTS
          cout << rx->id << " " << ry->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
    else
    {
      if (overlap + ry->length - ry->indexPrefixLength >= ry->requiredOverlap)
      {
        ++n_candidates;

#ifdef PRINT_CANDIDATES
        cout << rx->id << " " << ry->id << endl;
#endif

        if (QualifyTextual(rx, ry, overlap, ry->indexPrefixLength, overlap))
        {
          ++n_results;

#ifdef PRINT_RESULTS
          cout << rx->id << " " << ry->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
  }
  // numResults += n_results; // When we do this we count everything double
  // numCandidates += n_candidates;
}

void VerifyAllPairs(Record *rx, Record *ry, int overlap)
{
#ifdef JACCARD
  ry->requiredOverlap =
      int((rx->length + ry->length) * FLAGS_threshold / (1.0 + FLAGS_threshold) - EPS) + 1;
#elif defined COSINE
  ry->requiredOverlap =
      int(rx->sqrtLength * ry->sqrtLength * FLAGS_threshold - EPS) + 1;
#elif defined DICE
  ry->requiredOverlap =
      int((rx->length + ry->length) * FLAGS_threshold / 2.0 - EPS) + 1;
#elif defined OVERLAP
  ry->requiredOverlap = (int)FLAGS_threshold;
#endif

  if (FLAGS_inline_records == 1)
  {
    if (rx->tids[rx->probePrefixLength - 1] <
        ry->tids[ry->indexPrefixLength - 1])
    {
      if (overlap + rx->length - rx->probePrefixLength >= ry->requiredOverlap)
      {
        numCandidates++;

#ifdef PRINT_CANDIDATES
        cout << rx->id << " " << ry->id << endl;
#endif

        if (QualifyTextual(rx, ry, rx->probePrefixLength, overlap, overlap))
        {
          numResults++;

#ifdef PRINT_RESULTS
          cout << rx->id << " " << ry->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
    else
    {
      if (overlap + ry->length - ry->indexPrefixLength >= ry->requiredOverlap)
      {
        numCandidates++;

#ifdef PRINT_CANDIDATES
        cout << rx->id << " " << ry->id << endl;
#endif

        if (QualifyTextual(rx, ry, overlap, ry->indexPrefixLength, overlap))
        {
          numResults++;

#ifdef PRINT_RESULTS
          cout << rx->id << " " << ry->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
  }
  else
  {
    if (rx->keywords[rx->probePrefixLength - 1] <
        ry->keywords[ry->indexPrefixLength - 1])
    {
      if (overlap + rx->length - rx->probePrefixLength >= ry->requiredOverlap)
      {
        numCandidates++;

#ifdef PRINT_CANDIDATES
        cout << rx->id << " " << ry->id << endl;
#endif

        if (QualifyTextual(rx, ry, rx->probePrefixLength, overlap, overlap))
        {
          numResults++;

#ifdef PRINT_RESULTS
          cout << rx->id << " " << ry->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
    else
    {
      if (overlap + ry->length - ry->indexPrefixLength >= ry->requiredOverlap)
      {
        numCandidates++;

#ifdef PRINT_CANDIDATES
        cout << rx->id << " " << ry->id << endl;
#endif

        if (QualifyTextual(rx, ry, overlap, ry->indexPrefixLength, overlap))
        {
          numResults++;

#ifdef PRINT_RESULTS
          cout << rx->id << " " << ry->id << endl;
              //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
        }
      }
    }
  }
}

void Verify(Record *rx, Record *ry, int overlap)
{
  VerifyAllPairs(rx, ry, overlap);
}

void VerifyMPJoin(Record *rx, Record *ry, int overlap)
{
  // if (rx->keywords[rx->probePrefixLength] <
  // ry->keywords[ry->validIndexPrefixLength]) ??????
  if (rx->keywords[rx->probePrefixLength - 1] <
      ry->keywords[ry->validIndexPrefixLength - 1])
  {
    if (overlap + rx->length - rx->probePrefixLength >= ry->requiredOverlap)
    {
      numCandidates++;

#ifdef PRINT_CANDIDATES
      cout << rx->id << " " << ry->id << endl;
#endif

      if (QualifyTextual(rx, ry, rx->probePrefixLength, ry->ypos_mp + 1,
                         overlap))
      {
        numResults++;

#ifdef PRINT_RESULTS
        cout << rx->id << " " << ry->id << endl;
            //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
      }
    }
  }
  else
  {
    if (overlap + ry->length - ry->validIndexPrefixLength >=
        ry->requiredOverlap)
    {
      numCandidates++;

#ifdef PRINT_CANDIDATES
      cout << rx->id << " " << ry->id << endl;
#endif

      if (QualifyTextual(rx, ry, ry->xpos_mp + 1, ry->validIndexPrefixLength,
                         overlap))
      {
        numResults++;

#ifdef PRINT_RESULTS
        cout << rx->id << " " << ry->id << endl;
            //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
      }
    }
  }
}

void Verify(GroupRecord *gr)
{
  if (gr->size > 1)
  {
    Record *rx, *ry;
    vector<Record *>::iterator iterX, iterY, beginB, endB;
    int overlap = gr->probePrefixLength;

#ifdef JACCARD
    int requiredOverlap =
        int((gr->length + gr->length) * FLAGS_threshold / (1 + FLAGS_threshold) - EPS) + 1;
#elif defined COSINE
    int requiredOverlap = int(gr->length * FLAGS_threshold - EPS) + 1;
#elif defined DICE
    int requiredOverlap = int(gr->length * FLAGS_threshold - EPS) + 1;
#elif defined OVERLAP
    int requiredOverlap = (int)FLAGS_threshold;
#endif

    beginB = bagsR[gr->id].begin();
    endB = bagsR[gr->id].end();
    if (*endB != NULL) { // FF: This is weird. It should not be null, but for BMS it becomes null sometimes and runs into an infinite loop. DBLP works.
      for (iterX = beginB; iterX != endB; ++iterX)
      {
        rx = (*iterX);
        for (iterY = beginB; iterY != endB; ++iterY)
        {
          ry = (*iterY);
          if (rx->id <= ry->id)
            break;

          if (overlap + ry->length - ry->probePrefixLength >= requiredOverlap)
          {
            numCandidates++;

  #ifdef PRINT_CANDIDATES
            cout << rx->id << " " << ry->id << endl;
  #endif

            ry->requiredOverlap = requiredOverlap;
            if (QualifyTextual(rx, ry, overlap, ry->probePrefixLength, overlap))
            {
              numResults++;

  #ifdef PRINT_RESULTS
              cout << rx->id << " " << ry->id << endl;
                  // << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
  #endif
            }
          }
        }
      }
    } // end if not NULL
  } // end if gr.size > 1
}

void VerifyPlus(GroupRecord *gr)
{
  if (gr->size > 1)
  {
    Record *rx, *ry;
    vector<Record *>::iterator iterX, iterY, beginB, endB;
    int overlap = gr->probePrefixLength;

#ifdef JACCARD
    int requiredOverlap =
        int((gr->length + gr->length) * FLAGS_threshold / (1 + FLAGS_threshold) - EPS) + 1;
#elif defined COSINE
    int requiredOverlap = int(gr->length * FLAGS_threshold - EPS) + 1;
#elif defined DICE
    int requiredOverlap = int(gr->length * FLAGS_threshold - EPS) + 1;
#elif defined OVERLAP
    int requiredOverlap = (int)FLAGS_threshold;
#endif

    int hammingDistanceThreshold =
        gr->length + gr->length - 2 * requiredOverlap;

    beginB = bagsR[gr->id].begin();
    endB = bagsR[gr->id].end();
    for (iterX = beginB; iterX != endB; ++iterX)
    {
      rx = (*iterX);
      for (iterY = beginB; iterY != endB; ++iterY)
      {
        ry = (*iterY);
        if (rx->id <= ry->id)
          break;

        if (overlap + ry->length - ry->probePrefixLength >= requiredOverlap)
        {
          // Apply suffix filter.
          if (SuffixFilter(ry->keywords, rx->keywords, 1, ry->length, 1,
                           rx->length, hammingDistanceThreshold,
                           1) > hammingDistanceThreshold)
            continue;

          numCandidates++;

#ifdef PRINT_CANDIDATES
          cout << rx->id << " " << ry->id << endl;
#endif

          ry->requiredOverlap = requiredOverlap;
          if (QualifyTextual(rx, ry, overlap, ry->probePrefixLength, overlap))
          {
            numResults++;

#ifdef PRINT_RESULTS
            cout << rx->id << " " << ry->id << endl;
                //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
          }
        }
      }
    }
  }
}

void VerifyMPJoin(GroupRecord *gr)
{
  if (gr->size > 1)
  {
    Record *rx, *ry;
    vector<Record *>::iterator iterX, iterY, beginB, endB;
    int overlap = gr->probePrefixLength;

#ifdef JACCARD
    int requiredOverlap =
        int((gr->length + gr->length) * FLAGS_threshold / (1 + FLAGS_threshold) - EPS) + 1;
#elif defined COSINE
    int requiredOverlap = int(gr->length * FLAGS_threshold - EPS) + 1;
#elif defined DICE
    int requiredOverlap = int(gr->length * FLAGS_threshold - EPS) + 1;
#elif defined OVERLAP
    int requiredOverlap = (int)FLAGS_threshold;
#endif

    beginB = bagsR[gr->id].begin();
    endB = bagsR[gr->id].end();
    for (iterX = beginB; iterX != endB; ++iterX)
    {
      rx = (*iterX);
      for (iterY = beginB; iterY != endB; ++iterY)
      {
        ry = (*iterY);
        if (rx->id <= ry->id)
          break;

        if (overlap + ry->length - gr->probePrefixLength >= requiredOverlap)
        {
          numCandidates++;

#ifdef PRINT_CANDIDATES
          cout << rx->id << " " << ry->id << endl;
#endif

          ry->requiredOverlap = requiredOverlap;
          if (QualifyTextual(rx, ry, overlap, gr->probePrefixLength, overlap))
          {
            numResults++;

#ifdef PRINT_RESULTS
            cout << rx->id << " " << ry->id << endl;
                //  << ": sim_t = " << GetTextualSimilarity(rx, ry) << endl;
#endif
          }
        }
      }
    }
  }
}

void Verify(GroupRecord *grx, GroupRecord *gry, int overlap)
{
  if (grx->keywords[grx->probePrefixLength - 1] <
      gry->keywords[gry->indexPrefixLength - 1])
  {
    if (overlap + grx->length - grx->probePrefixLength >= gry->requiredOverlap)
      VerifyNonSelf(grx, gry, grx->probePrefixLength, overlap, overlap);
  }
  else
  {
    if (overlap + gry->length - gry->indexPrefixLength >= gry->requiredOverlap)
      VerifyNonSelf(grx, gry, overlap, gry->indexPrefixLength, overlap);
  }
}

void VerifyPlus(GroupRecord *grx, GroupRecord *gry, int overlap)
{
  if (grx->keywords[grx->probePrefixLength - 1] <
      gry->keywords[gry->indexPrefixLength - 1])
  {
    if (overlap + grx->length - grx->probePrefixLength >= gry->requiredOverlap)
      VerifyNonSelfPlus(grx, gry, grx->probePrefixLength, overlap, overlap);
  }
  else
  {
    if (overlap + gry->length - gry->indexPrefixLength >= gry->requiredOverlap)
      VerifyNonSelfPlus(grx, gry, overlap, gry->indexPrefixLength, overlap);
  }
}

void VerifyMPJoin(GroupRecord *grx, GroupRecord *gry, int overlap)
{
  if (grx->keywords[grx->probePrefixLength - 1] <
      gry->keywords[gry->validIndexPrefixLength - 1])
  {
    if (overlap + grx->length - grx->probePrefixLength >= gry->requiredOverlap)
      VerifyNonSelf(grx, gry, grx->probePrefixLength, gry->ypos_mp + 1,
                    overlap);
  }
  else
  {
    if (overlap + gry->length - gry->validIndexPrefixLength >=
        gry->requiredOverlap)
      VerifyNonSelf(grx, gry, gry->xpos_mp + 1, gry->validIndexPrefixLength,
                    overlap);
  }
}

void VerifyMPJoinPlus(GroupRecord *grx, GroupRecord *gry, int overlap)
{
  if (grx->keywords[grx->probePrefixLength - 1] <
      gry->keywords[gry->validIndexPrefixLength - 1])
  {
    if (overlap + grx->length - grx->probePrefixLength >= gry->requiredOverlap)
      VerifyNonSelfPlus(grx, gry, grx->probePrefixLength, gry->ypos_mp + 1,
                        overlap);
  }
  else
  {
    if (overlap + gry->length - gry->validIndexPrefixLength >=
        gry->requiredOverlap)
      VerifyNonSelfPlus(grx, gry, gry->xpos_mp + 1, gry->validIndexPrefixLength,
                        overlap);
  }
}
