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

#ifndef PF_DYNAMIC_RANGE_COMPRESSOR_H
#define PF_DYNAMIC_RANGE_COMPRESSOR_H

//#ifdef __SSE2__
//#include "../rt/rtengine/sleefsseavx.c"
//#else
#include "../rt/rtengine/sleef.c"
//#endif

#include "../base/splinecurve.hh"
#include "../base/processor.hh"

namespace PF 
{

  class DynamicRangeCompressorPar: public OpParBase
  {
    Property<float> amount;
    Property<bool> enable_equalizer;
    Property<float> blacks_amount;
    Property<float> shadows_amount;
    Property<float> midtones_amount;
    Property<float> highlights_amount;
    Property<float> whites_amount;

    Property<int> bilateral_iterations;
    Property<float> bilateral_sigma_s;
    Property<float> bilateral_sigma_r;

    Property<float> strength_s, strength_h;
    Property<float> local_contrast;

    Property<bool> show_residual;
    bool show_residual_;

    ProcessorBase* loglumi;
    ProcessorBase* bilateral;

    PF::ICCProfile* profile;

    SplineCurve tone_curve;
    bool caching;
  public:
    float vec8[UCHAR_MAX+1];
    float vec16[65536/*USHRT_MAX+1*/];

    DynamicRangeCompressorPar();

    bool has_intensity() { return false; }
    bool needs_caching() {
      return caching;
    }

    void compute_padding( VipsImage* full_res, unsigned int id, unsigned int level );
    void propagate_settings();

    PF::ICCProfile* get_profile() { return profile; }

    float get_strength_s() { return strength_s.get(); }
    float get_strength_h() { return strength_h.get(); }
    float get_local_contrast() { return local_contrast.get(); }
    bool get_show_residual() { return show_residual_; }

    float get_amount() { return amount.get(); }
    bool get_equalizer_enabled() { return enable_equalizer.get(); }
    SplineCurve& get_tone_curve() { return tone_curve; }
      
    VipsImage* build(std::vector<VipsImage*>& in, int first, 
		     VipsImage* imap, VipsImage* omap, 
		     unsigned int& level);
  };

  

  template < OP_TEMPLATE_DEF > 
  class DynamicRangeCompressorProc
  {
  public: 
    void render(VipsRegion** in, int n, int in_first,
								VipsRegion* imap, VipsRegion* omap, 
								VipsRegion* out, OpParBase* par) 
    {
      std::cout<<"DynamicRangeCompressorProc::render() called."<<std::endl;
    }
  };


  template < OP_TEMPLATE_DEF_CS_SPEC >
  class DynamicRangeCompressorProc< OP_TEMPLATE_IMP_CS_SPEC(PF_COLORSPACE_RGB) >
  {
  public:
    void render(VipsRegion** ireg, int n, int in_first,
                VipsRegion* imap, VipsRegion* omap,
                VipsRegion* oreg, OpParBase* par)
    {
      static const double inv_log_base = 1.0 / log(10.0);
      static const double min_value = -2.5, max_value = 2;
      //static const double gamma = log(50) * inv_log_base / (max_value - min_value);
      //if( n != 3 ) return;
      if( ireg[0] == NULL ) return;
      //if( ireg[1] == NULL ) return;
      //if( ireg[2] == NULL ) return;

      DynamicRangeCompressorPar* opar = dynamic_cast<DynamicRangeCompressorPar*>(par);
      if( !opar ) return;
      PF::ICCProfile* profile = opar->get_profile();
      if( !profile ) return;

      const float gamma_s = 1.0/(1.0 + 0.09*opar->get_strength_s());
      const float gamma_h = 1.0/(1.0 + 0.09*opar->get_strength_h());
      const float lc = opar->get_local_contrast() + 1;
      const float lcm = 1.0f-lc;

      VipsRect *r = &oreg->valid;
      int width = r->width;
      int line_size = r->width * oreg->im->Bands;
      //int width = r->width;
      int height = r->height;

      if( false && r->left<10000 && r->top<10000 )
        std::cout<<"DynamicRangeCompressorProc::render(): region="<<r->width<<"x"<<r->height<<"+"<<r->left<<","<<r->top<<std::endl;

      T* psmooth;
      T* plog;
      T* pin;
      T* pout;
      //typename FormatInfo<T>::SIGNED diff;
      float diff, out, lout;
      float grey, ngrey, intensity, L, lL;
      int x, y, pos;
      //float threshold = opar->get_threshold()*FormatInfo<T>::RANGE;
      float bias = profile->perceptual2linear(0.5);
      //float lbias = log(bias) * inv_log_base;

      for( y = 0; y < height; y++ ) {
        psmooth = (T*)VIPS_REGION_ADDR( ireg[0], r->left, r->top + y );
        plog = (T*)VIPS_REGION_ADDR( ireg[1], r->left, r->top + y );
        pin = (T*)VIPS_REGION_ADDR( ireg[2], r->left, r->top + y );
        pout = (T*)VIPS_REGION_ADDR( oreg, r->left, r->top + y );

        for( x = 0; x < width; x++, pin+=3, pout+=3, psmooth++, plog++ ) {
          //pout[0] = pin[0]; pout[1] = pin[1]; pout[2] = pin[2]; continue;

          float l = ( plog[0]*(6*2+1) ) - 6;
          float s = ( psmooth[0]*(6*2+1) ) - 6;
          //float s = (0.1*(plog[0]+psmooth[0])) - 6;

          diff = l - s;
          float gamma1 = (s>0) ? gamma_h : gamma_s;
          float gamma2 = (l>0) ? gamma_h : gamma_s;
          //double exp = s;
          //double exp = ((s * gamma) + lc*diff);
          double exp = opar->get_show_residual() ? s : ( lc * ((s * gamma1) + diff) + lcm * l * gamma2 );

          //out = pow( 10.0, l * gamma );
          //out = pow( 10.0, exp );
          //out = psmooth[0];
          out = xexp10( exp );

          out *= bias;

          //out = pow( 10.0, (0.1*psmooth[0]) - 6 + diff/** gamma + diff*/ );
          //out = (((psmooth[0]-50) * gamma) + 50 + diff)/100;
          //out = (((plog[0]-50) * gamma) + 50)/100;

          //if( profile->is_linear() )
          //  lout = profile->perceptual2linear(out);
          //else lout = out;

          //pout[0] = pout[1] = pout[2] = out;

          /**/
          //intensity = 0;
          L = profile->get_lightness(pin[0], pin[1], pin[2]);
          if( opar->get_equalizer_enabled() ) {
            ngrey = (L+FormatInfo<T>::MIN)/FormatInfo<T>::RANGE;
            intensity = opar->get_tone_curve().get_value( ngrey ) * opar->get_amount();
          } else
            intensity = opar->get_amount();
          const float ratio = (L>1.0e-12) ? out / L : 0;
          pout[0] = pin[0] * ratio; pout[1] = pin[1] * ratio; pout[2] = pin[2] * ratio;
          if(false)
          //if(false && x==(width-1) && y==(r->height-1) && r->left==0 && r->top==0)
            std::cout<<"L="<<L<<"  plog[0]="<<*plog<<"  psmooth[0]="<<*psmooth
            <<"  l="<<l<<"  s="<<s<<"  gamma_s="<<gamma_s<<"  diff="<<diff<<"  out="<<out
            <<"  ratio="<<ratio<<std::endl;

          /**/
        }
      }
    }
  };


  ProcessorBase* new_dynamic_range_compressor();

}

#endif 


