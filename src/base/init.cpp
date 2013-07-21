#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <mpi.h>
#include <signal.h>
#include <string.h>
#include <omp.h>

#include "../communicate/communicate.h"
#include "debug.h"
#include "global_variables.h"
#include "vectors.h"

#include "../IO/input.h"
#include "../IO/endianess.h"
#include "../geometry/geometry_eo.h"
#include "../geometry/geometry_lx.h"
#include "../geometry/geometry_Wsklx.h"
#ifdef USE_VNODES
 #include "../geometry/geometry_vir.h"
#endif
#include "../new_types/dirac.h"
 #include "../new_types/su3.h"
#include "../routines/ios.h"
#include "../routines/math.h"
#include "../routines/mpi.h"
#ifdef USE_THREADS
 #include "../routines/thread.h"
#endif

#ifdef SPI
 #include "../bgq/spi.h"
#endif

//init nissa
void init_nissa(int narg,char **arg)
{
  //init base things
  int provided;
  MPI_Init_thread(&narg,&arg,MPI_THREAD_SERIALIZED,&provided);
  tot_nissa_time=-take_time();
#ifdef COMM_BENCH
  tot_nissa_comm_time=0;
#endif
  verb_call=0;
  
  //get the number of rank and the id of the local one
  MPI_Comm_size(MPI_COMM_WORLD,&nissa_nranks);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  
  //associate sigsegv with proper handle
  signal(SIGSEGV,terminate_sigsegv);
  signal(SIGFPE,terminate_sigsegv);
  
  //print SVN version and configuration and compilation time
  master_printf("Initializing nissa, version: %s\n",SVN_REVISION);
  master_printf("Configured at %s with flags: %s\n",CONFIG_TIME,CONFIG_FLAGS);
  master_printf("Compiled at %s of %s\n",__TIME__,__DATE__);
  
  //128 bit float
  MPI_Type_contiguous(2,MPI_DOUBLE,&MPI_FLOAT_128);
  MPI_Type_commit(&MPI_FLOAT_128);
  
  //define the gauge link
  MPI_Type_contiguous(18,MPI_DOUBLE,&MPI_SU3);
  MPI_Type_commit(&MPI_SU3);
  
  //four links starting from a single point
  MPI_Type_contiguous(4,MPI_SU3,&MPI_QUAD_SU3);
  MPI_Type_commit(&MPI_QUAD_SU3);
  
  //a color (6 doubles)
  MPI_Type_contiguous(6,MPI_DOUBLE,&MPI_COLOR);
  MPI_Type_commit(&MPI_COLOR);
  
  //a spin (8 doubles)
  MPI_Type_contiguous(8,MPI_DOUBLE,&MPI_SPIN);
  MPI_Type_commit(&MPI_SPIN);

  //a spinspin (32 doubles)
  MPI_Type_contiguous(32,MPI_DOUBLE,&MPI_SPINSPIN);
  MPI_Type_commit(&MPI_SPINSPIN);
  
  //a spincolor (24 doubles)
  MPI_Type_contiguous(24,MPI_DOUBLE,&MPI_SPINCOLOR);
  MPI_Type_commit(&MPI_SPINCOLOR);
  
  //a spincolor_128 (24 float_128)
  MPI_Type_contiguous(24,MPI_FLOAT_128,&MPI_SPINCOLOR_128);
  MPI_Type_commit(&MPI_SPINCOLOR_128);
  
  //a reduced spincolor (12 doubles)
  MPI_Type_contiguous(12,MPI_DOUBLE,&MPI_REDSPINCOLOR);
  MPI_Type_commit(&MPI_REDSPINCOLOR);
  
  //summ for 128 bit float
  MPI_Op_create((MPI_User_function*)MPI_FLOAT_128_SUM_routine,1,&MPI_FLOAT_128_SUM);
  
  //initialize the first vector of nissa
  initialize_main_nissa_vect();
  
  //initialize global variables
  nissa_lx_geom_inited=0;
  nissa_Wsklx_order_inited=0;
  nissa_eo_geom_inited=0;
  nissa_loc_rnd_gen_inited=0;
  nissa_glb_rnd_gen_inited=0;
  nissa_grid_inited=0;
#ifdef SPI
  nissa_spi_inited=0;
#endif
  memset(rank_coord,0,4*sizeof(int));
  memset(nrank_dir,0,4*sizeof(int));
  ONE[0]=I[1]=1;
  ONE[1]=I[0]=0;
  //check endianess
  check_endianess();
  if(little_endian) master_printf("System endianess: little (ordinary machine)\n");
  else master_printf("System endianess: big (BG, etc)\n");

  //set default value for parameters
  nissa_verbosity=nissa_default_verbosity;
  nissa_use_128_bit_precision=nissa_default_use_128_bit_precision;
  nissa_use_eo_geom=nissa_default_use_eo_geom;
  nissa_warn_if_not_disallocated=nissa_default_warn_if_not_disallocated;
  nissa_warn_if_not_communicated=nissa_default_warn_if_not_communicated;
  nissa_use_async_communications=nissa_default_use_async_communications;
  for(int mu=0;mu<4;mu++) nissa_set_nranks[mu]=0;
  nissa_vnode_paral_dir=nissa_default_vnode_paral_dir;
  
  //read the configuration file, if present
  read_nissa_config_file();
  
  //initialize the base of the gamma matrices
  init_base_gamma();
  
  master_printf("Nissa initialized!\n");
}

//start nissa in a threaded environment, sending all threads but first in the 
//thread pool and issuing the main function
void init_nissa_threaded(int narg,char **arg,void(*main_function)(int narg,char **arg))
{
#ifdef USE_THREADS

  //if BGQ, define appropriate barrier
#if defined BGQ && (! defined BGQ_EMU)
  bgq_barrier_define();
#endif

#pragma omp parallel
  {
    //initialize nissa (master thread only)
#pragma omp master
    init_nissa(narg,arg);
#pragma omp barrier
  
    //get the number of threads and thread id
    nthreads=omp_get_num_threads();
    master_printf("Using %d threads\n",nthreads);
    
    //distinguish master thread from the others
    GET_THREAD_ID();
    if(thread_id!=0) thread_pool();
    else thread_master_start(narg,arg,main_function);
  }
#else
  init_nissa(narg,arg);
  main_function(narg,arg);
#endif
}

//compute internal volume
int bulk_volume(int *L)
{
  int intvol=1,mu=0;
  do
    {
      if(L[mu]>2) intvol*=L[mu]-2;
      else intvol=0;
      
      mu++;
    }
  while(intvol!=0 && mu<4);
  
  return intvol;
}

//compute the bulk volume of the local lattice, given by L/R
int bulk_recip_lat_volume(int *R,int *L)
{
  int X[4]={L[0]/R[0],L[1]/R[1],L[2]/R[2],L[3]/R[3]};
  return bulk_volume(X);
}

//compute the variance of the border
int compute_border_variance(int *L,int *P,int factorize_processor)
{
  int S2B=0,SB=0;
  for(int ib=0;ib<4;ib++)
    {
      int B=1;
      for(int mu=0;mu<4;mu++) if(mu!=ib) B*=(factorize_processor) ? L[mu]/P[mu] : P[mu];
      SB+=B;
      S2B+=B*B;
    }
  SB/=4;
  S2B/=4;
  S2B-=SB*SB;
  
  return S2B;
}

//find the grid minimizing the surface
void find_minimal_surface_grid(int *mR,int *ext_L,int NR)
{
  int additionally_parallelize_dir[4]={0,0,0,0};
#ifdef SPI
  //switched off: conflict with E/O
  //additionally_parallelize_dir[0]=1;
#endif

  //if we want to repartition one dir we must take this into account
  int L[4];
  for(int mu=0;mu<4;mu++) L[mu]=additionally_parallelize_dir[mu]?ext_L[mu]/2:ext_L[mu];

  //compute total and local volume
  int V=L[0]*L[1]*L[2]*L[3];
  int LV=V/NR;
      
  int something_found=1;
  
  ////////////////////////////// set all the bounds ///////////////////////////////////
  
  int check_all_dir_parallelized=0;
  
  /////////////////////////////////// basic checks ///////////////////////////////////
  
  //check that all direction are parallelizable, if requested
  if(check_all_dir_parallelized)
    {
      //check that at least 16 ranks are present and is a multiple of 16
      if(NR<16) crash("in order to paralellize all the direcion, at least 16 ranks must be present");
      if(NR%16) crash("in order to paralellize all the direcion, the number of ranks must be a multiple of 16");
    }
  
  //check that all directions can bemade even, if requested
  if(nissa_use_eo_geom) if((V/NR)%16!=0) crash("in order to use eo geometry, local size must be a multiple of 16");
    
  //check that the global lattice is a multiple of the number of ranks
  if(V%NR) crash("global volume must be a multiple of ranks number");
  
  //check that we did not asked to fix in an impossible way
  int res_NR=NR;
  for(int mu=0;mu<4;mu++)
    {
      int nmin_dir=1;
      if(nissa_use_eo_geom) nmin_dir*=2;
      if(additionally_parallelize_dir[mu]) nmin_dir*=2;
      
      if(nissa_set_nranks[mu])
	{
	  if(L[mu]%nissa_set_nranks[mu]||L[mu]<nmin_dir)
	    crash("asked to fix dir % in an impossible way",mu);
	  res_NR/=nissa_set_nranks[mu];
	}
    }
  if(res_NR<1) crash("overfixed the ranks per direction");
  
  //////////////////// find the partitioning which minmize the surface /////////////////////
  
  //treat simple cases
  if(NR==1||NR==V)
    {
      if(NR==1) mR[0]=mR[1]=mR[2]=mR[3]=1;
      else for(int mu=0;mu<4;mu++) mR[mu]=L[mu];
    }
  else
    {
      //minimal variance border
      int mBV=-1;
      
      //factorize the local volume
      int list_fact_LV[log2N(LV)];
      int nfact_LV=factorize(list_fact_LV,LV);
      
      //factorize the number of rank
      int list_fact_NR[log2N(NR)];
      int nfact_NR=factorize(list_fact_NR,NR);
      
      //if nfact_LV>=nfact_NR factorize the number of rank, otherwise the local volume
      //in the first case we find the best way to assign the ranks to different directions
      //in the second case we find how many sites per direction to assign to each rank
      int factorize_rank=(nfact_LV>=nfact_NR);
      int nfact=factorize_rank ? nfact_NR : nfact_LV;
      int *list_fact=factorize_rank ? list_fact_NR : list_fact_LV;
      
      //compute the number of combinations: this is given by 4^nfact
      int ncombo=1;
      for(int ifact=0;ifact<nfact;ifact++) ncombo*=4;
      
      //find the partition which minimize the surface and the surface variance
      int min_surf_LV=-1;
      int icombo=0;
      mR[0]=mR[1]=mR[2]=mR[3]=-1;
      
      do
	{
	  //number of ranks in each direction for current partitioning
	  int R[4]={1,1,1,1};
	  
	  //find the partioning corresponding to icombo
	  int ifact=nfact-1;
	  int valid_partitioning=1;
	  do
	    {
	      //find the direction: this is given by the ifact digit of icombo wrote in base 4
	      int mu=(icombo>>(2*ifact)) & 0x3;
	      
	      //if we are factorizing local lattice, rank factor is given by list_fact, otherwise L/list_fact
	      R[mu]*=list_fact[ifact];
	      
	      //check that the total volume L is a multiple and it is larger than the number of proc
	      valid_partitioning=(L[mu]%R[mu]==0 && L[mu]>=R[mu]);
	      if(valid_partitioning) ifact--;
	    }
	  while(valid_partitioning && ifact>=0);
	  
	  if(valid_partitioning)
	    for(int mu=0;mu<4;mu++)
	      {
		//if we are factorizing reciprocal lattice, convert back to rank grid
		if(!factorize_rank)  R[mu]=L[mu]/R[mu];
		//check that all directions have at least 2 nodes
		if(check_all_dir_parallelized) valid_partitioning&=(R[mu]>=2);
		//check that lattice size is even in all directions
		if(nissa_use_eo_geom) valid_partitioning&=((L[mu]/R[mu])%2==0);
		//check that we match the possibly fixed dir
		if(nissa_set_nranks[mu]) valid_partitioning&=(nissa_set_nranks[mu]==R[mu]);
	      }
	  
	  //validity coulde have changed
          if(valid_partitioning)
	    {
	      //compute the surface=loc_vol-bulk_volume
	      int BV=bulk_recip_lat_volume(R,L);
	      int surf_LV=LV-BV;
	      
	      //look if this is the new minimal surface
	      int new_minimal=0;
	      //if it is the minimal surface (or first valid combo) copy it and compute the border size
	      if(surf_LV<min_surf_LV||min_surf_LV==-1)
		{
		  new_minimal=1;
		  mBV=compute_border_variance(L,R,factorize_rank);
		}
	      //if it is equal to previous found surface, consider borders variance
	      if(surf_LV==min_surf_LV)
		{
		  int BV=compute_border_variance(L,R,factorize_rank);
		  //if borders are more homogeneus consider this grid
		  if(BV<mBV)
		    {
		      mBV=BV;
		      new_minimal=1;
		    }
		}
	      
	      //save it as new minimal
	      if(new_minimal)
		{
		  min_surf_LV=surf_LV;
		  for(int mu=0;mu<4;mu++) mR[mu]=R[mu];
		  something_found=1;
		}
	      
	      icombo++;
	    }
	  //skip all remaining factorization using the same structure
	  else icombo+=(ifact>1) ? 1<<(2*(ifact-1)) : 1;
	}
      while(icombo<ncombo);
    }
  
  if(!something_found) crash("no valid partitioning found");
}

//initialize MPI grid
//if you need non-homogeneus glb_size[i] pass L=T=0 and
//set glb_size before calling the routine
void init_grid(int T,int L)
{
  //take initial time
  double time_init=-take_time();
  master_printf("\nInitializing grid, geometry and communications\n");
  
  if(nissa_grid_inited==1) crash("grid already intialized!");
  nissa_grid_inited=1;
  
  //set the volume
  if(T!=0 && L!=0)
    {
      glb_size[0]=T;
      glb_size[3]=glb_size[2]=glb_size[1]=L;
    }
  
  //broadcast the global sizes
  MPI_Bcast(glb_size,4,MPI_INT,0,MPI_COMM_WORLD);
  
  //calculate global volume, initialize local one
  glb_vol=1;
  for(int idir=0;idir<4;idir++)
    {
      loc_size[idir]=glb_size[idir];
      glb_vol*=glb_size[idir];
    }
  glb_spat_vol=glb_vol/glb_size[0];
  glb_vol2=(double)glb_vol*glb_vol;
  
  master_printf("Number of running ranks: %d\n",nissa_nranks);
  master_printf("Global lattice:\t%dx%dx%dx%d = %d\n",glb_size[0],glb_size[1],glb_size[2],glb_size[3],glb_vol);
  
  //find the grid minimizing the surface
  find_minimal_surface_grid(nrank_dir,glb_size,nissa_nranks);
  //check that lattice is commensurable with the grid
  //and check wether the idir dir is parallelized or not
  int ok=(glb_vol%nissa_nranks==0);
  if(!ok) crash("The lattice is incommensurable with the total ranks amount!");
  for(int idir=0;idir<4;idir++)
    {
      ok=ok && (nrank_dir[idir]>0);
      if(!ok) crash("dir nranks[%d]: %d",idir,nrank_dir[idir]);
      ok=ok && (glb_size[idir]%nrank_dir[idir]==0);
      if(!ok) crash("glb_size[%d]%nrank_dir[%d]=%d",idir,idir,glb_size[idir]%nrank_dir[idir]);
      paral_dir[idir]=(nrank_dir[idir]>1);
      nparal_dir+=paral_dir[idir];
    }

  master_printf("Creating grid:\t%dx%dx%dx%d\n",nrank_dir[0],nrank_dir[1],nrank_dir[2],nrank_dir[3]);  
  
  //creates the grid
  int periods[4]={1,1,1,1};
  MPI_Cart_create(MPI_COMM_WORLD,4,nrank_dir,periods,1,&cart_comm);
  //takes rank and ccord of local rank
  MPI_Comm_rank(cart_comm,&cart_rank);
  MPI_Cart_coords(cart_comm,cart_rank,4,rank_coord);
  
  //calculate the local volume
  for(int idir=0;idir<4;idir++) loc_size[idir]=glb_size[idir]/nrank_dir[idir];
  loc_vol=glb_vol/nissa_nranks;
  loc_spat_vol=loc_vol/loc_size[0];
  loc_vol2=(double)loc_vol*loc_vol;
  
  //calculate bulk size
  bulk_vol=non_bw_surf_vol=1;
  for(int idir=0;idir<4;idir++)
    if(paral_dir[idir])
      {
	bulk_vol*=loc_size[idir]-2;
	non_bw_surf_vol*=loc_size[idir]-1;
      }
    else
      {
	bulk_vol*=loc_size[idir];
	non_bw_surf_vol*=loc_size[idir];
      }
  non_fw_surf_vol=non_bw_surf_vol;
  fw_surf_vol=bw_surf_vol=loc_vol-non_bw_surf_vol;
  surf_vol=loc_vol-bulk_vol;
  
  //calculate the border size
  bord_vol=0;
  bord_offset[0]=0;
  for(int idir=0;idir<4;idir++)
    {
      //bord size along the idir dir
      if(paral_dir[idir]) bord_dir_vol[idir]=loc_vol/loc_size[idir];
      else bord_dir_vol[idir]=0;
      
      //total bord
      bord_vol+=bord_dir_vol[idir];
      
      //summ of the border extent up to dir idir
      if(idir>0) bord_offset[idir]=bord_offset[idir-1]+bord_dir_vol[idir-1];
    }
  bord_vol*=2;  
  
#ifdef USE_VNODES
  //two times the size of nissa_vnode_paral_dir face
  vbord_vol=2*bord_vol/loc_size[nissa_vnode_paral_dir]; //so is not counting all vsites
  //compute the offset between sites of different vnodes
  //this amount to the product of the local size of the direction running faster than
  //nissa_vnode_paral_dir, times half the local size along nissa_vnode_paral_dir
  vnode_lx_offset=loc_size[nissa_vnode_paral_dir]/nvnodes;
  for(int mu=nissa_vnode_paral_dir+1;mu<4;mu++) vnode_lx_offset*=loc_size[mu];
#endif
  
  //calculate the egdes size
  edge_vol=0;
  edge_offset[0]=0;
  int iedge=0;
  for(int idir=0;idir<4;idir++)
    for(int jdir=idir+1;jdir<4;jdir++)
      {
	//edge among the i and j dir
	if(paral_dir[idir] && paral_dir[jdir]) edge_dir_vol[iedge]=bord_dir_vol[idir]/loc_size[jdir];
	else edge_dir_vol[iedge]=0;
	
	//total edge
	edge_vol+=edge_dir_vol[iedge];
	
	//summ of the border extent up to dir i
	if(iedge>0)
	  edge_offset[iedge]=edge_offset[iedge-1]+edge_dir_vol[iedge-1];
	iedge++;
    }
  edge_vol*=4;
  
  //print information
  master_printf("Local volume\t%dx%dx%dx%d = %d\n",loc_size[0],loc_size[1],loc_size[2],loc_size[3],loc_vol);
  master_printf("Parallelized dirs: t=%d x=%d y=%d z=%d\n",paral_dir[0],paral_dir[1],paral_dir[2],paral_dir[3]);
  master_printf("Border size: %d\n",bord_vol);
  master_printf("Edge size: %d\n",edge_vol);
  for(int idir=0;idir<4;idir++)
    verbosity_lv3_master_printf("Border offset for dir %d: %d\n",idir,bord_offset[idir]);
  for(iedge=0;iedge<6;iedge++)
    verbosity_lv3_master_printf("Border offset for edge %d: %d\n",iedge,edge_offset[iedge]);
  
  //print orderd list of the rank names
  if(nissa_verbosity>=3)
    {
      char proc_name[1024];
      int proc_name_length;
      MPI_Get_processor_name(proc_name,&proc_name_length);
      
      for(int irank=0;irank<nissa_nranks;irank++)
	{
	  if(rank==irank) printf("Rank %d of %d running on processor %s: %d (%d %d %d %d)\n",rank,nissa_nranks,
				 proc_name,cart_rank,rank_coord[0],rank_coord[1],rank_coord[2],rank_coord[3]);
	  fflush(stdout);
	  MPI_Barrier(MPI_COMM_WORLD);
	}
    }
  
  //create communicator along plan
  for(int mu=0;mu<4;mu++)
    {
      int split_plan[4];
      coords proj_rank_coord;
      for(int nu=0;nu<4;nu++)
	{
	  split_plan[nu]=(nu==mu) ? 0 : 1;
	  proj_rank_coord[nu]=(nu==mu) ? 0 : rank_coord[nu];
	}
      MPI_Cart_sub(cart_comm,split_plan,&(plan_comm[mu]));
      MPI_Comm_rank(plan_comm[mu],&(plan_rank[mu]));
      if(plan_rank[mu]!=rank_of_coord(proj_rank_coord))
	crash("Plan communicator has messed up coord: %d and rank %d (implement reorder!)",rank_of_coord(proj_rank_coord),plan_rank[mu]);
   }
  
  //create communicator along line
  for(int mu=0;mu<4;mu++)
    {
      //split the communicator
      int split_line[4];
      memset(split_line,0,4*sizeof(int));
      split_line[mu]=1;
      MPI_Cart_sub(cart_comm,split_line,&(line_comm[mu]));
      
      //get rank id
      MPI_Comm_rank(line_comm[mu],&(line_rank[mu]));
      
      //get rank coord along line comm
      MPI_Cart_coords(line_comm[mu],line_rank[mu],1,&(line_coord_rank[mu]));
      
      //check communicator
      if(line_rank[mu]!=rank_coord[mu] || line_rank[mu]!=line_coord_rank[mu])
	crash("Line communicator has messed up coord and rank (implement reorder!)");
   }
  
  //////////////////////////////////////////////////////////////////////////////////////////
  
  //set the cartesian and eo geometry
  set_lx_geometry();
  set_Wsklx_order(); //sink-based
  
  if(nissa_use_eo_geom) set_eo_geometry();
  
#ifdef USE_VNODES
  set_vir_geometry();
#endif

  ///////////////////////////////////// start communicators /////////////////////////////////
  
  ncomm_allocated=0;
  
#ifdef SPI
  init_spi();
#endif
    
  //setup all lx borders communicators
  set_lx_comm(lx_su3_comm,sizeof(su3));
  set_lx_comm(lx_quad_su3_comm,sizeof(quad_su3));
  set_lx_comm(lx_spin_comm,sizeof(spin));
  set_lx_comm(lx_color_comm,sizeof(color));
  set_lx_comm(lx_spinspin_comm,sizeof(spinspin));
  set_lx_comm(lx_spincolor_comm,sizeof(spincolor));
  set_lx_comm(lx_spincolor_128_comm,sizeof(spincolor_128));
  set_lx_comm(lx_halfspincolor_comm,sizeof(halfspincolor));
  set_lx_comm(lx_colorspinspin_comm,sizeof(colorspinspin));
  set_lx_comm(lx_su3spinspin_comm,sizeof(su3spinspin));

  //setup all lx edges communicators
  set_lx_edge_senders_and_receivers(MPI_LX_SU3_EDGES_SEND,MPI_LX_SU3_EDGES_RECE,&MPI_SU3);
  set_lx_edge_senders_and_receivers(MPI_LX_QUAD_SU3_EDGES_SEND,MPI_LX_QUAD_SU3_EDGES_RECE,&MPI_QUAD_SU3);
  
  if(nissa_use_eo_geom)
    {
      set_eo_comm(eo_spin_comm,sizeof(spin));
      set_eo_comm(eo_spincolor_comm,sizeof(spincolor));
      set_eo_comm(eo_spincolor_128_comm,sizeof(spincolor_128));
      set_eo_comm(eo_color_comm,sizeof(color));
      set_eo_comm(eo_quad_su3_comm,sizeof(quad_su3));
      set_eo_comm(eo_su3_comm,sizeof(su3));
      
      set_eo_edge_senders_and_receivers(MPI_EO_QUAD_SU3_EDGES_SEND,MPI_EO_QUAD_SU3_EDGES_RECE,&MPI_QUAD_SU3);
    }
  
  /*
  //check that everything is fine
  quad_su3 *testing=nissa_malloc("testing",loc_vol+bord_vol+edge_vol,quad_su3);
  for(int ivol=0;ivol<loc_vol;ivol++)
    for(int mu=0;mu<4;mu++)
      su3_put_to_diag(testing[ivol][mu],glblx_of_loclx[ivol]);
  communicate_lx_quad_su3_borders(testing);
  for(int ivol=loc_vol;ivol<loc_vol+bord_vol;ivol++)
    for(int mu=0;mu<4;mu++)
      {
	int expe=glblx_of_bordlx[ivol-loc_vol];
	int obte=(int)testing[ivol][mu][0][0][0];
	if(expe!=obte) crash("expecting %d, obtained %d on rank %d",expe,obte,rank);
      }
  nissa_free(testing);
  */
  //take final time
  master_printf("Time elapsed for MPI inizialization: %f s\n",time_init+take_time());
}
