// Test of sequence matching.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <locale.h>
#include <assert.h>
#include <map>
#include "util.hpp"

#include "CAccelDriver.hpp"
#include "CSeqMatcherDriver.hpp"

#define NUM_CORES_IN_SYSTEM 2

#define MAX_SEQ_LENGTH 32

const bool SHOULD_LOG = true;

typedef int8_t TPathMatrix[MAX_SEQ_LENGTH+1][MAX_SEQ_LENGTH+1];

const char* DRIVER_NAME = "/dev/seq_matcher";

///////////////////////////////////////////////////////////////////////////////
bool InitDevice(CSeqMatcherDriver & seqMatcher, uint32_t numDBEntries, uint32_t numSeqsSpecimen,
    uint64_t * &seqsDB, uint64_t * &seqsSpecimen,
    uint8_t * &lengthsDB, uint8_t * &lengthsSpecimen, int8_t * &scores, bool log=true)
{
  printf("\n\nThis program requires that the bitstream is loaded in the FPGA.\n");
  printf("This program has to be run with sudo.\n");
  printf("Press ENTER to confirm that the bitstream is loaded (proceeding without it can crash the board).\n\n");
  getchar();

  if ( seqMatcher.Open(DRIVER_NAME) != CAccelDriver::OK ) {
    printf("Error opening the device driver %s", DRIVER_NAME);
    // printf("Error mapping device at physical address 0x%08X\n", CONV_ADDR);
    return false;
  }

  if (log)
    printf("Device driver %s succesfully open\n\n", DRIVER_NAME);

  // Allocate DMA memory for use by the device. We receive addresses in the *virtual* address space of the application.
  if (log)
    printf("Allocating DMA memory...\n");

  seqsDB = (uint64_t *)seqMatcher.AllocDMACompatible(numDBEntries*sizeof(uint64_t));
  lengthsDB = (uint8_t *)seqMatcher.AllocDMACompatible(numDBEntries*sizeof(uint8_t));
  seqsSpecimen = (uint64_t *)seqMatcher.AllocDMACompatible(numSeqsSpecimen*sizeof(uint64_t));
  lengthsSpecimen = (uint8_t *)seqMatcher.AllocDMACompatible(numSeqsSpecimen*sizeof(uint8_t));
  scores = (int8_t *)seqMatcher.AllocDMACompatible(numDBEntries*numSeqsSpecimen*sizeof(int8_t));

  if ( (seqsDB == NULL) || (lengthsDB == NULL) || (seqsSpecimen == NULL) || (lengthsSpecimen == NULL) || (scores == NULL) ) {
    printf("Error allocating DMA memory.\n");
    return false;
  }
 
  if (log) {
    printf("DMA memory allocated.\n");
    printf("seqsDB: Virtual address: 0x%08X (%u)\n", (uint32_t)seqsDB, (uint32_t)seqsDB);
    printf("lengthsDB: Virtual address: 0x%08X (%u)\n", (uint32_t)lengthsDB, (uint32_t)lengthsDB);
    printf("seqsSpecimen: Virtual address: 0x%08X (%u)\n", (uint32_t)seqsSpecimen, (uint32_t)seqsSpecimen);
    printf("lengthsSpecimen: Virtual address: 0x%08X (%u)\n", (uint32_t)lengthsSpecimen, (uint32_t)lengthsSpecimen);
    printf("scores: Virtual address: 0x%08X (%u)\n", (uint32_t)scores, (uint32_t)scores);
  }

  return true;
}



///////////////////////////////////////////////////////////////////////////////
void PrintSequence(char * seq, uint8_t length)
// Prints non-NULL terminated sequences.
{
  for (uint8_t ii = 0; ii < length; ++ ii)
    printf("%c", seq[ii]);
  printf("\n");
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
const uint8_t NB_A = 0;
const uint8_t NB_T = 1;
const uint8_t NB_G = 2;
const uint8_t NB_C = 3;

// Splits an uint8 into the 4 nucleobases it contains (each nucleobase is two bits)
inline uint8_t joinNucleobasesToUint8(uint8_t in[4]) {
	return ((in[0] & 0b11) << 6) | ((in[1] & 0b11) << 4) | ((in[2] & 0b11) << 2) | (in[3] & 0b11);
}

uint8_t compressNucleoBase(char c) {
	if (c == 'A') {
		return NB_A;
	} else if (c == 'T') {
		return NB_T;
	} else if (c == 'G') {
		return NB_G;
	} else if (c == 'C') {
		return NB_C;
	}

	// Shouldn't find any other letter
	assert(false);
}

uint32_t ReadLines(uint64_t* dest, uint8_t* lengths, const char * fileName, uint32_t numLines)
{
  FILE * input;
  uint32_t readLines = 0;

  if ( (input = fopen(fileName, "rt")) == NULL ) {
    printf("Error opening file [%s]\n", fileName);
    return 0;
  }

  for (readLines = 0; readLines < numLines; ++ readLines) {
    char line[MAX_SEQ_LENGTH+2];  // + newline + NULL
    if (fgets(line, MAX_SEQ_LENGTH+2, input) == NULL)
      break;
    uint32_t lineSize = strlen(line);
    uint32_t iChar;

    uint8_t length = 0;
    uint64_t seq = 0;

    for (iChar = 0; (iChar < lineSize) && (iChar < MAX_SEQ_LENGTH); ++ iChar) {
      if (line[iChar] != '\n') {  // New lines are included in the string by fgets

    	uint8_t nbase = compressNucleoBase(line[iChar]) & 0b11;
    	seq |= uint64_t(nbase) << (2 * iChar);

        ++length;
      }
    }

    *dest++ = seq;
    *lengths++ = length;
  }

  fclose(input);
  return readLines;
}

///////////////////////////////////////////////////////////////////////////////
bool DumpScores(int8_t * scores, uint32_t numScores, const char * fileName)
{
  FILE * output;

  if ( (output = fopen(fileName, "wb")) == NULL ) {
    printf("Error opening file [%s]\n", fileName);
    return false;
  }

  // Using I/O functions on non-cacheable memory can give trouble.
  /*if ( fwrite(scores, sizeof(int8_t), numScores, output) != numScores ) {
    printf("Error writing scores.\n");
    return false;
  }*/

  for (uint32_t iScore = 0; iScore < numScores; ++ iScore) {
    int8_t score = scores[iScore];
    if ( fwrite(&score, sizeof(int8_t), 1, output) != 1 ) {
      printf("Error writing scores.\n");
      return false;
    }
  }

  fclose(output);
  return true;
}


///////////////////////////////////////////////////////////////////////////////
uint32_t SeqMatcher_HW(CSeqMatcherDriver * seqMatcher,
    uint32_t numDBEntries, uint32_t numSeqsSpecimen,
    uint64_t * seqsDB, uint64_t * seqsSpecimen, uint8_t * lengthsDB, uint8_t * lengthsSpecimen,
    int8_t * scores, uint64_t & elapsedTime, double & cpuUtilization)
{
  struct timespec start, end;
  struct timespec startCPUTime, endCPUTime;
  uint32_t numComparisons = 0xdeadbeef; 

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, & startCPUTime);
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  seqMatcher->SeqMatcher_HW(numDBEntries, numSeqsSpecimen, seqsDB, seqsSpecimen, lengthsDB, lengthsSpecimen, scores, numComparisons);
  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, & endCPUTime);
  elapsedTime = CalcTimeDiff(end, start);
  cpuUtilization = (double)CalcTimeDiff(endCPUTime, startCPUTime) / elapsedTime;

  return numComparisons;
}


///////////////////////////////////////////////////////////////////////////////
int main(int argc, char ** argv)
{
  uint32_t numDBEntries;
  uint32_t numSeqsSpecimen;
  char * databaseTitle = NULL;
  char * specimenTitle = NULL;
  char * scoresTitle = NULL;
  uint64_t * seqsDB, * seqsSpecimen; // Have to be allocated for DMA access
  uint8_t * lengthsDB, * lengthsSpecimen; // Have to be allocated for DMA access
  int8_t * scores; // Has to be allocated for DMA access
  uint64_t elapsedTime;
  double cpuUtilization;
  bool res = true;
  
  // Obtain arguments from command line.
  setlocale(LC_NUMERIC, "en_US.utf8");  // Enables printing human-readable numbers with %'
  printf("\n");
  if ( (argc != 6) || 
       (sscanf(argv[1], "%u", &numDBEntries) != 1) ||
       (sscanf(argv[2], "%u", &numSeqsSpecimen) != 1) )
  {
    printf("Matches variable-length sequences from one specimen file against a sequence database.\n\n");
    printf("Usage: seqMatcherSW numDBEntries numSeqsSpecimen databaseFile specimenFile scoresFile\n\n");
    printf("Example: ./seqMatcherSW 10000 1000 database.txt specimen.txt scores.bin\n\n");
    return -1;
  }
  databaseTitle = argv[3];
  specimenTitle = argv[4];
  scoresTitle = argv[5];
  printf("Matching %'u DB entries against a specimen with %'u sequences.\n", numDBEntries, numSeqsSpecimen);
  printf("Database file: [%s]\n", databaseTitle);
  printf("Specimen file: [%s]\n", specimenTitle);
  printf("Scores file: [%s]\n", scoresTitle);


  // Initialize device and obtain memory for all the data arrays.
  CSeqMatcherDriver seqMatcher(SHOULD_LOG);
  if (!InitDevice(seqMatcher, numDBEntries, numSeqsSpecimen, seqsDB, seqsSpecimen, lengthsDB, lengthsSpecimen, scores))
    return -1;

  // Read the database and the specimen file
  if (res) {
    printf("Reading database file [%s]...\n", databaseTitle);
    uint32_t readLines;
    readLines = ReadLines(seqsDB, lengthsDB, databaseTitle, numDBEntries);
    if (readLines != numDBEntries) {
      printf("Error reading database: Read %'u lines instead of %'u\n", readLines, numDBEntries);
      res = false;
    }
    else
      printf("Read %'u lines from the DB\n", readLines);
  }

  if (res) {
    printf("Reading specimen file [%s]...\n", specimenTitle);
    uint32_t readLines;
    readLines = ReadLines(seqsSpecimen, lengthsSpecimen, specimenTitle, numSeqsSpecimen);
    if (readLines != numSeqsSpecimen) {
      printf("Error reading specimen: Read %'u lines instead of %'u\n", readLines, numSeqsSpecimen);
      res = false;
    }
    else
      printf("Read %'u lines from the specimen\n", readLines);
  }

  // Compute the scores
  if (res) {
    printf("Calculating scores. Num comparisons: %'u * %'u = %'u\n", numDBEntries, numSeqsSpecimen, numDBEntries*numSeqsSpecimen);

    uint32_t comparisons =
      SeqMatcher_HW(&seqMatcher, numDBEntries, numSeqsSpecimen, seqsDB, seqsSpecimen,
                    lengthsDB, lengthsSpecimen, scores, elapsedTime, cpuUtilization);

    assert(comparisons == numDBEntries * numSeqsSpecimen);
    printf("Calculated %'u scores in %0.3lf s (%'" PRIu64 " ns)\n", comparisons, elapsedTime/1e9, elapsedTime);
    printf("Sequence comparisons per second: %'0.3lf\n", numDBEntries*numSeqsSpecimen / (elapsedTime/1e9) );
    printf("CPU utilization percentage: %0.0lf %%\n", (cpuUtilization * 100) / NUM_CORES_IN_SYSTEM );

    printf("Dumping scores...\n");
    DumpScores(scores, numDBEntries*numSeqsSpecimen, scoresTitle);
    printf("Scores dumped.\n");
  }


  // Free DMA memory.
  if (seqsDB != NULL)
    seqMatcher.FreeDMACompatible(seqsDB);
  if (seqsSpecimen != NULL)
    seqMatcher.FreeDMACompatible(seqsSpecimen);
  if (lengthsDB != NULL)
    seqMatcher.FreeDMACompatible(lengthsDB);
  if (lengthsSpecimen != NULL)
    seqMatcher.FreeDMACompatible(lengthsSpecimen);
  if (scores != NULL)
    seqMatcher.FreeDMACompatible(scores);

  return 0;
}

