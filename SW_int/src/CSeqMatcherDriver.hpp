#ifndef CSEQMATCHERDRIVER_HPP
#define CSEQMATCHERDRIVER_HPP

class CSeqMatcherDriver : public CAccelDriver {
  protected:

  // Structure used to pass commands between user-space and kernel-space.
  struct user_message {
      uint32_t numDBEntries;
      uint32_t numSeqsSpecimen;
      uint32_t seqsDB;
      uint32_t seqsSpecimen;
      uint32_t lengthsDB;
      uint32_t lengthsSpecimen;
      uint32_t scores;

      uint32_t numComparisonsPtr;
  };
  
  public:
    CSeqMatcherDriver(bool Logging = false)
      : CAccelDriver(Logging) {}

    ~CSeqMatcherDriver() {}

    uint32_t SeqMatcher_HW(uint32_t numDBEntries, uint32_t numSeqsSpecimen,
        void * seqsDB, void * seqsSpecimen, void * lengthsDB, void * lengthsSpecimen,
        void * scores, uint32_t & numComparisons);
};

#endif  // CSEQMATCHERDRIVER_HPP

