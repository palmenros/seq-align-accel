#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <ap_int.h>
#include <string>
#include <assert.h>

#include "seqMatcher.h"

//#define NUM_DATABASE_ENTRIES_TO_CHECK 40000
#define NUM_DATABASE_ENTRIES_TO_CHECK 4
#define NUM_TIMES_TO_TEST 1

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

///////////////////////////////////////////////////////////////////////////////
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

  if ( fwrite(scores, sizeof(int8_t), numScores, output) != numScores )
  {
    printf("Error writing scores.\n");
    return false;
  }

  fclose(output);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int run_test(uint32_t numDBEntries) {
  printf("---------------------------------\n");
  printf(" TESTING %d DB entries... \n", numDBEntries);
  printf("---------------------------------\n");

  uint32_t numSeqsSpecimen = 1000;

  char databaseTitle[] = "../../../../../testdata/database.txt";
  char specimenTitle[] = "../../../../../testdata/specimen.txt";
  char scoresTitle[] = "../../../../../testdata/generated_scores_hls.bin";
  const char* goldScoresTitle = "../../../../../testdata/scores.bin";


  uint64_t* seqsDB, *seqsSpecimen;
  uint8_t* lengthsDB, *lengthsSpecimen;
  int8_t* scores;
  bool res = true;

  // Obtain arguments from command line.
  setlocale(LC_NUMERIC, "en_US.utf8");  // Enables printing human-readable numbers with %'
  printf("\nMatching %'u DB entries against a specimen with %'u sequences.\n", numDBEntries, numSeqsSpecimen);
  printf("Database file: [%s]\n", databaseTitle);
  printf("Specimen file: [%s]\n", specimenTitle);
  printf("Scores file: [%s]\n", scoresTitle);

  fflush(stdout);

  // Allocate memory for all the data arrays
  seqsDB = (uint64_t*)malloc(numDBEntries*sizeof(uint64_t));
  lengthsDB = (uint8_t *)malloc(numDBEntries*sizeof(uint8_t));

  seqsSpecimen = (uint64_t*)malloc(numSeqsSpecimen*sizeof(uint64_t));
  lengthsSpecimen = (uint8_t*)malloc(numSeqsSpecimen*sizeof(uint8_t));

  scores = (int8_t *)malloc(numDBEntries*numSeqsSpecimen*sizeof(int8_t));

  if ( (seqsDB == NULL) || (seqsSpecimen == NULL) || (lengthsDB == NULL) || (lengthsSpecimen == NULL) || (scores == NULL) ) {
	printf("Error allocating memory\n");
	res = false;
  }

  // Read the database and the specimen file
  if (res) {
	printf("Reading database file [%s]...\n", databaseTitle);
	fflush(stdout);
	uint32_t readLines;
	readLines = ReadLines(seqsDB, lengthsDB, databaseTitle, numDBEntries);
	if (readLines != numDBEntries) {
	  printf("Error reading database: Read %'u lines instead of %'u\n", readLines, numDBEntries);
	  res = false;
	}
	else
	  printf("Read %'u lines from the DB\n", readLines);
  }

  fflush(stdout);

  if (res) {
	printf("Reading specimen file [%s]...\n", specimenTitle);
	fflush(stdout);
	uint32_t readLines;
	readLines = ReadLines(seqsSpecimen, lengthsSpecimen, specimenTitle, numSeqsSpecimen);
	if (readLines != numSeqsSpecimen) {
	  printf("Error reading specimen: Read %'u lines instead of %'u\n", readLines, numSeqsSpecimen);
	  res = false;
	}
	else
	  printf("Read %'u lines from the specimen\n", readLines);
  }

  fflush(stdout);

  // Compute the scores
  if (res) {
	printf("Calculating scores. Num comparisons: %'u * %'u = %'u\n", numDBEntries, numSeqsSpecimen, numDBEntries*numSeqsSpecimen);
	uint32_t comparisons = SeqMatcher_HW(numDBEntries, numSeqsSpecimen, seqsDB, seqsSpecimen, lengthsDB, lengthsSpecimen, scores);

	assert(comparisons == numDBEntries * numSeqsSpecimen);
	printf("Calculated %'u scores\n", comparisons);

	printf("Dumping scores to file ...\n");
	DumpScores(scores, numDBEntries*numSeqsSpecimen, scoresTitle);
	printf("Scores dumped.\n");
  }

  // Compare the scores with the golden output
  if (res) {
	  printf("Comparing scores to gold...\n\n");

	  fflush(stdout);

	  std::string comparisonCommand = std::string("cmp -n ") + std::to_string(numDBEntries * numSeqsSpecimen) + " " + scoresTitle + " " + goldScoresTitle;
	  int rc = system(comparisonCommand.c_str());
	  if (rc == 0) {
		  std::cout << std::endl << "SUCCESS!: Output matches golden" << std::endl << std::endl;
		  res = true;
	  } else {
		  std::cout << std::endl << "ERROR!: Output does not match golden" << std::endl << std::endl;
		  res = false;
	  }
  }

  // Free array memory.
  if (seqsDB != NULL)
	free(seqsDB);
  if (seqsSpecimen != NULL)
	free(seqsSpecimen);
  if (lengthsDB != NULL)
	free(lengthsDB);
  if (lengthsSpecimen != NULL)
	free(lengthsSpecimen);
  if (scores != NULL)
	free(scores);

 return res ? 0 : -1;
}


int main(int argc, char ** argv)
{
  for(int i = 0; i < NUM_TIMES_TO_TEST; ++i) {
	  if(run_test(NUM_DATABASE_ENTRIES_TO_CHECK) != 0) {
		  printf("---------------------------------\n");
		  printf(" SOME TEST FAILED \n");
		  printf("---------------------------------\n");
		  return -1;
	  }
  }

  printf("---------------------------------\n");
  printf(" ALL TESTS PASSED \n");
  printf("---------------------------------\n");

  return 0;
}

