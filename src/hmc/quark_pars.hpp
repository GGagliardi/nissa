#ifndef _QUARK_PARS_HPP
#define _QUARK_PARS_HPP

#include <stdio.h>
#include <string>
#include <string.h>
#include <sstream>

#include "base/debug.hpp"
#include "routines/ios.hpp"

namespace nissa
{
  namespace ferm_discretiz
  {
    const int nknown=3;
    enum name_t{ROOT_STAG,ROOT_TM_CLOV,OVERLAP};
    const name_t list[nknown]={ROOT_STAG,ROOT_TM_CLOV,OVERLAP};
    const char text[nknown][20]={"RootStag","RootTmClov","Overlap"};
    
    //change from name to string
    inline std::string str_from_name(name_t name)
    {
      switch(name)
	{
	case ROOT_STAG:return "RootStag";break;
	case ROOT_TM_CLOV:return "RootTMClov";break;
	case OVERLAP:return "Overlap";break;
	}
      
      return "";
    }
    
    //string into name
    inline name_t name_from_str(const char *in)
    {
      //search
      int iname=0;
      while(iname<nknown && strcasecmp(in,text[iname])!=0) iname++;
      
      //check
      if(iname==nknown) crash("unknown fermion discretiz action: %s",in);
      
      return list[iname];
    }
    
    //determine if staggered or not
    inline bool is_stag(name_t name)
    {
      switch(name)
	{
	case ROOT_STAG: return true;break;
	case ROOT_TM_CLOV: return false;break;
	case OVERLAP: return false;break;
	}
      
      return false;
    }
    
    //determine if clover or not
    inline bool include_clover(name_t name)
    {
      switch(name)
	{
	case ROOT_STAG: return false;break;
	case ROOT_TM_CLOV: return true;break;
	case OVERLAP: return false;break;
	}
      
      return false;
    }
    
    //root needed to have 1 quarks
    inline int root_needed(name_t name)
    {
      switch(name)
	{
	case ROOT_STAG:return 4;break;
	case ROOT_TM_CLOV:return 2;break;
	case OVERLAP:return 1;break;
	}
      
      return 1;
    }
  }
  
  //quark content
  struct quark_content_t
  {
    std::string name;
    int deg;
    ferm_discretiz::name_t discretiz;
    double mass;
    double mass_overlap;
    double kappa;
    double cSW;
    double re_pot;
    double im_pot;
    double charge;
    
    std::string def_name(){return "quark";}
    int def_deg(){return 1;}
    ferm_discretiz::name_t def_discretiz(){return ferm_discretiz::ROOT_STAG;}
    double def_mass(){return 0.1;}
    double def_mass_overlap(){return 1.0;}
    double def_kappa(){return 0.125;}
    double def_cSW(){return 0;}
    double def_re_pot(){return 0;}
    double def_im_pot(){return 0;}
    double def_charge(){return 0;}
    
    int master_fprintf(FILE *fout,int full) {return nissa::master_fprintf(fout,"%s",get_str().c_str());}
    std::string get_str(bool full=false)
    {
      std::ostringstream os;
      os<<"Quark\t\t=\t\""<<name.c_str()<<"\"\n";
      if(full or deg!=def_deg()) os<<" Degeneracy\t=\t"<<deg<<"\n";
      if(full or discretiz!=def_discretiz()) os<<" Discretiz\t=\t"<<ferm_discretiz::str_from_name(discretiz)<<"\n";
      if(full or mass!=def_mass()) os<<" Mass\t\t=\t"<<mass<<"\n";
      if(full or mass_overlap!=def_mass_overlap()) os<<" MassOverlap\t\t=\t"<<mass_overlap<<"\n";
      if(full or kappa!=def_kappa()) os<<" Kappa\t\t=\t"<<kappa<<"\n";
      if(full or cSW!=def_cSW()) os<<" cSW\t\t=\t"<<cSW<<"\n";
      if(full or re_pot!=def_re_pot()) os<<" RePotCh\t=\t"<<re_pot<<"\n";
      if(full or im_pot!=def_im_pot()) os<<" ImPotCh\t=\t"<<im_pot<<"\n";
      if(full or charge!=def_charge()) os<<" ElecCharge\t=\t"<<charge<<"\n";
      
      return os.str();
    }
    
    int is_nonstandard()
    {
      return
	deg!=def_deg() or
	discretiz!=def_discretiz() or
	mass!=def_mass() or
	mass_overlap!=def_mass_overlap() or
	kappa!=def_kappa() or
	cSW!=def_cSW() or
	re_pot!=def_re_pot() or
	im_pot!=def_im_pot() or
	charge!=def_charge();
    }
    
    quark_content_t() :
      deg(def_deg()),
      discretiz(def_discretiz()),
      mass(def_mass()),
      mass_overlap(def_mass_overlap()),
      kappa(def_kappa()),
      cSW(def_cSW()),
      re_pot(def_re_pot()),
      im_pot(def_im_pot()),
      charge(def_charge()) {}
  };
}

#endif
