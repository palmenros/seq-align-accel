#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <map>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "util.hpp"
#include "CAccelDriver.hpp"
#include "CSeqMatcherDriver.hpp"

uint32_t CSeqMatcherDriver::SeqMatcher_HW(uint32_t numDBEntries, uint32_t numSeqsSpecimen,
    void * seqsDB, void * seqsSpecimen, void * lengthsDB, void * lengthsSpecimen,
    void * scores, uint32_t &numComparisons)
{
  uint32_t phySeqsDB, phySeqsSpecimen, phyLengthsDB, phyLengthsSpecimen, phyScores;
  uint32_t status;

  if (logging)
    printf("CSeqMatcherDriver::SeqMatcher_HW():\n\tnumDBEnttries=%u\n\tnumSeqsSpecimen=%u\n\tseqsDB=0x%08X\n\tseqsSpecimen=0x%08X\n\t"
          "lengtsDB=0x%08X\n\tlengthsSpecimen=0x%08X\n\tscores=0x%08X\n\n", 
          (uint32_t)numDBEntries, (uint32_t)numSeqsSpecimen, (uint32_t)seqsDB, (uint32_t)seqsSpecimen,
          (uint32_t)lengthsDB, (uint32_t)lengthsSpecimen, (uint32_t)scores);

  if (driver == 0) {
    if (logging)
      printf("Error: Calling SeqMatcher_HW() on a non-initialized accelerator.\n");
    return DEVICE_NOT_INITIALIZED;
  }

  // We need to obtain the physical addresses corresponding to each of the virtual addresses passed by the application.
  // The accelerator uses only the physical addresses (and only contiguous memory).
  phySeqsDB = GetDMAPhysicalAddr(seqsDB);
  if (phySeqsDB == 0) {
    if (logging)
      printf("Error: No physical address found for virtual address 0x%08X\n", (uint32_t)seqsDB);
    return VIRT_ADDR_NOT_FOUND;
  }
  phySeqsSpecimen = GetDMAPhysicalAddr(seqsSpecimen);
  if (phySeqsSpecimen == 0) {
    if (logging)
      printf("Error: No physical address found for virtual address 0x%08X\n", (uint32_t)seqsSpecimen);
    return VIRT_ADDR_NOT_FOUND;
  }
  phyLengthsDB = GetDMAPhysicalAddr(lengthsDB);
  if (phyLengthsDB == 0) {
    if (logging)
      printf("Error: No physical address found for virtual address 0x%08X\n", (uint32_t)lengthsDB);
    return VIRT_ADDR_NOT_FOUND;
  }
  phyLengthsSpecimen = GetDMAPhysicalAddr(lengthsSpecimen);
  if (phyLengthsSpecimen == 0) {
    if (logging)
      printf("Error: No physical address found for virtual address 0x%08X\n", (uint32_t)lengthsSpecimen);
    return VIRT_ADDR_NOT_FOUND;
  }
  phyScores = GetDMAPhysicalAddr(scores);
  if (phyScores == 0) {
    if (logging)
      printf("Error: No physical address found for virtual address 0x%08X\n", (uint32_t)scores);
    return VIRT_ADDR_NOT_FOUND;
  }

  struct user_message message = {
      numDBEntries,
      numSeqsSpecimen,
      (uint32_t) phySeqsDB,
      (uint32_t) phySeqsSpecimen,
      (uint32_t) phyLengthsDB,
      (uint32_t) phyLengthsSpecimen,
      (uint32_t) phyScores,

      (uint32_t)(&numComparisons)
  };

  if (logging)
    printf("\nStarting accel...\n");

  int32_t readBytes = read(driver, (void *)&message, sizeof(message));
  if (readBytes != 0)
    printf("Warning! Read %d bytes instead than %d\n", readBytes, 0);

  return OK;
}


