#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define MAX_MUTATIONS 1024
#define MIN_SEQ_LENGTH  16

template<typename T>
bool GenListUniqueRandoms(T * values, uint32_t length, T moduleValue)
{
  uint32_t count = 0;

  if (moduleValue < length)
    return false;

  while (count < length) {
    T newValue = rand() % moduleValue;
    bool found = false;
    for (uint32_t ii = 0; !found && (ii < count); ++ ii)
      found = (values[ii] == newValue);
    if (!found) {
      values[count] = newValue;
      ++ count;
    }
  }
  return true;
}

uint32_t NucleotideToInt(char n)
{
  switch (n) {
  case 'A': return 0; break;
  case 'C': return 1; break;
  case 'G': return 2; break;
  case 'T': return 3; break;
  default: return 9999; break;
  }
}

char GenRandomNucleotide(char previousToAvoid)
{
  uint32_t value;
  uint32_t previous = NucleotideToInt(previousToAvoid);

  do {
    value = rand() % 4;
  } while (value == previous);

  switch (value) {
  case 0:
    return 'A'; break;
  case 1:
    return 'C'; break;
  case 2:
    return 'G'; break;
  case 3:
    return 'T'; break;
  default:
    return '-'; break;
  }
}

void MutateSeq(char * oldSeq, char * newSeq, uint32_t length, uint32_t maxMutations)
{
  uint32_t indexes[MAX_MUTATIONS];
  uint32_t numMutations;

  numMutations = rand() % (maxMutations + 1); // There may be zero mutations, and up to including maxMutations

  memcpy(newSeq, oldSeq, length);
  GenListUniqueRandoms<uint32_t>(indexes, numMutations, length);
  for (uint32_t ii = 0; ii < numMutations; ++ ii) {
    newSeq[indexes[ii]] = GenRandomNucleotide(newSeq[indexes[ii]]);
    //printf("Changing index %u from %c to %c\n", indexes[ii], oldSeq[indexes[ii]], newSeq[indexes[ii]]);
  }
}

void PrintSequence(char * seq, uint32_t length, FILE * outputFile = stdout)
// Prints non-NULL terminated sequences.
{
  for (uint32_t ii = 0; ii < length; ++ ii)
    fprintf(outputFile, "%c", seq[ii]);
  fprintf(outputFile, "\n");
}

void PrintSequenceDiffs(char * orig, char * mutated, uint32_t length)
// Prints non-NULL terminated sequences.
{
  for (uint32_t ii = 0; ii < length; ++ ii)
    printf("%c", orig[ii] == mutated[ii] ? orig[ii] : mutated[ii]+32);
  printf("\n");
}

void GenSequence(char * seq, uint32_t length)
{
  for (uint32_t ii = 0; ii < length; ++ ii)
    seq[ii] = GenRandomNucleotide('-');
}

bool IsSequenceInList(char * sequences, char * seq, uint32_t numSequences, uint32_t length)
{
  bool found = false;

  for (uint32_t iSeq = 0; !found && (iSeq < numSequences); ++ iSeq) {
    found = true;
    for (uint32_t iChar = 0; found && (iChar < length); ++ iChar) {
      found = sequences[iSeq*length+iChar] == seq[iChar];
    }
  }

  return found;
}

void FreeResourcesAndExit(void * p1, void * p2, void * p3, FILE * f1, FILE * f2, int32_t exitValue)
{
  if (p1 != NULL)
    free(p1);
  if (p2 != NULL)
    free(p2);
  if (p3 != NULL)
    free(p3);
  if (f1 != NULL)
    fclose(f1);
  if (f2 != NULL)
    fclose(f2);

  exit(exitValue);
}

int main(int argc, char ** argv)
{
  uint32_t numSpecimens, seqsPerSpecimen, maxMutations, maxSeqLength;
  char databaseTitle[256], specimenTitle[256];
  FILE * databaseFile = NULL, * specimenFile = NULL;
  char * sequences = NULL, * newSeq = NULL;
  char * p;
  uint8_t * lengths = NULL, *pLengths;

  if ( (argc != 7) || 
       (sscanf(argv[1], "%u", &numSpecimens) != 1) ||
       (sscanf(argv[2], "%u", &seqsPerSpecimen) != 1) || 
       (sscanf(argv[3], "%u", &maxMutations) != 1) || 
       (sscanf(argv[4], "%u", &maxSeqLength) != 1) ||
       (sscanf(argv[5], "%s", databaseTitle) != 1) ||
       (sscanf(argv[6], "%s", specimenTitle) != 1) )
  {
    fprintf(stderr, "\nUsage: genSequenceFiles numSpecimens seqsPerSpecimen maxMutations maxSeqLength databaseFile specimenFile\n\n");
    fprintf(stderr, "Generates variable-length sequences for all the specimenes. Then, it generates another file with mutations for all the sequences of the last specimen.\n\n");
    return -1;
  }
  fprintf(stderr, "Generating %u specimens with %u sequences each of length up to %u. MaxMutations in specimen: %u\n",
    numSpecimens, seqsPerSpecimen, maxSeqLength, maxMutations);
  fprintf(stderr, "Database file: [%s]\n", databaseTitle);
  fprintf(stderr, "Specimen file: [%s]\n", specimenTitle);
  if (maxSeqLength > 255) {
    fprintf(stderr, "Maximum sequence length cannot be larger than 255.\n");
    FreeResourcesAndExit(NULL, NULL, NULL, NULL, NULL, -1);
  }

  if ( (databaseFile = fopen(databaseTitle, "wt")) == NULL ) {
    fprintf(stderr, "Impossible to open file [%s]\n", databaseTitle);
    FreeResourcesAndExit(sequences, newSeq, lengths, databaseFile, specimenFile, -1);
  }
  if ( (specimenFile = fopen(specimenTitle, "wt")) == NULL ) {
    fprintf(stderr, "Impossible to open file [%s]\n", specimenTitle);
    FreeResourcesAndExit(sequences, newSeq, lengths, databaseFile, specimenFile, -1);
  }

  sequences = (char *)malloc(numSpecimens * seqsPerSpecimen * maxSeqLength * sizeof(char));
  newSeq = (char *)malloc(maxSeqLength * sizeof(char));
  lengths = (uint8_t *)malloc(numSpecimens * seqsPerSpecimen * sizeof(uint8_t));
  if ( (sequences == NULL) || (newSeq == NULL) || (lengths == NULL) ) {
    printf("Error allocating memory\n");
    FreeResourcesAndExit(sequences, newSeq, lengths, databaseFile, specimenFile, -1);
  }

  srand(time(NULL));

  fprintf(stderr, "Generating sequences...\n");
  p = sequences;
  pLengths = lengths;
  for (uint32_t iSpecimen = 0; iSpecimen < numSpecimens; ++ iSpecimen) {
    for (uint32_t iSeq = 0; iSeq < seqsPerSpecimen; ++ iSeq) {
      *pLengths = (rand() % maxSeqLength) + 1;
      if (*pLengths < MIN_SEQ_LENGTH) *pLengths = MIN_SEQ_LENGTH;
      GenSequence(p, *pLengths);
      p += *pLengths;
      ++ pLengths;
    }
  }
  fprintf(stderr, "Generated %u sequences.\n", numSpecimens * seqsPerSpecimen);

  
  fprintf(stderr, "Dumping sequences...\n");
  p = sequences;
  pLengths = lengths;
  for (uint32_t iSpecimen = 0; iSpecimen < numSpecimens; ++ iSpecimen) {
    for (uint32_t iSeq = 0; iSeq < seqsPerSpecimen; ++ iSeq) {
      PrintSequence(p, *pLengths, databaseFile);
      p += *pLengths;
      ++ pLengths;
    }
  }
  fprintf(stderr, "Dumped %u sequences.\n", numSpecimens * seqsPerSpecimen);
  

  fprintf(stderr, "Generating mutations for the last specimen...\n");
  p = sequences;
  pLengths = lengths;
  for (uint32_t iSpecimen = 0; iSpecimen < (numSpecimens - 1); ++ iSpecimen) {
    for (uint32_t iSeq = 0; iSeq < seqsPerSpecimen; ++ iSeq) {
      p += *pLengths;
      ++pLengths;
    }
  }
  for (uint32_t iSeq = 0; iSeq < seqsPerSpecimen; ++ iSeq) {
    MutateSeq(p, newSeq, *pLengths, maxMutations);
    PrintSequence(newSeq, *pLengths, specimenFile);
    p += *pLengths;
    ++ pLengths;
  }


  FreeResourcesAndExit(sequences, newSeq, lengths, databaseFile, specimenFile, -1);
  return 0;
}


