/*********************** Author: Mainak Chaudhuri ****************/

#include <stdio.h>
#include "pin.H"

#define LOG_BLOCK_SIZE 6
#define BLOCK_SIZE (1 << LOG_BLOCK_SIZE)
#define BLOCK_MASK 0x3f

FILE * trace;
FILE *trace_l;
UINT64 trace_length;
PIN_LOCK pinLock;

// Print a memory read record
VOID RecordMemAccess(VOID * addr, USIZE size, THREADID tid,USIZE flag)
{
    UINT64 start = (UINT64)addr;
    USIZE size1;
    UINT64 start1, end1, elem;
    UINT32 num8, num4, num2, num1, i;

    start1 = start;
    end1 = start1 | BLOCK_MASK;
    if (start1 + size - 1 <= end1) {
       num8 = size >> 3;
       size1 = size & 0x7;
       num4 = size1 >> 2;
       size1 = size1 & 0x3;
       num2 = size1 >> 1;
       num1 = size1 & 0x1;
       for (i=0; i<num8; i++) {
          elem = (((start1 + (i << 3)) << 3) | tid) << 1 | flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
       if (num4) {
          elem = (((start1 + (num8 << 3)) << 3) | tid) << 1 | flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
       if (num2) {
          elem = (((start1 + (num8 << 3) + (num4 << 2)) << 3) | tid) << 1 | flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
       if (num1) {
          elem = (((start1 + (num8 << 3) + (num4 << 2) + (num2 << 1)) << 3) | tid) << 1 | flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
    }
    else {
       size1 = end1 - start1 + 1;
       num8 = size1 >> 3;
       size1 = size1 & 0x7;
       num4 = size1 >> 2;
       size1 = size1 & 0x3;
       num2 = size1 >> 1;
       num1 = size1 & 0x1;
       for (i=0; i<num8; i++) {
          elem = (((start1 + (i << 3)) << 3) | tid) << 1 | flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
       if (num4) {
          elem = (((start1 + (num8 << 3)) << 3) | tid) << 1| flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
       if (num2) {
          elem = (((start1 + (num8 << 3) + (num4 << 2)) << 3) | tid) << 1 | flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
       if (num1) {
          elem = (((start1 + (num8 << 3) + (num4 << 2) + (num2 << 1)) << 3) | tid) << 1| flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }

       size1 = size - (end1 - start1 + 1);
       start1 = end1 + 1;
       while (size1 > BLOCK_SIZE) {
          for (i=0; i<BLOCK_SIZE >> 3; i++) {
             elem = (((start1 + (i << 3)) << 3) | tid) << 1 | flag;
             PIN_GetLock(&pinLock, tid+1);
             fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
             trace_length++;
             PIN_ReleaseLock(&pinLock);
          }
          size1 = size1 - BLOCK_SIZE;
          start1 = start1 + BLOCK_SIZE;
       }

       num8 = size1 >> 3;
       size1 = size1 & 0x7;
       num4 = size1 >> 2;
       size1 = size1 & 0x3;
       num2 = size1 >> 1;
       num1 = size1 & 0x1;
       for (i=0; i<num8; i++) {
          elem = (((start1 + (i << 3)) << 3) | tid) << 1 | flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
       if (num4) {
          elem = (((start1 + (num8 << 3)) << 3) | tid) << 1 | flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
       if (num2) {
          elem = (((start1 + (num8 << 3) + (num4 << 2)) << 3) | tid) << 1 | flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
       if (num1) {
          elem = (((start1 + (num8 << 3) + (num4 << 2) + (num2 << 1)) << 3) | tid) << 1 | flag;
          PIN_GetLock(&pinLock, tid+1);
          fwrite(&elem, sizeof(elem), 1, trace); fflush(trace);
          trace_length++;
          PIN_ReleaseLock(&pinLock);
       }
    }
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemAccess,
                IARG_MEMORYOP_EA, memOp, IARG_UINT32, INS_MemoryOperandSize(ins, memOp), IARG_THREAD_ID,IARG_UINT32,0,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemAccess,
                IARG_MEMORYOP_EA, memOp,  IARG_UINT32, INS_MemoryOperandSize(ins, memOp), IARG_THREAD_ID,IARG_UINT32,1,
                IARG_END);
        }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    fprintf(trace_l, "%lu\n", trace_length);
    fclose(trace);
    fclose(trace_l);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    trace = fopen("addrtrace.out", "wb");
    trace_l = fopen("addrtrace_l.out", "w");
    trace_length = 0;

    PIN_InitLock(&pinLock);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}
