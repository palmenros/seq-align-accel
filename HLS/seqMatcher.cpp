#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ap_int.h>
#include <assert.h>
#include <hls_vector.h>
#include <hls_stream.h>

#define NUM_SYSTOLIC_ARRAYS 20

#define DUPLICATION_FACTOR_SPECIMEN_CACHE ((NUM_SYSTOLIC_ARRAYS + 1) / 2)
#define INPUT_AXI_STREAM_BUFFER_SIZE NUM_SYSTOLIC_ARRAYS

#define WORKER_INPUT_STREAM_DEPTH 512
#define WORKER_OUTPUT_STREAM_DEPTH 4000

#include "seqMatcher.h"

static int ceil_div(int x, int y) {
	return (x + y - 1) / y;
}

#define MAX_CACHED_SPECIMENS 1000

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline score_t max(score_t a, score_t b) {
	return a > b ? a : b;
}

inline score_t max3(score_t a, score_t b, score_t c) {
    return a > b ? (a > c ? a : c) : (b > c ? b : c);
}

// Compute the maximum of all the elements in the systolic array
inline score_t maxReduce(score_t maxScores[MAX_SEQ_LENGTH]) {

	assert(MAX_SEQ_LENGTH == 32);

	// Maximums in layer 0
	score_t layer0[16];
#pragma HLS ARRAY_PARTITION variable=layer0 type=complete

	for(int i = 0; i < 16; ++i) {
#pragma HLS UNROLL
		layer0[i] = max(maxScores[2*i], maxScores[2*i+1]);
	}

	// Maximums in layer 1
	score_t layer1[8];
#pragma HLS ARRAY_PARTITION variable=layer1 type=complete

	for(int i = 0; i < 8; ++i) {
#pragma HLS UNROLL
		layer1[i] = max(layer0[2*i], layer0[2*i+1]);
	}

	// Maximums in layer 2
	score_t layer2[4];
#pragma HLS ARRAY_PARTITION variable=layer2 type=complete

	for(int i = 0; i < 4; ++i) {
#pragma HLS UNROLL
		layer2[i] = max(layer1[2*i], layer1[2*i+1]);
	}

	// Maximums in layer 3
	score_t layer3[2];
#pragma HLS ARRAY_PARTITION variable=layer3 type=complete

	for(int i = 0; i < 2; ++i) {
#pragma HLS UNROLL
		layer3[i] = max(layer2[2*i], layer2[2*i+1]);
	}

	return max(layer3[0], layer3[1]);
}

inline uint8_t maxLength(uint8_t a, uint8_t b) {
	return a > b ? a : b;
}

int8_t CalcScoreLinearSystolicArray(seq_t seqA, uint8_t lengthA, seq_t seqB, uint8_t lengthB) {

  // Max score that a given cell in the systolic array has seen until this moment. We will then only compute the final
  // global maximum at the end of the algorithm to prevent data dependencies.
  score_t maxScores[MAX_SEQ_LENGTH];
  #pragma HLS ARRAY_PARTITION variable=maxScores type=complete

	nbase_t seq_b_SR[2*MAX_SEQ_LENGTH];
	#pragma HLS ARRAY_PARTITION variable=seq_b_SR type=complete

	// Load the shift register with the first nucleobases of seqB
	for(int i = 0; i < MAX_SEQ_LENGTH; ++i) {
		#pragma HLS UNROLL
		seq_b_SR[i] = 0;
		seq_b_SR[MAX_SEQ_LENGTH + i] = seqB[i];
	}

  score_t scores[MAX_SEQ_LENGTH];

	// Scores of the older diagonal (scores but delayed one iteration). It is needed
	// because when computing M[i-1][j-1] is not in the previous diagonal, is two
	// diagonals away
	score_t oldScores[MAX_SEQ_LENGTH];

	#pragma HLS ARRAY_PARTITION variable=scores type=complete
	#pragma HLS ARRAY_PARTITION variable=oldScores type=complete

	for(int i = 0; i < MAX_SEQ_LENGTH; ++i) {
		#pragma HLS UNROLL
		scores[i] = score_t(0);
		oldScores[i] = score_t(0);
		maxScores[i] = score_t(0);
	}

	uint8_t totalDiagNumber = lengthB + lengthA - 1;

	diagLoop:  for(uint8_t iDiag = 0; iDiag < totalDiagNumber ; ++iDiag) {
	#pragma HLS PIPELINE
	#pragma HLS LOOP_TRIPCOUNT min=16 max=32

	  for(int j = MAX_SEQ_LENGTH - 1; j > 0; j--) {
		#pragma HLS UNROLL

		  score_t top_lhs = ((j == iDiag) ? score_t(0) : scores[j]);
		  score_t top = top_lhs == 0 ? top_lhs : score_t(top_lhs - score_t(1));

		  score_t left_lhs = scores[j-1];
		  score_t left = left_lhs == 0 ? left_lhs : score_t(left_lhs - score_t(1));

		  int8_t idxSeqA = j;
		  int8_t idxSeqB = iDiag - j;

		  score_t hit = 0;

		  if (idxSeqA < lengthA && idxSeqB >= 0 && idxSeqB < lengthB) {
			  score_t lhs = oldScores[j-1];
			  hit = ((seqA[idxSeqA] == seq_b_SR[32-j]) ? score_t(lhs+score_t(1)) : ( lhs == 0 ? lhs : score_t(lhs-score_t(1))));
		  }

		  score_t newScore = max3(top, left, hit);

		  if (newScore < 0) {
			  newScore = 0;
		  }

		  oldScores[j] = scores[j];
		  scores[j] = newScore;

		  score_t maxScore = max(maxScores[j], newScore);
		  maxScores[j] = maxScore;
	  }

	  score_t top_lhs = ((0 == iDiag) ? score_t(0) : scores[0]);
	  score_t top = top_lhs == 0 ? top_lhs : score_t(top_lhs - score_t(1));

	  score_t hit = ((seqA[0] == seq_b_SR[32]) ? score_t(1) : score_t(0));

	  score_t newScore = max(top, hit);

	  if (newScore < 0) {
		 newScore = 0;
      }

	  oldScores[0] = scores[0];
	  scores[0] = newScore;
	  maxScores[0] = max(maxScores[0], newScore);

		// Shift seqB to the left
	  for(int i = 0; i < 2*MAX_SEQ_LENGTH - 1; ++i) {
		  #pragma HLS UNROLL
		  seq_b_SR[i] = seq_b_SR[i+1];
	  }
  }

  return maxReduce(maxScores);
}

// Load all specimens into the cache. For now, assume that all specimens fit into the BRAM cache.
inline void loadSpecimenCache(
		uint64_t* seqsSpecimen,																	// Pointer to DRAM seqs specimen (already compressed: one nucleobase=2bits)
		uint8_t* lengthsSpecimen,																// Pointer to DRAM array containing the lengths of each specimen.
		uint8_t cachedSpecimenLengths[DUPLICATION_FACTOR_SPECIMEN_CACHE][MAX_CACHED_SPECIMENS],	// Output array with all the cached specimen lengths
		uint64_t cachedSpecimens[DUPLICATION_FACTOR_SPECIMEN_CACHE][MAX_CACHED_SPECIMENS],		// Output array storing all the cached specimens
		uint32_t numSeqsSpecimen																// Number of specimens the program wants to use
) {
	uint32_t max_specimens_to_cache = numSeqsSpecimen > MAX_CACHED_SPECIMENS ? MAX_CACHED_SPECIMENS : numSeqsSpecimen;

specCacheLoop:	for(int iSpecimen = 0; iSpecimen < max_specimens_to_cache; ++iSpecimen) {
#pragma HLS PIPELINE
		 for (int i = 0; i < DUPLICATION_FACTOR_SPECIMEN_CACHE; ++i){
		#pragma HLS UNROLL
				cachedSpecimens[i][iSpecimen] = seqsSpecimen[iSpecimen];
				cachedSpecimenLengths[i][iSpecimen] = lengthsSpecimen[iSpecimen];
		}
	}
}

struct WorkerInput {
	ap_uint<64> seqDB;
	uint8_t lengthDB;
};

inline seq_t seqFromUInt64(ap_uint<64> x) {
	seq_t res;

	res[0] = x.range(1, 0);
	res[1] = x.range(3, 2);
	res[2] = x.range(5, 4);
	res[3] = x.range(7, 6);
	res[4] = x.range(9, 8);
	res[5] = x.range(11, 10);
	res[6] = x.range(13, 12);
	res[7] = x.range(15, 14);
	res[8] = x.range(17, 16);
	res[9] = x.range(19, 18);
	res[10] = x.range(21, 20);
	res[11] = x.range(23, 22);
	res[12] = x.range(25, 24);
	res[13] = x.range(27, 26);
	res[14] = x.range(29, 28);
	res[15] = x.range(31, 30);
	res[16] = x.range(33, 32);
	res[17] = x.range(35, 34);
	res[18] = x.range(37, 36);
	res[19] = x.range(39, 38);
	res[20] = x.range(41, 40);
	res[21] = x.range(43, 42);
	res[22] = x.range(45, 44);
	res[23] = x.range(47, 46);
	res[24] = x.range(49, 48);
	res[25] = x.range(51, 50);
	res[26] = x.range(53, 52);
	res[27] = x.range(55, 54);
	res[28] = x.range(57, 56);
	res[29] = x.range(59, 58);
	res[30] = x.range(61, 60);
	res[31] = x.range(63, 62);

	return res;
}

void SystolicArrayWorker(
		uint8_t systolicArrayId,
		hls::stream<WorkerInput>& in,
		hls::stream<int8_t>& out, uint32_t numSeqsSpecimen, uint32_t numDBEntries,
		uint64_t cachedSpecimens[DUPLICATION_FACTOR_SPECIMEN_CACHE][MAX_CACHED_SPECIMENS],
		uint8_t cachedSpecimenLengths[DUPLICATION_FACTOR_SPECIMEN_CACHE][MAX_CACHED_SPECIMENS]
) {
	const uint8_t cacheIndex = systolicArrayId >> 1;

	uint32_t numDBEntriesToProcess = numDBEntries / NUM_SYSTOLIC_ARRAYS + (systolicArrayId < (numDBEntries % NUM_SYSTOLIC_ARRAYS) ? 1 : 0);

	for(int iDB = 0; iDB < numDBEntriesToProcess; ++iDB) {
		WorkerInput input = in.read();

		seq_t seqA = seqFromUInt64(input.seqDB);
		uint8_t lengthA = input.lengthDB;

		for(uint32_t iSpec = 0; iSpec < numSeqsSpecimen; ++iSpec) {
			seq_t seqB = seqFromUInt64(cachedSpecimens[cacheIndex][iSpec]);
			uint8_t lengthB = cachedSpecimenLengths[cacheIndex][iSpec];

			int8_t res = (lengthA == 32 && lengthB == 32 && seqA == seqB) ? 32 : CalcScoreLinearSystolicArray(seqA, lengthA, seqB, lengthB);
			out.write(res);
		}
	}

}

void ReadSystolicArrayInputs(
		uint64_t* seqsDB,
		uint8_t* lengthsDB,
		uint32_t numDBEntries,
		hls::stream<WorkerInput> out[NUM_SYSTOLIC_ARRAYS]
) {

	uint8_t streamIdx = 0;

readDbLoop: for (uint32_t iDB = 0; iDB < numDBEntries; ++iDB) {
#pragma HLS LOOP_TRIPCOUNT min=40000 max=40000
#pragma HLS PIPELINE
		uint64_t seqDB = seqsDB[iDB];
		uint8_t dbLength = lengthsDB[iDB];

		  WorkerInput input = {
			seqDB,
			dbLength,
		  };

		  out[streamIdx].write(input);
		  streamIdx++;
		  if (streamIdx == NUM_SYSTOLIC_ARRAYS) {
			  streamIdx = 0;
		  }
	}

}

void WriteSystolicArrayResults(
	hls::burst_maxi<int8_t> scores,
	uint32_t numDBEntries,
	uint32_t numSeqsSpecimen,
	hls::stream<int8_t> in[NUM_SYSTOLIC_ARRAYS]
) {

	uint32_t streamIdx = 0;

	scores.write_request(0, numDBEntries * numSeqsSpecimen);

writeDBLoop: for (uint32_t iDB = 0; iDB  < numDBEntries; ++iDB ) {
writeSpecLoop: for(uint32_t iSpec = 0; iSpec < numSeqsSpecimen; ++iSpec) {
			int8_t val = in[streamIdx].read();
			scores.write(val);
		}

		// Wrap streamIdx around
		streamIdx++;
		if(streamIdx == NUM_SYSTOLIC_ARRAYS) {
			streamIdx = 0;
		}
    }

	scores.write_response();
}

void SeqMatchMultipleSystolicArrays(
		uint32_t numDBEntries,
		uint32_t numSeqsSpecimen,
		uint64_t* seqsDB,
		uint8_t* lengthsDB,
		uint64_t cachedSpecimens[DUPLICATION_FACTOR_SPECIMEN_CACHE][MAX_CACHED_SPECIMENS],
		uint8_t cachedSpecimenLengths[DUPLICATION_FACTOR_SPECIMEN_CACHE][MAX_CACHED_SPECIMENS],
		hls::burst_maxi<int8_t> scores
) {

	hls::stream<WorkerInput, WORKER_INPUT_STREAM_DEPTH> inputStreams[NUM_SYSTOLIC_ARRAYS];
	hls::stream<int8_t, WORKER_OUTPUT_STREAM_DEPTH> outputStreams[NUM_SYSTOLIC_ARRAYS];

#pragma HLS DATAFLOW

    ReadSystolicArrayInputs(seqsDB, lengthsDB, numDBEntries, inputStreams);

    for (int i=0; i<NUM_SYSTOLIC_ARRAYS; ++i) {
#pragma HLS unroll
    	SystolicArrayWorker(i, inputStreams[i], outputStreams[i], numSeqsSpecimen, numDBEntries, cachedSpecimens, cachedSpecimenLengths);
    }

    WriteSystolicArrayResults(scores, numDBEntries, numSeqsSpecimen, outputStreams);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint32_t SeqMatcher_HW(
	uint32_t numDBEntries, uint32_t numSeqsSpecimen,
	uint64_t* seqsDB,
	uint64_t* seqsSpecimen,
	uint8_t* lengthsDB, uint8_t* lengthsSpecimen,
	hls::burst_maxi<int8_t> scores
) {

#pragma HLS INTERFACE mode=s_axilite port=numDBEntries
#pragma HLS INTERFACE mode=s_axilite port=numSeqsSpecimen
#pragma HLS INTERFACE mode=s_axilite port=return

#pragma HLS INTERFACE mode=m_axi port=seqsDB bundle=seqs num_read_outstanding=2 max_read_burst_length=256 latency=30
#pragma HLS INTERFACE mode=m_axi port=seqsSpecimen bundle=seqs num_read_outstanding=2 max_read_burst_length=256 latency=30
#pragma HLS INTERFACE mode=m_axi port=lengthsDB bundle=lengths num_read_outstanding=2 max_read_burst_length=256 latency=30
#pragma HLS INTERFACE mode=m_axi port=lengthsSpecimen bundle=lengths num_read_outstanding=2 max_read_burst_length=256 latency=30
#pragma HLS INTERFACE mode=m_axi port=scores bundle=scores num_write_outstanding=2 max_write_burst_length=256 latency=30

  // Specimen caches
  uint8_t cachedSpecimenLengths[DUPLICATION_FACTOR_SPECIMEN_CACHE][MAX_CACHED_SPECIMENS];
#pragma HLS ARRAY_PARTITION variable=cachedSpecimenLengths complete dim=1

  uint64_t cachedSpecimens[DUPLICATION_FACTOR_SPECIMEN_CACHE][MAX_CACHED_SPECIMENS];
#pragma HLS ARRAY_PARTITION variable=cachedSpecimens complete dim=1

  loadSpecimenCache(seqsSpecimen, lengthsSpecimen, cachedSpecimenLengths, cachedSpecimens, numSeqsSpecimen);

  assert(numSeqsSpecimen <= MAX_CACHED_SPECIMENS);

  // Right now, we assume that MAX_SEQ_LENGTH is even
  assert((MAX_SEQ_LENGTH & 1) == 0);

  uint32_t numComparisons = numDBEntries * numSeqsSpecimen;

  SeqMatchMultipleSystolicArrays(
		numDBEntries,
		numSeqsSpecimen,
		seqsDB,
		lengthsDB,
		cachedSpecimens,
		cachedSpecimenLengths,
		scores
	);

 return numComparisons;

}

