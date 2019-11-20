#ifdef HAVE_CONFIG_H
 #include "config.hpp"
#endif

#include "base/vectors.hpp"
#include "communicate/borders.hpp"
#include "geometry/geometry_eo.hpp"
#include "new_types/su3_op.hpp"
#include "threads/threads.hpp"

namespace nissa
{
  THREADABLE_FUNCTION_3ARG(apply_st2Doe, color*,out, eo_ptr<quad_su3>,conf, color*,in)
  {
    if(!check_borders_valid(conf[EVN])||!check_borders_valid(conf[ODD]))
      communicate_ev_and_od_quad_su3_borders(conf);
    if(!check_borders_valid(in)) communicate_ev_color_borders(in);
    
    GET_THREAD_ID();
    NISSA_PARALLEL_LOOP(io,0,loc_volh)
      {
	//neighbours search
	int evup0=loceo_neighup[ODD][io][0];
	int evdw0=loceo_neighdw[ODD][io][0];
	
	//derivative in the time direction - without self-summ
	unsafe_su3_prod_color(      out[io],conf[ODD][io   ][0],in[evup0]);
	su3_dag_subt_the_prod_color(out[io],conf[EVN][evdw0][0],in[evdw0]);
	
	//derivatives in the spatial direction - with self summ
	for(int mu=1;mu<4;mu++)
	  {
	    int evup=loceo_neighup[ODD][io][mu];
	    int evdw=loceo_neighdw[ODD][io][mu];
	    
	    su3_summ_the_prod_color(    out[io],conf[ODD][io  ][mu],in[evup]);
	    su3_dag_subt_the_prod_color(out[io],conf[EVN][evdw][mu],in[evdw]);
	  }
      }
    NISSA_PARALLEL_LOOP_END;
    
    set_borders_invalid(out);
  }
  THREADABLE_FUNCTION_END
  
  //put the 0.5 factor
  THREADABLE_FUNCTION_3ARG(apply_stDoe, color*,out, eo_ptr<quad_su3>,conf, color*,in)
  {
    apply_st2Doe(out,conf,in);
    
    GET_THREAD_ID();
    NISSA_PARALLEL_LOOP(io,0,loc_volh)
      for(int ic=0;ic<3;ic++)
	for(int ri=0;ri<2;ri++)
	  out[io][ic][ri]*=0.5;
    NISSA_PARALLEL_LOOP_END;
    
    set_borders_invalid(out);
  }
  THREADABLE_FUNCTION_END
  
  THREADABLE_FUNCTION_3ARG(apply_stDeo_half, color*,out, eo_ptr<quad_su3>,conf, color*,in)
  {
    if(!check_borders_valid(conf[EVN])||!check_borders_valid(conf[ODD]))
      communicate_ev_and_od_quad_su3_borders(conf);
    if(!check_borders_valid(in)) communicate_od_color_borders(in);
    
    GET_THREAD_ID();
    NISSA_PARALLEL_LOOP(ie,0,loc_volh)
      {
	//neighbours search
	int odup0=loceo_neighup[EVN][ie][0];
	int oddw0=loceo_neighdw[EVN][ie][0];
	
	//derivative in the time direction - without self-summ
	unsafe_su3_prod_color(      out[ie],conf[EVN][ie   ][0],in[odup0]);
	su3_dag_subt_the_prod_color(out[ie],conf[ODD][oddw0][0],in[oddw0]);
	
	//derivatives in the spatial direction - with self summ
	for(int mu=1;mu<4;mu++)
	  {
	    int odup=loceo_neighup[EVN][ie][mu];
	    int oddw=loceo_neighdw[EVN][ie][mu];
	    
	    su3_summ_the_prod_color(    out[ie],conf[EVN][ie  ][mu],in[odup]);
	    su3_dag_subt_the_prod_color(out[ie],conf[ODD][oddw][mu],in[oddw]);
	  }
	
	//Doe contains 1/2, we put an additional one
	color_prod_double(out[ie],out[ie],0.25);
      }
    NISSA_PARALLEL_LOOP_END;
    
    set_borders_invalid(out);
  }
  THREADABLE_FUNCTION_END
  
  THREADABLE_FUNCTION_5ARG(apply_stD2ee_m2, color*,out, eo_ptr<quad_su3>,conf, color*,temp, double,mass2, color*,in)
  {
    GET_THREAD_ID();
    if(IS_MASTER_THREAD)
      {
	//check arguments
	if(out==in)   crash("out==in!");
	if(out==temp) crash("out==temp!");
	if(temp==in)  crash("temp==in!");
      }
    START_TIMING(portable_stD_app_time,nportable_stD_app);
    
    if(!check_borders_valid(conf[EVN])||!check_borders_valid(conf[ODD]))
      communicate_ev_and_od_quad_su3_borders(conf);
    if(!check_borders_valid(in)) communicate_ev_color_borders(in);
    
    NISSA_PARALLEL_LOOP_CUDA(io,0,loc_volh)
      {
	//neighbours search
	int evup0=loceo_neighup[ODD][io][0];
	int evdw0=loceo_neighdw[ODD][io][0];
	
	//derivative in the time direction - without self-summ
	unsafe_su3_prod_color(      temp[io],conf[ODD][io   ][0],in[evup0]);
	su3_dag_subt_the_prod_color(temp[io],conf[EVN][evdw0][0],in[evdw0]);
	
	//derivatives in the spatial direction - with self summ
	for(int mu=1;mu<NDIM;mu++)
	  {
	    int evup=loceo_neighup[ODD][io][mu];
	    int evdw=loceo_neighdw[ODD][io][mu];
	    
	    su3_summ_the_prod_color(    temp[io],conf[ODD][io  ][mu],in[evup]);
	    su3_dag_subt_the_prod_color(temp[io],conf[EVN][evdw][mu],in[evdw]);
	  }
      }
    NISSA_PARALLEL_LOOP_END;
    
    set_borders_invalid(temp);
    communicate_od_color_borders(temp);
    
    //we still apply Deo, but then we put a - because we should apply Doe^+=-Deo
    NISSA_PARALLEL_LOOP(ie,0,loc_volh)
      {
	int odup0=loceo_neighup[EVN][ie][0];
	int oddw0=loceo_neighdw[EVN][ie][0];
	
	unsafe_su3_prod_color(      out[ie],conf[EVN][ie   ][0],temp[odup0]);
	su3_dag_subt_the_prod_color(out[ie],conf[ODD][oddw0][0],temp[oddw0]);
	
	for(int mu=1;mu<4;mu++)
	  {
	    int odup=loceo_neighup[EVN][ie][mu];
	    int oddw=loceo_neighdw[EVN][ie][mu];
	    
	    su3_summ_the_prod_color(    out[ie],conf[EVN][ie  ][mu],temp[odup]);
	    su3_dag_subt_the_prod_color(out[ie],conf[ODD][oddw][mu],temp[oddw]);
	  }
      }
    NISSA_PARALLEL_LOOP_END;
    
    if(mass2!=0)
      {
	NISSA_PARALLEL_LOOP(ie,0,loc_volh)
	  for(int ic=0;ic<3;ic++)
	    for(int ri=0;ri<2;ri++)
	      out[ie][ic][ri]=mass2*in[ie][ic][ri]-out[ie][ic][ri]*0.25;
	NISSA_PARALLEL_LOOP_END;
      }
    else
      {
	NISSA_PARALLEL_LOOP(ie,0,loc_volh)
	  for(int ic=0;ic<3;ic++)
	    for(int ri=0;ri<2;ri++)
	      out[ie][ic][ri]*=-0.25;
	NISSA_PARALLEL_LOOP_END;
      }
    set_borders_invalid(out);
    
    STOP_TIMING(portable_stD_app_time);
  }
  THREADABLE_FUNCTION_END
}
