#ifndef _OPENMP_THREADS_HPP
#define _OPENMP_THREADS_HPP

#ifdef HAVE_CONFIG_H
 #include "config.hpp"
#endif

#include "base/debug.hpp"
#include <omp.h>

#include "routines/thread.hpp"

#if defined BGQ && !defined BGQ_EMU
 #include <bgpm/include/bgpm.h>
 #include "bgq/bgq_barrier.hpp"
#endif

#define NACTIVE_THREADS ((thread_pool_locked)?1:nthreads)
#define MANDATORY_PARALLEL if(nthreads>1 && thread_pool_locked) crash("this cannot be called when threads are locked")
#define MANDATORY_NOT_PARALLEL if(nthreads>1 && !thread_pool_locked) crash("this cannot be called when threads are not locked")

#define IS_PARALLEL (NACTIVE_THREADS!=1)

#if defined BGQ && !defined BGQ_EMU
  #include <spi/include/kernel/location.h>
  //#define GET_THREAD_ID() uint32_t thread_id=Kernel_ProcessorID() //works only if 64 threads are used...
  #define GET_THREAD_ID() uint32_t thread_id=omp_get_thread_num()
 #else
  #define GET_THREAD_ID() uint32_t thread_id=omp_get_thread_num()
 #endif

#define THREAD_ID thread_id
 
#ifdef THREAD_DEBUG
 #define THREAD_BARRIER_FORCE() thread_barrier_internal()
 #define THREAD_BARRIER()       if(!thread_pool_locked) thread_barrier_with_check(__FILE__,__LINE__)
#else
 #define THREAD_BARRIER_FORCE() thread_barrier_internal()
 #define THREAD_BARRIER()       if(!thread_pool_locked) thread_barrier_without_check()
#endif
 
#define IS_MASTER_THREAD (THREAD_ID==0)

#define NISSA_PARALLEL_LOOP(INDEX,START,END)			\
  NISSA_CHUNK_LOOP(INDEX,START,END,thread_id,NACTIVE_THREADS){
#define NISSA_PARALLEL_LOOP_END }
  
#define THREAD_ATOMIC_EXEC(inst) do{THREAD_BARRIER();inst;THREAD_BARRIER();}while(0)
#define THREAD_BROADCAST(out,in)			\
  if(IS_MASTER_THREAD) broadcast_ptr=(void*)&in;		\
  THREAD_ATOMIC_EXEC(memcpy(&out,broadcast_ptr,sizeof(out)));
#define THREAD_BROADCAST_PTR(out,in)		\
  if(IS_MASTER_THREAD) broadcast_ptr=in;				\
  THREAD_ATOMIC_EXEC(memcpy(&out,&broadcast_ptr,sizeof(void*)));

//external argument to exchange info between function and worker
#define EXTERNAL_ARG(FUNC_NAME,LINE,ARG_TYPE,ARG) namespace{ARG_TYPE NAME4(FUNC_NAME,LINE,EXT_ARG,ARG);}
#define EXPORT_ARG(FUNC_NAME,LINE,ARG) NAME4(FUNC_NAME,LINE,EXT_ARG,ARG)=ARG;
#define IMPORT_ARG(FUNC_NAME,LINE,ARG_TYPE,ARG) ARG_TYPE ARG=NAME4(FUNC_NAME,LINE,EXT_ARG,ARG);

//headers: external parameters
#define THREADABLE_FUNCTION_0ARG_EXTERNAL_ARGS(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_1ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1)	\
  EXTERNAL_ARG(FUNC_NAME,LINE,AT1,A1)					\
  THREADABLE_FUNCTION_0ARG_EXTERNAL_ARGS(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_2ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2) \
  EXTERNAL_ARG(FUNC_NAME,LINE,AT2,A2)					\
  THREADABLE_FUNCTION_1ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1)
#define THREADABLE_FUNCTION_3ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3) \
  EXTERNAL_ARG(FUNC_NAME,LINE,AT3,A3)					\
  THREADABLE_FUNCTION_2ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2)
#define THREADABLE_FUNCTION_4ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  EXTERNAL_ARG(FUNC_NAME,LINE,AT4,A4)					\
  THREADABLE_FUNCTION_3ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3)
#define THREADABLE_FUNCTION_5ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  EXTERNAL_ARG(FUNC_NAME,LINE,AT5,A5)					\
  THREADABLE_FUNCTION_4ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4)
#define THREADABLE_FUNCTION_6ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  EXTERNAL_ARG(FUNC_NAME,LINE,AT6,A6)					\
  THREADABLE_FUNCTION_5ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5)
#define THREADABLE_FUNCTION_7ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  EXTERNAL_ARG(FUNC_NAME,LINE,AT7,A7)					\
  THREADABLE_FUNCTION_6ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6)
#define THREADABLE_FUNCTION_8ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  EXTERNAL_ARG(FUNC_NAME,LINE,AT8,A8)					\
  THREADABLE_FUNCTION_7ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7)
#define THREADABLE_FUNCTION_9ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  EXTERNAL_ARG(FUNC_NAME,LINE,AT9,A9)					\
  THREADABLE_FUNCTION_8ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8)
#define THREADABLE_FUNCTION_10ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  EXTERNAL_ARG(FUNC_NAME,LINE,AT10,A10)					\
  THREADABLE_FUNCTION_9ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9)
#define THREADABLE_FUNCTION_11ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11) \
  EXTERNAL_ARG(FUNC_NAME,LINE,AT11,A11)					\
  THREADABLE_FUNCTION_10ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10)

///////////////////////////////////////////////////////////////////////////////////////

//external function: exportation (last line is most external)
#define THREADABLE_FUNCTION_0ARG_EXPORT(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_1ARG_EXPORT(FUNC_NAME,LINE,AT1,A1)		\
  THREADABLE_FUNCTION_0ARG_EXPORT(FUNC_NAME,LINE)				\
  EXPORT_ARG(FUNC_NAME,LINE,A1)
#define THREADABLE_FUNCTION_2ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2)	\
  THREADABLE_FUNCTION_1ARG_EXPORT(FUNC_NAME,LINE,AT1,A1)			\
  EXPORT_ARG(FUNC_NAME,LINE,A2)
#define THREADABLE_FUNCTION_3ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3)	\
  THREADABLE_FUNCTION_2ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2)		\
  EXPORT_ARG(FUNC_NAME,LINE,A3)
#define THREADABLE_FUNCTION_4ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  THREADABLE_FUNCTION_3ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3)	\
  EXPORT_ARG(FUNC_NAME,LINE,A4)
#define THREADABLE_FUNCTION_5ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  THREADABLE_FUNCTION_4ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  EXPORT_ARG(FUNC_NAME,LINE,A5)
#define THREADABLE_FUNCTION_6ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  THREADABLE_FUNCTION_5ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  EXPORT_ARG(FUNC_NAME,LINE,A6)
#define THREADABLE_FUNCTION_7ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  THREADABLE_FUNCTION_6ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  EXPORT_ARG(FUNC_NAME,LINE,A7)
#define THREADABLE_FUNCTION_8ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  THREADABLE_FUNCTION_7ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  EXPORT_ARG(FUNC_NAME,LINE,A8)
#define THREADABLE_FUNCTION_9ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  THREADABLE_FUNCTION_8ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  EXPORT_ARG(FUNC_NAME,LINE,A9)
#define THREADABLE_FUNCTION_10ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  THREADABLE_FUNCTION_9ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  EXPORT_ARG(FUNC_NAME,LINE,A10)
#define THREADABLE_FUNCTION_11ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11) \
  THREADABLE_FUNCTION_10ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  EXPORT_ARG(FUNC_NAME,LINE,A11)

/////////////////////////////////////////////////////////////////////////////////////////

//body: issue worker and reimport (last line is again the most external, so this is why we have to split)
#define THREADABLE_FUNCTION_TRIGGER_PT1()				\
  if(nthreads>1 && thread_pool_locked)					\
    {
#define THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)		\
      start_threaded_function(NAME3(FUNC_NAME,LINE,SUMMONER),#FUNC_NAME);	\
    }									\
  else

#define THREADABLE_FUNCTION_0ARG_TRIGGER(FUNC_NAME,LINE)	\
  THREADABLE_FUNCTION_TRIGGER_PT1()				\
  THREADABLE_FUNCTION_0ARG_EXPORT(FUNC_NAME,LINE)			\
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_1ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1)	\
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_1ARG_EXPORT(FUNC_NAME,LINE,AT1,A1)			\
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_2ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2) \
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_2ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2)		\
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_3ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3) \
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_3ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3)	\
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_4ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_4ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_5ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_5ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_6ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_6ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_7ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_7ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_8ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_8ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_9ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_9ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_10ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_10ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)
#define THREADABLE_FUNCTION_11ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11) \
  THREADABLE_FUNCTION_TRIGGER_PT1()					\
  THREADABLE_FUNCTION_11ARG_EXPORT(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11) \
  THREADABLE_FUNCTION_TRIGGER_PT2(FUNC_NAME,LINE)

/////////////////////////////////////////// summoner ///////////////////////////////////////////////////

#define THREADABLE_FUNCTION_0ARG_SUMMONER(FUNC_NAME,LINE)	\
  void NAME3(FUNC_NAME,LINE,SUMMONER)()				\
  {								\
    FUNC_NAME();						\
  }
#define THREADABLE_FUNCTION_1ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1)	\
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    FUNC_NAME(A1);							\
  }
#define THREADABLE_FUNCTION_2ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2)	\
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT2,A2);					\
    FUNC_NAME(A1,A2);							\
  }
#define THREADABLE_FUNCTION_3ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3) \
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT2,A2);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT3,A3);					\
    FUNC_NAME(A1,A2,A3);							\
  }
#define THREADABLE_FUNCTION_4ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT2,A2);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT3,A3);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT4,A4);					\
    FUNC_NAME(A1,A2,A3,A4);						\
  }
#define THREADABLE_FUNCTION_5ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT2,A2);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT3,A3);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT4,A4);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT5,A5);					\
    FUNC_NAME(A1,A2,A3,A4,A5);						\
  }
#define THREADABLE_FUNCTION_6ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT2,A2);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT3,A3);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT4,A4);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT5,A5);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT6,A6);					\
    FUNC_NAME(A1,A2,A3,A4,A5,A6);					\
  }
#define THREADABLE_FUNCTION_7ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT2,A2);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT3,A3);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT4,A4);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT5,A5);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT6,A6);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT7,A7);					\
    FUNC_NAME(A1,A2,A3,A4,A5,A6,A7);					\
  }
#define THREADABLE_FUNCTION_8ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT2,A2);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT3,A3);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT4,A4);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT5,A5);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT6,A6);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT7,A7);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT8,A8);					\
    FUNC_NAME(A1,A2,A3,A4,A5,A6,A7,A8);					\
  }
#define THREADABLE_FUNCTION_9ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT2,A2);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT3,A3);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT4,A4);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT5,A5);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT6,A6);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT7,A7);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT8,A8);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT9,A9);					\
    FUNC_NAME(A1,A2,A3,A4,A5,A6,A7,A8,A9);				\
  }
#define THREADABLE_FUNCTION_10ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT2,A2);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT3,A3);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT4,A4);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT5,A5);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT6,A6);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT7,A7);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT8,A8);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT9,A9);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT10,A10);				\
    FUNC_NAME(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10);				\
  }
#define THREADABLE_FUNCTION_11ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11) \
  void NAME3(FUNC_NAME,LINE,SUMMONER)()					\
  {									\
    IMPORT_ARG(FUNC_NAME,LINE,AT1,A1);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT2,A2);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT3,A3);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT4,A4);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT5,A5);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT6,A6);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT7,A7);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT8,A8);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT9,A9);					\
    IMPORT_ARG(FUNC_NAME,LINE,AT10,A10);				\
    IMPORT_ARG(FUNC_NAME,LINE,AT11,A11);				\
    FUNC_NAME(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11);			\
  }

//////////////////////////////////////////////////////////////////////////////////////////

//threadable function with 0 arguments
#define THREADABLE_FUNCTION_0ARG_INSIDE(FUNC_NAME,LINE)			\
  THREADABLE_FUNCTION_0ARG_EXTERNAL_ARGS(FUNC_NAME,LINE)		\
  void FUNC_NAME();							\
  THREADABLE_FUNCTION_0ARG_SUMMONER(FUNC_NAME,LINE)			\
  void FUNC_NAME(){							\
    THREADABLE_FUNCTION_0ARG_TRIGGER(FUNC_NAME,LINE)

//threadable function with 1 arguments
#define THREADABLE_FUNCTION_1ARG_INSIDE(FUNC_NAME,LINE,AT1,A1)		\
  THREADABLE_FUNCTION_1ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1)		\
  void FUNC_NAME(AT1 A1);						\
  THREADABLE_FUNCTION_1ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1)		\
  void FUNC_NAME(AT1 A1){						\
    THREADABLE_FUNCTION_1ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1)

//threadable function with 2 arguments
#define THREADABLE_FUNCTION_2ARG_INSIDE(FUNC_NAME,LINE,AT1,A1,AT2,A2)	\
  THREADABLE_FUNCTION_2ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2)	\
  void FUNC_NAME(AT1 A1,AT2 A2);					\
  THREADABLE_FUNCTION_2ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2)	\
  void FUNC_NAME(AT1 A1,AT2 A2){					\
    THREADABLE_FUNCTION_2ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2)

//threadable function with 3 arguments
#define THREADABLE_FUNCTION_3ARG_INSIDE(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3) \
  THREADABLE_FUNCTION_3ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3);					\
  THREADABLE_FUNCTION_3ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3){					\
    THREADABLE_FUNCTION_3ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3)

//threadable function with 4 arguments
#define THREADABLE_FUNCTION_4ARG_INSIDE(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  THREADABLE_FUNCTION_4ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4);				\
  THREADABLE_FUNCTION_4ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4){				\
    THREADABLE_FUNCTION_4ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4)

//threadable function with 5 arguments
#define THREADABLE_FUNCTION_5ARG_INSIDE(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  THREADABLE_FUNCTION_5ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5);			\
  THREADABLE_FUNCTION_5ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5){			\
    THREADABLE_FUNCTION_5ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5)

//threadable function with 6 arguments
#define THREADABLE_FUNCTION_6ARG_INSIDE(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  THREADABLE_FUNCTION_6ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6);		\
  THREADABLE_FUNCTION_6ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6){		\
    THREADABLE_FUNCTION_6ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6)

//threadable function with 7 arguments
#define THREADABLE_FUNCTION_7ARG_INSIDE(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  THREADABLE_FUNCTION_7ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7);	\
  THREADABLE_FUNCTION_7ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7){	\
    THREADABLE_FUNCTION_7ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7)
  
  //threadable function with 8 arguments
#define THREADABLE_FUNCTION_8ARG_INSIDE(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  THREADABLE_FUNCTION_8ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8); \
  THREADABLE_FUNCTION_8ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8){ \
    THREADABLE_FUNCTION_8ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8)

//threadable function with 9 arguments
#define THREADABLE_FUNCTION_9ARG_INSIDE(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  THREADABLE_FUNCTION_9ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8,AT9 A9); \
  THREADABLE_FUNCTION_9ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8,AT9 A9){ \
    THREADABLE_FUNCTION_9ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9)

//threadable function with 10 arguments
#define THREADABLE_FUNCTION_10ARG_INSIDE(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  THREADABLE_FUNCTION_10ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8,AT9 A9,AT10 A10); \
  THREADABLE_FUNCTION_10ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8,AT9 A9,AT10 A10){ \
    THREADABLE_FUNCTION_10ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10)

//threadable function with 11 arguments
#define THREADABLE_FUNCTION_11ARG_INSIDE(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11) \
  THREADABLE_FUNCTION_11ARG_EXTERNAL_ARGS(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8,AT9 A9,AT10 A10,AT11 A11); \
  THREADABLE_FUNCTION_11ARG_SUMMONER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11) \
  void FUNC_NAME(AT1 A1,AT2 A2,AT3 A3,AT4 A4,AT5 A5,AT6 A6,AT7 A7,AT8 A8,AT9 A9,AT10 A10,AT11 A11){ \
    THREADABLE_FUNCTION_11ARG_TRIGGER(FUNC_NAME,LINE,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11)

///////////////////////////////////////////////////////////////////////////////////////////////////

#define THREADABLE_FUNCTION_0ARG(FUNC_NAME)		\
  THREADABLE_FUNCTION_0ARG_INSIDE(FUNC_NAME,__LINE__)
#define THREADABLE_FUNCTION_1ARG(FUNC_NAME,AT1,A1)		\
  THREADABLE_FUNCTION_1ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1)
#define THREADABLE_FUNCTION_2ARG(FUNC_NAME,AT1,A1,AT2,A2)		\
  THREADABLE_FUNCTION_2ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1,AT2,A2)
#define THREADABLE_FUNCTION_3ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3)	\
  THREADABLE_FUNCTION_3ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1,AT2,A2,AT3,A3)
#define THREADABLE_FUNCTION_4ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4)	\
  THREADABLE_FUNCTION_4ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1,AT2,A2,AT3,A3,AT4,A4)
#define THREADABLE_FUNCTION_5ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5) \
  THREADABLE_FUNCTION_5ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5)
#define THREADABLE_FUNCTION_6ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6) \
  THREADABLE_FUNCTION_6ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6)
#define THREADABLE_FUNCTION_7ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7) \
  THREADABLE_FUNCTION_7ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7)
#define THREADABLE_FUNCTION_8ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8) \
  THREADABLE_FUNCTION_8ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8)
#define THREADABLE_FUNCTION_9ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9) \
  THREADABLE_FUNCTION_9ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9)
#define THREADABLE_FUNCTION_10ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10) \
  THREADABLE_FUNCTION_10ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10)
#define THREADABLE_FUNCTION_11ARG(FUNC_NAME,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11) \
  THREADABLE_FUNCTION_11ARG_INSIDE(FUNC_NAME,__LINE__,AT1,A1,AT2,A2,AT3,A3,AT4,A4,AT5,A5,AT6,A6,AT7,A7,AT8,A8,AT9,A9,AT10,A10,AT11,A11)

namespace nissa
{
  //flush the cache
  inline void cache_flush()
  {
#if defined BGQ
 #if ! defined BGQ_EMU
    mbar();
 #else
    __sync_synchronize();
 #endif
#else
     #pragma omp flush
#endif
  }
}

#if defined BGQ && (! defined BGQ_EMU)
namespace nissa
{
  extern unsigned int nthreads;
}
#endif

//barrier without any possible checking
namespace nissa
{
  inline void thread_barrier_internal()
  {
#if defined BGQ && (! defined BGQ_EMU)
    bgq_barrier(nissa::nthreads);
#else
    #pragma omp barrier
#endif
  }
}

#define FORM_TWO_THREAD_TEAMS()						\
  bool is_in_first_team,is_in_second_team;				\
  unsigned int nthreads_in_team,thread_in_team_id;			\
  if(thread_pool_locked||nthreads==1)					\
    {									\
      is_in_first_team=is_in_second_team=true;				\
      nthreads_in_team=1;						\
      thread_in_team_id=0;						\
    }									\
  else									\
    {									\
      is_in_first_team=(thread_id<nthreads/2);				\
      is_in_second_team=!is_in_first_team;				\
      if(is_in_first_team)						\
	{								\
	  nthreads_in_team=nthreads/2;					\
	  thread_in_team_id=thread_id;					\
	}								\
      else								\
	{								\
	  nthreads_in_team=nthreads-nthreads/2;				\
	  thread_in_team_id=thread_id-nthreads/2;			\
	}								\
    }

#endif
