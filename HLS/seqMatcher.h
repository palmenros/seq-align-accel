#ifndef SEQMATCHER_H
#define SEQMATCHER_H

#include <ap_int.h>
#include <hls_vector.h>
#include <hls_burst_maxi.h>

#define MAX_SEQ_LENGTH 32

using nbase_t = ap_uint<2>;

const nbase_t NB_A = 0;
const nbase_t NB_T = 1;
const nbase_t NB_G = 2;
const nbase_t NB_C = 3;

#define SCORE_NUM_BITS 5

using score_t = ap_uint<SCORE_NUM_BITS>;

using seq_t = hls::vector<nbase_t, MAX_SEQ_LENGTH>;

////////////////////////////////

uint32_t SeqMatcher_HW(
	uint32_t numDBEntries, uint32_t numSeqsSpecimen,
	uint64_t* seqsDB,
	uint64_t* seqsSpecimen,
	uint8_t* lengthsDB, uint8_t* lengthsSpecimen,
	hls::burst_maxi<int8_t> scores
);


int8_t CalcScoreLinearSystolicArray(seq_t seqA, uint8_t lengthA, seq_t seqB, uint8_t lengthB);

#endif // SEQMATCHER_H

