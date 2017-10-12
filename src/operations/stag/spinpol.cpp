#ifdef HAVE_CONFIG_H
 #include "config.hpp"
#endif

#include "base/thread_macros.hpp"
#include "geometry/geometry_eo.hpp"
#include "geometry/geometry_mix.hpp"
#include "hmc/theory_pars.hpp"
#include "linalgs/linalgs.hpp"
#include "new_types/complex.hpp"
#include "operations/gaugeconf.hpp"
#include "operations/su3_paths/plaquette.hpp"
#include "operations/su3_paths/topological_charge.hpp"
#include "routines/ios.hpp"
#include "routines/mpi_routines.hpp"
#include "operations/smearing/recursive_Wflower.hpp"
#include "spinpol.hpp"

#ifdef USE_THREADS
 #include "routines/thread.hpp"
#endif

namespace nissa
{
  using namespace stag;
  typedef std::vector<std::pair<int,int> > op_list_t;
  
  namespace
  {
    const int nphieta=2,iphi=0,ieta=1;
    int ncopies,nflavs,nhits,nmeas,nops;
    int ind_copy_flav_meas_hit(int icopy,int iflav,int ihit,int imeas){return imeas+nmeas*(ihit+nhits*(iflav+nflavs*icopy));}
    int ind_copy_flav_meas_hit_op(int icopy,int iflav,int ihit,int imeas,int iop){return iop+nops*(imeas+nmeas*(ihit+nhits*(iflav+nflavs*icopy)));}
    int ind_copy_flav_hit_phieta(int icopy,int iflav,int ihit,int iphieta){return iphieta+nphieta*(ihit+nhits*(iflav+nflavs*icopy));}
    int ind_op_flav(int iop,int iflav){return iop+nops*iflav;}
  }
  
  //make the complex-double product
  void compute_tens_dens_topo_correlation(complex *spinpol_dens,complex *tens_dens,double *topo_dens)
  {
    GET_THREAD_ID();
    NISSA_PARALLEL_LOOP(ivol,0,loc_vol) complex_prod_double(spinpol_dens[ivol],tens_dens[ivol],topo_dens[ivol]);
    THREAD_BARRIER();
  }
  
  //compute the spin-polarization for all flavors
  THREADABLE_FUNCTION_5ARG(measure_spinpol, theory_pars_t*,tp, spinpol_meas_pars_t*,mp,int,iconf, int,conf_created, quad_su3**,glu_conf)
  {
    GET_THREAD_ID();
    
    verbosity_lv1_master_printf("Evaluating spinpol\n");
    
    //set-up the smoother
    smooth_pars_t &sp=mp->smooth_pars;
    if(sp.method!=smooth_pars_t::WFLOW) crash("spinpol makes sense only with Wflow");
    Wflow_pars_t &Wf=sp.Wflow;
    int nflows=Wf.nflows;
    double dt=Wf.dt;
    int meas_each=sp.meas_each_nsmooth;
    
    //take number of flavors and operators
    nflavs=tp->nflavs();
    nops=mp->nops();
    
    //open the file
    FILE *fout=open_file(mp->path,conf_created?"w":"a");
    
    //allocate point and local results
    double *topo_dens=nissa_malloc("topo_dens",loc_vol,double);
    //operator applied to a field
    color *chiop[2]={nissa_malloc("chiop_EVN",loc_volh+bord_volh,color),nissa_malloc("chiop_ODD",loc_volh+bord_volh,color)};
    //temporary vectors
    color *temp[2][2];
    for(int itemp=0;itemp<2;itemp++)
      for(int eo=0;eo<2;eo++)
	temp[itemp][eo]=nissa_malloc("temp",loc_volh+bord_volh,color);
    color *temp_eta[2]={nissa_malloc("eta_EVN",loc_volh+bord_volh,color),nissa_malloc("eta_ODD",loc_volh+bord_volh,color)};
    color *temp_phi[2]={nissa_malloc("phi_EVN",loc_volh+bord_volh,color),nissa_malloc("phi_ODD",loc_volh+bord_volh,color)};
    
    //Create the mask
    int mask[nops],shift[nops];
    for(int iop=0;iop<nops;iop++)
      {
	int spin=mp->operators[iop].first;
	int taste=mp->operators[iop].second;
	shift[iop]=(spin^taste);
	mask[iop]=form_stag_op_pattern(spin,taste);
	verbosity_lv3_master_printf(" iop %d (%d %d),\tmask: %d,\tshift: %d\n",iop,spin,taste,mask[iop],shift[iop]);
      }
    
    //allocate the smoothed conf
    quad_su3 *smoothed_conf=nissa_malloc("smoothed_conf",loc_vol+bord_vol,quad_su3);
    paste_eo_parts_into_lx_vector(smoothed_conf,glu_conf);
    //allocate the fermion (possibly stouted) conf
    quad_su3 *ferm_conf[2];
    for(int eo=0;eo<2;eo++) ferm_conf[eo]=nissa_malloc("ferm_conf",loc_volh+bord_volh+edge_volh,quad_su3);
    
    //allocate sources, to be flown
    ncopies=mp->ncopies;
    nmeas=mp->smooth_pars.nmeas_nonzero()+1;
    nhits=mp->nhits;
    
    if(mp->use_adjoint_flow)
      {
	complex tens[nflavs*nops];
	//tens density
	complex *tens_dens[nflavs*nops];
	for(int iflav_op=0;iflav_op<nflavs*nops;iflav_op++) tens_dens[iflav_op]=nissa_malloc("tens_dens",loc_vol+bord_vol,complex);
	//spinpol
	complex *spinpol_dens[nflavs*nops],spinpol[nflavs*nops];
	for(int iflav_op=0;iflav_op<nflavs*nops;iflav_op++) spinpol_dens[iflav_op]=nissa_malloc("spinpol_dens",loc_vol,complex);
	
	int ntot_sources=ind_copy_flav_meas_hit(ncopies-1,nflavs-1,nhits-1,nmeas-1)+1;
	color *source[ntot_sources];
	for(int is=0;is<ntot_sources;is++) source[is]=nissa_malloc("source",loc_vol+bord_vol,color);
	int ntot_sources_op=ind_copy_flav_meas_hit(ncopies-1,nflavs-1,nhits-1,nmeas-1)+1;
	color *source_op[ntot_sources_op];
	for(int is=0;is<ntot_sources_op;is++) source_op[is]=nissa_malloc("source_op",loc_vol+bord_vol,color);
	
	//fill all the sources, putting for all measures the same hit
	for(int imeas=0;imeas<nmeas;imeas++)
	  {
	    for(int icopy=0;icopy<ncopies;icopy++)
	      for(int ihit=0;ihit<nhits;ihit++)
		for(int iflav=0;iflav<nflavs;iflav++)
		  {
		    color *s =source[ind_copy_flav_meas_hit(icopy,iflav,ihit,imeas)];
		    color *s0=source[ind_copy_flav_meas_hit(icopy,0/*flav*/,ihit,0 /*imeas*/)];
		    if(iflav==0 and imeas==0) generate_fully_undiluted_lx_source(s0,RND_Z4,-1);
		    else vector_copy(s,s0);
		    
		    //prepare the fermionic conf
		    split_lx_vector_into_eo_parts(ferm_conf,smoothed_conf);
		    if(tp->stout_pars.nlevels)
		      stout_smear(ferm_conf,ferm_conf,&tp->stout_pars);
		    
		    for(int iop=0;iop<nops;iop++)
		      {
			//temp_eta=eta^\dag
			split_lx_vector_into_eo_parts(temp_eta,source[ind_copy_flav_meas_hit(icopy,iflav,ihit,imeas)]);
			for(int eo=0;eo<2;eo++) complex_vector_self_conj((complex*)(temp_eta[eo]),loc_volh*sizeof(color)/sizeof(complex));
			//temp_phi=op \eta^\dag
			apply_op(temp_phi,temp[0],temp[1],ferm_conf,tp->backfield[iflav],shift[iop],temp_eta);
			put_stag_phases(temp_phi,mask[iop]);
			//phi=(op \eta^\dag)^\dag
			for(int eo=0;eo<2;eo++) complex_vector_self_conj((complex*)(temp_phi[eo]),loc_volh*sizeof(color)/sizeof(complex));
			paste_eo_parts_into_lx_vector(source_op[ind_copy_flav_meas_hit_op(icopy,iflav,ihit,imeas,iop)],temp_phi);
		      }
		  }
	    for(int iflow=0;iflow<meas_each;iflow++) Wflow_lx_conf(smoothed_conf,dt);
	  }
	
	//the reecursive flower, need to cache backward integration
	paste_eo_parts_into_lx_vector(smoothed_conf,glu_conf);
	recursive_Wflower_t recu(Wf,smoothed_conf);
	//the adjoint flower needed for fermionic source
	fermion_adjoint_flower_t<> adj_ferm_flower(dt,all_dirs,true);
	
#if 0
	//test
	fermion_flower_t ferm_flower(dt,all_dirs,true);
	color *ori_0=source[1];
	generate_fully_undiluted_lx_source(ori_0,RND_Z4,-1);
	color *ori_t=source[3];
	generate_fully_undiluted_lx_source(ori_t,RND_Z4,-1);
	color *evo_0_to_t=source[0];
	vector_copy(evo_0_to_t,ori_0);
	for(int iflow=1;iflow<=nflows;iflow++)
	  {
	    //update conf to iflow
	    double t=dt*(iflow-1);
	    recu.update(iflow-1);
	    //verbosity_lv2_
	    master_printf(" flow forward to %d/%d, t %lg, plaquette: %.16lg\n",iflow,nflows,t,global_plaquette_lx_conf(smoothed_conf));
	    
	    //make the flower generate the intermediate step between iflow-1 and iflow
	    ferm_flower.generate_intermediate_steps(smoothed_conf);
	    
	    ferm_flower.add_or_rem_backfield_to_confs(0,tp->backfield[0]);
	    ferm_flower.flow_fermion(evo_0_to_t);
	    ferm_flower.add_or_rem_backfield_to_confs(1,tp->backfield[0]);
	    
	    // master_printf("t %lg, entry %lg, norm2 %lg\n",t,source[0][0][0][0],double_vector_glb_norm2(source[0],loc_vol));
	  }
	
	color *evo_t_to_0=source[2];
	vector_copy(evo_t_to_0,ori_t);
	//at each step it goes from iflow+1 to iflow
	for(int iflow=nflows-1;iflow>=0;iflow--)
	  {
	    //update conf to iflow
	    double t=dt*iflow;
	    recu.update(iflow);
	    //verbosity_lv2_
	    master_printf(" flow back to %d/%d, t %lg, plaquette: %.16lg\n",iflow,nflows,t,global_plaquette_lx_conf(smoothed_conf));
	    
	    //make the flower generate the intermediate step between iflow and iflow+1
	    adj_ferm_flower.generate_intermediate_steps(smoothed_conf);
	    
	    adj_ferm_flower.add_or_rem_backfield_to_confs(0,tp->backfield[0]);
	    adj_ferm_flower.flow_fermion(evo_t_to_0);
	    adj_ferm_flower.add_or_rem_backfield_to_confs(1,tp->backfield[0]);
	    
	    // master_printf("t %lg, entry %lg, norm2 %lg\n",t,source[0][0][0][0],double_vector_glb_norm2(source[0],loc_vol));
	  }
	
	
	double flown_back;
	double_vector_glb_scalar_prod(&flown_back,(double*)evo_t_to_0,(double*)ori_0,loc_vol*sizeof(color)/sizeof(double));
	double flown_forw;
	double_vector_glb_scalar_prod(&flown_forw,(double*)ori_t,(double*)evo_0_to_t,loc_vol*sizeof(color)/sizeof(double));
	
	crash(" back: %.16lg , forw: %.16lg",flown_back,flown_forw);
	
#endif
	
	// int nevol=0;
	// for(int i=sp.nsmooth();i>=0;i--)
	//   {
	// 	master_printf("\n");
	// 	nevol+=recu.update(i);
	//   }
	// master_printf("nevol: %d\n",nevol);
	
	//compute the topological charge and the product of topological and tensorial density
	
	//1) draw all hits and fill them at all time we want to measure, evolve to time 0
	//2) use plain guon action from t 0 (eq.7.7 of 1302.5246) to evolve back from tmeas to 0
	//3) solve all hits using fermion action and plug all operators
	//4) take the trace with the hits
	//5) compute topocharge at all intermediate times
	
	//at each step it goes from iflow+1 to iflow
	for(int iflow=nflows-1;iflow>=0;iflow--)
	  {
	    //update conf to iflow
	    double t=dt*iflow;
	    verbosity_lv2_master_printf(" flow back to %d/%d, t %lg\n",iflow,nflows,t);
	    recu.update(iflow);
	    
	    //make the flower generate the intermediate step between iflow and iflow+1
	    adj_ferm_flower.generate_intermediate_steps(smoothed_conf);
	    
	    //have to flow back all sources for which iflow is smaller than meas_each*imeas
	    int imeas_min=iflow/meas_each+1;
	    for(int iflav=0;iflav<nflavs;iflav++)
	      {
		adj_ferm_flower.add_or_rem_backfield_to_confs(0,tp->backfield[iflav]);
		for(int imeas=imeas_min;imeas<nmeas;imeas++)
		  for(int icopy=0;icopy<ncopies;icopy++)
		    for(int ihit=0;ihit<nhits;ihit++)
		      {
			adj_ferm_flower.flow_fermion(source[ind_copy_flav_meas_hit(icopy,iflav,ihit,imeas)]);
			for(int iop=0;iop<nops;iop++) adj_ferm_flower.flow_fermion(source_op[ind_copy_flav_meas_hit_op(icopy,iflav,ihit,imeas,iop)]);
		      }
		adj_ferm_flower.add_or_rem_backfield_to_confs(1,tp->backfield[iflav]);
	      }
	  }
	
	//measure all
	for(int imeas=0;imeas<nmeas;imeas++)
	  {
	    int iflow=imeas*meas_each;
	    master_printf(" imeas: %d, t-back-fluxed: %d\n",imeas,iflow);
	    recu.update(iflow);
	    
	    //plaquette and local charge
	    double plaq=global_plaquette_lx_conf(smoothed_conf);
	    local_topological_charge(topo_dens,smoothed_conf);
	    
	    //total topological charge
	    double tot_charge;
	    double_vector_glb_collapse(&tot_charge,topo_dens,loc_vol);
	    double tot_charge2=double_vector_glb_norm2(topo_dens,1);
	    
	    //build fermionic conf from gluonic one
	    split_lx_vector_into_eo_parts(ferm_conf,smoothed_conf);
	    if(tp->stout_pars.nlevels)
	      stout_smear(ferm_conf,ferm_conf,&tp->stout_pars);
	    
	    for(int icopy=0;icopy<ncopies;icopy++)
	      {
		//reset the local density of tensorial density
		for(int iflav=0;iflav<nflavs;iflav++)
		  for(int iop=0;iop<nops;iop++)
		    vector_reset(tens_dens[ind_op_flav(iop,iflav)]);
		
		//evaluate the tensorial density for all quarks
		for(int ihit=0;ihit<nhits;ihit++)
		  for(int iflav=0;iflav<nflavs;iflav++)
		    {
		      int iso=ind_copy_flav_meas_hit(icopy,iflav,ihit,imeas);
		      split_lx_vector_into_eo_parts(temp_eta,source[iso]);
		      mult_Minv(temp_phi,ferm_conf,tp,iflav,mp->residue,temp_eta);
		      
		      for(int iop=0;iop<nops;iop++)
			for(int eo=0;eo<2;eo++)
			  NISSA_PARALLEL_LOOP(ieo,0,loc_volh)
			    {
			      int ivol=loclx_of_loceo[eo][ieo];
			      complex prod;
			      color_scalar_prod(prod,source_op[ind_copy_flav_meas_hit_op(icopy,iflav,ihit,imeas,iop)][ivol],temp_phi[eo][ieo]);
			      complex_summassign(tens_dens[ind_op_flav(iop,iflav)][ivol],prod);
			    }
		      THREAD_BARRIER();
		    }
		
		//print
		for(int iop=0;iop<nops;iop++)
		  for(int iflav=0;iflav<nflavs;iflav++)
		    {
		      int iop_flav=ind_op_flav(iop,iflav);
		      
		      //final normalization and collapse
		      double_vector_prodassign_double((double*)(tens_dens[iop_flav]),1.0/(glb_vol*nhits),loc_vol*2);
		      complex_vector_glb_collapse(tens[iop_flav],tens_dens[iop_flav],loc_vol);
		      //compute correlation with topocharge
		      compute_tens_dens_topo_correlation(spinpol_dens[iop_flav],tens_dens[iop_flav],topo_dens);
		      complex_vector_glb_collapse(spinpol[iop_flav],spinpol_dens[iop_flav],loc_vol);
		      
		      master_fprintf(fout,"%d\t",iconf);
		      master_fprintf(fout,"%d\t",icopy);
		      master_fprintf(fout,"%d\t",imeas*meas_each);
		      master_fprintf(fout,"%d\t",iflav);
		      master_fprintf(fout,"%d,%d\t",mp->operators[iop].first,mp->operators[iop].second);
		      master_fprintf(fout,"%+16.16lg\t",plaq);
		      master_fprintf(fout,"%+16.16lg" "\t" "%+16.16lg\t",tot_charge,tot_charge2);
		      master_fprintf(fout,"%+16.16lg" "\t" "%+16.16lg\t",spinpol[iop_flav][RE],spinpol[iop_flav][IM]);
		      master_fprintf(fout,"%+16.16lg" "\t" "%+16.16lg\t",tens[iop_flav][RE],tens[iop_flav][IM]);
		      master_fprintf(fout,"\n");
		    }
	      }
	  }
	
	//free
	for(int is=0;is<ntot_sources;is++) nissa_free(source[is]);
	for(int is=0;is<ntot_sources_op;is++) nissa_free(source_op[is]);
	for(int iflav_op=0;iflav_op<nflavs*nops;iflav_op++)
	  {
	    nissa_free(spinpol_dens[iflav_op]);
	    nissa_free(tens_dens[iflav_op]);
	  }
      }
    else
      {
	//allocate tens and spinpol
	complex *tens_dens=nissa_malloc("tens_dens",loc_vol+bord_vol,complex);
	complex *spinpol_dens=nissa_malloc("spinpol_dens",loc_vol,complex);
	
	//allocate and create all fields
	int nfields=ind_copy_flav_hit_phieta(ncopies-1,nflavs-1,nhits-1,nphieta-1)+1;
	color *fields[nfields];
	for(int ifield=0;ifield<nfields;ifield++)
	  fields[ifield]=nissa_malloc("field",loc_vol+bord_vol,color);
	
	//create the fermionic conf
	split_lx_vector_into_eo_parts(ferm_conf,smoothed_conf);
	if(tp->stout_pars.nlevels)
	  stout_smear(ferm_conf,ferm_conf,&tp->stout_pars);
	
	for(int icopy=0;icopy<ncopies;icopy++)
	  for(int ihit=0;ihit<nhits;ihit++)
	    {
	      int isource=ind_copy_flav_hit_phieta(icopy,0,ihit, ieta);
	      generate_fully_undiluted_lx_source(fields[isource],RND_Z4,-1);
	      
	      for(int iflav=0;iflav<nflavs;iflav++)
		{
		  color *eta=fields[ind_copy_flav_hit_phieta(icopy,iflav,ihit,ieta)];
		  color *phi=fields[ind_copy_flav_hit_phieta(icopy,iflav,ihit,iphi)];
		  
		  //if not first flavour, copy the source, and split it
		  if(iflav!=0) vector_copy(eta,fields[isource]);
		  split_lx_vector_into_eo_parts(temp_eta,eta);
		  mult_Minv(temp_phi,ferm_conf,tp,iflav,mp->residue,temp_eta);
		  paste_eo_parts_into_lx_vector(phi,temp_phi);
		}
	    }
	
	fermion_flower_t<4> ferm_flower(dt,all_dirs,true);
	for(int iflow=0;iflow<=nflows;iflow++)
	  {
	    //take current meas index
	    int imeas=iflow/meas_each;
	    
	    if(imeas*meas_each==iflow)
	      {
		//plaquette and local charge
		double plaq=global_plaquette_lx_conf(smoothed_conf);
		local_topological_charge(topo_dens,smoothed_conf);
		
		//total topological charge
		double tot_charge;
		double_vector_glb_collapse(&tot_charge,topo_dens,loc_vol);
		double tot_charge2=double_vector_glb_norm2(topo_dens,1);
		
		for(int icopy=0;icopy<ncopies;icopy++)
		  for(int iflav=0;iflav<nflavs;iflav++)
		    for(int iop=0;iop<nops;iop++)
		      {
			//compute the local tensorial density
			vector_reset(tens_dens);
			for(int ihit=0;ihit<nhits;ihit++)
			  {
			    split_lx_vector_into_eo_parts(temp_eta,fields[ind_copy_flav_hit_phieta(icopy,iflav,ihit,ieta)]);
			    split_lx_vector_into_eo_parts(temp_phi,fields[ind_copy_flav_hit_phieta(icopy,iflav,ihit,iphi)]);
			    summ_dens(tens_dens,chiop,temp[0],temp[1],ferm_conf,tp->backfield[iflav],shift[iop],mask[iop],temp_phi,temp_eta);
			  }
			
			//compute the average tensorial density
			complex tens;
			double_vector_prodassign_double((double*)tens_dens,1.0/(glb_vol*nhits),loc_vol*2);
			complex_vector_glb_collapse(tens,tens_dens,loc_vol);
			
			//compute correlation with topocharge
			complex spinpol;
			compute_tens_dens_topo_correlation(spinpol_dens,tens_dens,topo_dens);
			complex_vector_glb_collapse(spinpol,spinpol_dens,loc_vol);
			
			master_fprintf(fout,"%d\t",iconf);
			master_fprintf(fout,"%d\t",icopy);
			master_fprintf(fout,"%d\t",imeas*meas_each);
			master_fprintf(fout,"%d\t",iflav);
			master_fprintf(fout,"%d,%d\t",mp->operators[iop].first,mp->operators[iop].second);
			master_fprintf(fout,"%+16.16lg\t",plaq);
			master_fprintf(fout,"%+16.16lg" "\t" "%+16.16lg\t",tot_charge,tot_charge2);
			master_fprintf(fout,"%+16.16lg" "\t" "%+16.16lg\t",spinpol[RE],spinpol[IM]);
			master_fprintf(fout,"%+16.16lg" "\t" "%+16.16lg\t",tens[RE],tens[IM]);
			master_fprintf(fout,"\n");
		      }
	      }
	    
	    //update conf to iflow
	    double t=dt*iflow;
	    //verbosity_lv2_
	    master_printf(" flow forward to %d/%d, t %lg, initial plaquette: %.16lg\n",iflow,nflows,t,global_plaquette_lx_conf(smoothed_conf));
	    
	    //make the flower generate the intermediate step between iflow-1 and iflow
	    ferm_flower.generate_intermediate_steps(smoothed_conf);
	    // for(int iflav=0;iflav<nflavs;iflav++)
	    //   {
	    // 	ferm_flower.add_or_rem_backfield_to_confs(0,tp->backfield[iflav]);
	    // 	for(int icopy=0;icopy<ncopies;icopy++)
	    // 	  for(int ihit=0;ihit<nhits;ihit++)
	    // 	    for(int iphieta=0;iphieta<nphieta;iphieta++)
	    // 	      ferm_flower.flow_fermion(fields[ind_copy_flav_hit_phieta(icopy,iflav,ihit,iphieta)]);
	    // 	ferm_flower.add_or_rem_backfield_to_confs(1,tp->backfield[iflav]);
	    //   }
	    ferm_flower.prepare_for_next_flow(smoothed_conf);
	  }
	
	for(int ifield=0;ifield<nfields;ifield++) nissa_free(fields[ifield]);
	nissa_free(spinpol_dens);
	nissa_free(tens_dens);
      }
    
    for(int eo=0;eo<2;eo++)
      {
	nissa_free(chiop[eo]);
	nissa_free(temp_eta[eo]);
	nissa_free(temp_phi[eo]);
	for(int itemp=0;itemp<2;itemp++)
	  nissa_free(temp[itemp][eo]);
	nissa_free(ferm_conf[eo]);
      }
    nissa_free(smoothed_conf);
    nissa_free(topo_dens);
    
    //close
    close_file(fout);
  }
  THREADABLE_FUNCTION_END
  
  std::string spinpol_meas_pars_t::get_str(bool full)
  {
    std::ostringstream os;
    
    os<<"MeasSpinPol\n";
    os<<base_fermionic_meas_t::get_str(full);
    if(use_adjoint_flow!=def_use_adjoint_flow() or full) os<<" UseAdjointFlow\t=\t"<<use_adjoint_flow<<"\n";
    if(use_ferm_conf_for_gluons!=def_use_ferm_conf_for_gluons() or full) os<<" UseFermConfForGluons\t=\t"<<use_ferm_conf_for_gluons<<"\n";
    if(operators.size())
      {
	os<<" Operators\t=\t{";
	for(size_t i=0;i<operators.size();i++)
	  {
	    os<<"("<<operators[i].first<<","<<operators[i].second<<")";
	    if(i!=operators.size()-1) os<<",";
	  }
	os<<"}\n";
      }
    os<<smooth_pars.get_str(full);
    
    return os.str();
  }
}
