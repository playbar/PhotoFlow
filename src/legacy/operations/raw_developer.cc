/* 
 */

/*

    Copyright (C) 2014 Ferrero Andrea

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.


 */

/*

    These files are distributed with PhotoFlow - http://aferrero2707.github.io/PhotoFlow/

 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../../operations/convertformat.hh"
#include "raw_preprocessor.hh"
#include "../../operations/ca_correct.hh"
#include "../../operations/amaze_demosaic.hh"
#include "../../operations/lmmse_demosaic.hh"
#include "../../operations/igv_demosaic.hh"
#include "../../operations/xtrans_demosaic.hh"
#include "../../operations/fast_demosaic.hh"
#include "../../operations/fast_demosaic_xtrans.hh"
#include "../../operations/false_color_correction.hh"
#include "raw_output.hh"
#include "../../operations/hotpixels.hh"
#include "../../operations/lensfun.hh"

#include "raw_developer.hh"


PF::RawDeveloperV1Par::RawDeveloperV1Par():
  OpParBase(), output_format( VIPS_FORMAT_NOTSET ),
  lf_prop_camera_maker( "lf_camera_maker", this ),
  lf_prop_camera_model( "lf_camera_model", this ),
  lf_prop_lens( "lf_lens", this ),
  enable_distortion( "lf_enable_distortion", this, false ),
  enable_tca( "lf_enable_tca", this, false ),
  enable_vignetting( "lf_enable_vignetting", this, false ),
  enable_all( "lf_enable_all", this, false ),
  tca_method("tca_method",this,PF::PF_TCA_CORR_PROFILED_AUTO,"TCA_CORR_PROFILED_AUTO",_("profiled + auto")),
  demo_method("demo_method",this,PF::PF_DEMO_AMAZE,"AMAZE","Amaze"),
	fcs_steps("fcs_steps",this,0),
	caching_enabled( true )
{
  tca_method.add_enum_value(PF::PF_TCA_CORR_AUTO,"TCA_CORR_AUTO",_("auto"));
  tca_method.add_enum_value(PF::PF_TCA_CORR_PROFILED,"TCA_CORR_PROFILED",_("profiled"));
  tca_method.add_enum_value(PF::PF_TCA_CORR_PROFILED_AUTO,"TCA_CORR_PROFILED_AUTO",_("profiled + auto"));
  //tca_method.add_enum_value(PF::PF_TCA_CORR_MANUAL,"TCA_CORR_MANUAL",_("manual"));

  demo_method.add_enum_value(PF::PF_DEMO_AMAZE,"AMAZE","Amaze");
  //demo_method.add_enum_value(PF::PF_DEMO_FAST,"FAST","Fast");
  demo_method.add_enum_value(PF::PF_DEMO_LMMSE,"LMMSE","LMMSE");
  demo_method.add_enum_value(PF::PF_DEMO_IGV,"IGV","Igv");

  amaze_demosaic = new_amaze_demosaic();
  lmmse_demosaic = new_lmmse_demosaic();
  igv_demosaic = new_igv_demosaic();
  xtrans_demosaic = new_xtrans_demosaic();
  fast_demosaic = new_fast_demosaic();
  fast_demosaic_xtrans = new_fast_demosaic_xtrans();
  FastDemosaicXTransPar* xtrans_par =
      dynamic_cast<FastDemosaicXTransPar*>(fast_demosaic_xtrans->get_par());
  if( xtrans_par ) xtrans_par->set_normalize( true );
  raw_preprocessor = new_raw_preprocessor_v1();
  ca_correct = new_ca_correct();
  lensfun = new_lensfun();
  raw_output = new_raw_output_v1();
  hotpixels = new_hotpixels();
  convert_format = new_convert_format();
	for(int ifcs = 0; ifcs < 4; ifcs++) 
		fcs[ifcs] = new_false_color_correction();

  map_properties( raw_preprocessor->get_par()->get_properties() );
  map_properties( ca_correct->get_par()->get_properties() );
  map_properties( raw_output->get_par()->get_properties() );
  map_properties( hotpixels->get_par()->get_properties() );

  set_type("raw_developer" );

  set_default_name( _("RAW developer") );
}



PF::wb_mode_t PF::RawDeveloperV1Par::get_wb_mode()
{
  PF::wb_mode_t result = PF::WB_CAMERA;
  PF::RawPreprocessorV1Par* par = dynamic_cast<PF::RawPreprocessorV1Par*>( raw_preprocessor->get_par() );
  if( par ) result = par->get_wb_mode();
  return result;
}


void PF::RawDeveloperV1Par::set_wb(float r, float g, float b)
{
  PF::RawPreprocessorV1Par* par = dynamic_cast<PF::RawPreprocessorV1Par*>( raw_preprocessor->get_par() );
  if( par ) par->set_wb(r,g,b);
}

int PF::RawDeveloperV1Par::get_hotp_fixed()
{
  int result = 0;
  PF::HotPixelsPar* par = dynamic_cast<PF::HotPixelsPar*>( hotpixels->get_par() );
  if( par ) result = par->get_pixels_fixed();
  return result;
}

void PF::RawDeveloperV1Par::get_wb(float* mul)
{
  PF::RawPreprocessorV1Par* par = dynamic_cast<PF::RawPreprocessorV1Par*>( raw_preprocessor->get_par() );
  if( par ) {
    mul[0] = par->get_wb_red();
    mul[1] = par->get_wb_green();
    mul[2] = par->get_wb_blue();
  }
}


std::string PF::RawDeveloperV1Par::get_lf_maker()
{
  std::string result;
  LensFunPar* lfpar = dynamic_cast<LensFunPar*>( lensfun->get_par() );
  if( !lfpar ) {
    std::cout<<"RawDeveloperPar::build(): could not get LensFunPar object."<<std::endl;
    return result;
  }
  return lfpar->camera_maker();
}


std::string PF::RawDeveloperV1Par::get_lf_model()
{
  std::string result;
  LensFunPar* lfpar = dynamic_cast<LensFunPar*>( lensfun->get_par() );
  if( !lfpar ) {
    std::cout<<"RawDeveloperPar::build(): could not get LensFunPar object."<<std::endl;
    return result;
  }
  return lfpar->camera_model();
}


std::string PF::RawDeveloperV1Par::get_lf_lens()
{
  std::string result;
  LensFunPar* lfpar = dynamic_cast<LensFunPar*>( lensfun->get_par() );
  if( !lfpar ) {
    std::cout<<"RawDeveloperPar::build(): could not get LensFunPar object."<<std::endl;
    return result;
  }
  return lfpar->lens();
}


VipsImage* PF::RawDeveloperV1Par::build(std::vector<VipsImage*>& in, int first,
				     VipsImage* imap, VipsImage* omap, 
				     unsigned int& level)
{
  if( (in.size()<1) || (in[0]==NULL) )
    return NULL;
  
  VipsImage* out_demo;
  std::vector<VipsImage*> in2;

  size_t blobsz;
  if( vips_image_get_blob( in[0], "raw_image_data",
			   (void**)&image_data, 
			   &blobsz ) ) {
    std::cout<<"RawDeveloperV1Par::build(): could not extract raw_image_data."<<std::endl;
    return NULL;
  }
	//std::cout<<"RawDeveloperV1Par::build(): blobsz="<<blobsz<<std::endl;
  if( blobsz != sizeof(dcraw_data_t) ) {
    std::cout<<"RawDeveloperV1Par::build(): wrong raw_image_data size."<<std::endl;
    return NULL;
  }
  
  
  LensFunPar* lfpar = dynamic_cast<LensFunPar*>( lensfun->get_par() );
  if( !lfpar ) {
    std::cout<<"RawDeveloperPar::build(): could not get LensFunPar object."<<std::endl;
    return NULL;
  }
  int lf_modflags = lfpar->get_flags( in[0] );

  VipsImage* input_img = in[0];
  //std::cout<<"RawDeveloperV1Par::build(): input_img->Bands="<<input_img->Bands<<std::endl;
  if( input_img->Bands != 3 ) {
    raw_preprocessor->get_par()->set_image_hints( in[0] );
    raw_preprocessor->get_par()->set_format( VIPS_FORMAT_FLOAT );
    VipsImage* image = raw_preprocessor->get_par()->build( in, 0, NULL, NULL, level );
    //VipsImage* image = in[0]; PF_REF( image, "");
    if( !image )
      return NULL;

    VipsImage* out_ca = NULL;
    PF::ProcessorBase* demo = NULL;
    if( PF::check_xtrans(image_data->idata.filters) ) {
      out_ca = image;
      //PF_REF( out_ca, "RawDeveloperV1Par::build(): in[0] ref for xtrans");
      //demo = fast_demosaic_xtrans;
      demo = xtrans_demosaic;
    } else {
      in2.push_back( image );
      hotpixels->get_par()->set_image_hints( image );
      hotpixels->get_par()->set_format( VIPS_FORMAT_FLOAT );
      HotPixelsPar* hppar = dynamic_cast<HotPixelsPar*>( hotpixels->get_par() );
      hppar->set_pixels_fixed( 0 );
      VipsImage* out_hotp = hotpixels->get_par()->build( in2, 0, NULL, NULL, level );
      g_object_unref( image );
      //VipsImage* out_hotp = image;

      CACorrectPar* capar = dynamic_cast<CACorrectPar*>( ca_correct->get_par() );
      if( !capar ) {
        std::cout<<"RawDeveloperPar::build(): could not get CACorrectPar object."<<std::endl;
        return NULL;
      }

      capar->set_enable_ca( false );
      capar->set_auto_ca( false );
      if( enable_tca.get() || enable_all.get() ) {
        switch(tca_method.get_enum_value().first) {
        case PF::PF_TCA_CORR_AUTO:
          capar->set_enable_ca( true );
          capar->set_auto_ca( true );
          break;
        case PF::PF_TCA_CORR_PROFILED_AUTO:
          if( (lf_modflags & LF_MODIFY_TCA) == 0) {
            capar->set_enable_ca( true );
            capar->set_auto_ca( true );
          }
          break;
        case PF::PF_TCA_CORR_MANUAL:
          capar->set_enable_ca( true );
          capar->set_auto_ca( false );
          break;
        default:
          break;
        }
      }

      in2.clear(); in2.push_back( out_hotp );
      ca_correct->get_par()->set_image_hints( out_hotp );
      ca_correct->get_par()->set_format( VIPS_FORMAT_FLOAT );
      out_ca = ca_correct->get_par()->build( in2, 0, NULL, NULL, level );
      g_object_unref( out_hotp );
      //VipsImage* out_ca = out_hotp;

      switch( demo_method.get_enum_value().first ) {
      case PF::PF_DEMO_FAST: demo = fast_demosaic; break;
      case PF::PF_DEMO_AMAZE: demo = amaze_demosaic; break;
      case PF::PF_DEMO_LMMSE: demo = lmmse_demosaic; break;
      case PF::PF_DEMO_IGV: demo = igv_demosaic; break;
      default: break;
      }
      //PF::ProcessorBase* demo = amaze_demosaic;
      //PF::ProcessorBase* demo = igv_demosaic;
      //PF::ProcessorBase* demo = fast_demosaic;
    }
    if( !demo ) return NULL;
    in2.clear(); in2.push_back( out_ca );
    demo->get_par()->set_image_hints( out_ca );
    demo->get_par()->set_format( VIPS_FORMAT_FLOAT );
    out_demo = demo->get_par()->build( in2, 0, NULL, NULL, level );
    g_object_unref( out_ca );

    for(int ifcs = 0; ifcs < VIPS_MIN(fcs_steps.get(),4); ifcs++) {
      VipsImage* temp = out_demo;
      in2.clear(); in2.push_back( temp );
      fcs[ifcs]->get_par()->set_image_hints( temp );
			fcs[ifcs]->get_par()->set_format( VIPS_FORMAT_FLOAT );
			out_demo = fcs[ifcs]->get_par()->build( in2, 0, NULL, NULL, level );
			PF_UNREF( temp, "RawDeveloperV1Par::build(): temp unref");
		}
  } else {
    raw_preprocessor->get_par()->set_image_hints( in[0] );
    raw_preprocessor->get_par()->set_format( VIPS_FORMAT_FLOAT );
    VipsImage* image = raw_preprocessor->get_par()->build( in, 0, NULL, NULL, level );
    if( !image )
      return NULL;
  
    out_demo = image;
  }

  /**/
  lensfun->get_par()->set_image_hints( out_demo );
  lensfun->get_par()->set_format( VIPS_FORMAT_FLOAT );
  lfpar->set_vignetting_enabled( enable_vignetting.get() || enable_all.get() );
  lfpar->set_distortion_enabled( enable_distortion.get() || enable_all.get() );
  lfpar->set_tca_enabled( false );
  if( enable_tca.get() || enable_all.get() ) {
    switch(tca_method.get_enum_value().first) {
    case PF::PF_TCA_CORR_PROFILED:
      lfpar->set_tca_enabled( true );
      break;
    case PF::PF_TCA_CORR_PROFILED_AUTO:
      if( lf_modflags & LF_MODIFY_TCA )
        lfpar->set_tca_enabled( true );
      break;
    default:
      break;
    }
  }
  in2.clear(); in2.push_back( out_demo );
  VipsImage* out_lf = lensfun->get_par()->build( in2, 0, NULL, NULL, level );
  g_object_unref( out_demo );


  raw_output->get_par()->set_image_hints( out_lf );
  raw_output->get_par()->set_format( VIPS_FORMAT_FLOAT );
  RawPreprocessorV1Par* rppar = dynamic_cast<RawPreprocessorV1Par*>( raw_preprocessor->get_par() );
  RawOutputV1Par* ropar = dynamic_cast<RawOutputV1Par*>( raw_output->get_par() );
  if( rppar && ropar ) {
    switch( rppar->get_wb_mode() ) {
    case WB_SPOT:
    case WB_COLOR_SPOT:
      ropar->set_wb( rppar->get_wb_red(), rppar->get_wb_green(), rppar->get_wb_blue() );
      break;
    default:
      ropar->set_wb( rppar->get_wb_red()*rppar->get_camwb_corr_red(),
          rppar->get_wb_green()*rppar->get_camwb_corr_green(),
          rppar->get_wb_blue()*rppar->get_camwb_corr_blue() );
      break;
    }
  }

  in2.clear(); in2.push_back( out_lf );
  VipsImage* out = raw_output->get_par()->build( in2, 0, NULL, NULL, level );
  g_object_unref( out_lf );
  /**/

  //VipsImage* gamma_in = out_demo;
  VipsImage* gamma_in = out;
  VipsImage* gamma = gamma_in;
  /*
  if( vips_gamma(gamma_in, &gamma, "exponent", (float)(2.2), NULL) ) {
    g_object_unref(gamma_in);
    return NULL;
  }
  */

  /*
  void *data;
  size_t data_length;
  cmsHPROFILE profile_in;
  if( gamma ) {
    if( !vips_image_get_blob( gamma, VIPS_META_ICC_NAME, 
                              &data, &data_length ) ) {
    
      profile_in = cmsOpenProfileFromMem( data, data_length );
      if( profile_in ) {
        char tstr[1024];
        cmsGetProfileInfoASCII(profile_in, cmsInfoDescription, "en", "US", tstr, 1024);
        std::cout<<"RawDeveloperV1::build(): convert_format input profile: "<<tstr<<std::endl;
        cmsCloseProfile( profile_in );
      }
    }  
  }
  */

  VipsImage* out2;
  std::vector<VipsImage*> in3;
  in3.push_back( gamma );
  convert_format->get_par()->set_image_hints( gamma );
  convert_format->get_par()->set_format( get_format() );
  out2 = convert_format->get_par()->build( in3, 0, NULL, NULL, level );
  g_object_unref( gamma );

  set_image_hints( out2 );
  /*
  if( out2 ) {
    if( !vips_image_get_blob( out2, VIPS_META_ICC_NAME, 
                              &data, &data_length ) ) {
    
      profile_in = cmsOpenProfileFromMem( data, data_length );
      if( profile_in ) {
        char tstr[1024];
        cmsGetProfileInfoASCII(profile_in, cmsInfoDescription, "en", "US", tstr, 1024);
        std::cout<<"RawDeveloperV1::build(): convert_format output profile: "<<tstr<<std::endl;
        cmsCloseProfile( profile_in );
      }
    }  
  }
  */

//std::cout<<"RawDeveloperV1::build(): output image: "<<out2<<std::endl;
  return out2;
}
