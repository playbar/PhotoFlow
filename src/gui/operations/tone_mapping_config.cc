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

#include "tone_mapping_config.hh"


PF::ToneMappingConfigGUI::ToneMappingConfigGUI( PF::Layer* layer ):
  OperationConfigGUI( layer, "Tone Mapping" ),
  exposureSlider( this, "exposure", _("exposure"), 0, -10, 10, 0.1, 1 ),
  modeSelector( this, "method", "method: ", 0 ),
  gamma_slider( this, "gamma", _("gamma adjustment"), 1, 1, 5, 0.2, 1, 1 ),
  filmic_A_slider( this, "filmic_A", _("shoulder strength"), 0.5, 0, 1, 0.02, 0.1, 1 ),
  filmic_B_slider( this, "filmic_B", _("linear strength"), 0.5, 0, 1, 0.02, 0.1, 1 ),
  filmic_C_slider( this, "filmic_C", _("linear angle"), 0.5, 0, 1, 0.02, 0.1, 1 ),
  filmic_D_slider( this, "filmic_D", _("toe strength"), 0.5, 0, 1, 0.02, 0.1, 1 ),
  filmic_E_slider( this, "filmic_E", _("toe num."), 0.5, 0, 0.1, 0.002, 0.01, 1 ),
  filmic_F_slider( this, "filmic_F", _("toe den."), 0.5, 0, 1, 0.02, 0.1, 1 ),
  filmic_W_slider( this, "filmic_W", _("lin. white point"), 10, 1, 100, 2, 10, 1 ),
  filmic2_TS_slider( this, "filmic2_TS", _("toe strength"), 0.5, 0, 1, 0.02, 0.1, 1 ),
  filmic2_TL_slider( this, "filmic2_TL", _("toe lenght"), 0.5, 0, 1, 0.02, 0.1, 1 ),
  filmic2_SS_slider( this, "filmic2_SS", _("shoulder strength"), 0.5, 0, 1, 0.02, 0.1, 0.1 ),
  filmic2_SL_slider( this, "filmic2_SL", _("shoulder lenght"), 0.5, 0, 1, 0.02, 0.1, 1 ),
  filmic2_SA_slider( this, "filmic2_SA", _("shoulder angle"), 0.5, 0, 1, 0.02, 0.1, 1 ),
  lumi_blend_frac_slider( this, "lumi_blend_frac", _("preserve colors"), 1, 0, 1, 0.02, 0.1, 1 )
{
  gammaControlsBox.pack_start( gamma_slider, Gtk::PACK_SHRINK );

  filmicControlsBox.pack_start( filmic_A_slider, Gtk::PACK_SHRINK );
  filmicControlsBox.pack_start( filmic_B_slider, Gtk::PACK_SHRINK );
  filmicControlsBox.pack_start( filmic_C_slider, Gtk::PACK_SHRINK );
  filmicControlsBox.pack_start( filmic_D_slider, Gtk::PACK_SHRINK );
  filmicControlsBox.pack_start( filmic_E_slider, Gtk::PACK_SHRINK );
  filmicControlsBox.pack_start( filmic_F_slider, Gtk::PACK_SHRINK );
  filmicControlsBox.pack_start( filmic_W_slider, Gtk::PACK_SHRINK );

  filmic2ControlsBox.pack_start( filmic2_TS_slider, Gtk::PACK_SHRINK );
  filmic2ControlsBox.pack_start( filmic2_TL_slider, Gtk::PACK_SHRINK );
  filmic2ControlsBox.pack_start( filmic2_SS_slider, Gtk::PACK_SHRINK );
  filmic2ControlsBox.pack_start( filmic2_SL_slider, Gtk::PACK_SHRINK );
  filmic2ControlsBox.pack_start( filmic2_SA_slider, Gtk::PACK_SHRINK );

  controlsBox.pack_start( exposureSlider, Gtk::PACK_SHRINK, 10 );
  controlsBox.pack_start( modeSelector, Gtk::PACK_SHRINK, 10 );
  controlsBox.pack_start( controlsBox2, Gtk::PACK_SHRINK, 10 );
  controlsBox.pack_start( separator, Gtk::PACK_SHRINK, 10 );

  controlsBox.pack_start( lumi_blend_frac_slider, Gtk::PACK_SHRINK );


  globalBox.pack_start( controlsBox, Gtk::PACK_SHRINK );
  add_widget( globalBox );
}




void PF::ToneMappingConfigGUI::do_update()
{
  if( get_layer() && get_layer()->get_image() && 
      get_layer()->get_processor() &&
      get_layer()->get_processor()->get_par() ) {

    OpParBase* par = get_layer()->get_processor()->get_par();
    PropertyBase* prop = par->get_property( "method" );
    if( !prop )  return;

    //std::cout<<"PF::ToneMappingConfigGUI::do_update() called."<<std::endl;

    bool need_update = false;

    if( prop->get_enum_value().first == PF::TONE_MAPPING_EXP_GAMMA ) {
      if( gammaControlsBox.get_parent() != &controlsBox2 )
        need_update = true;
    } else if( prop->get_enum_value().first == PF::TONE_MAPPING_FILMIC ) {
      if( filmicControlsBox.get_parent() != &controlsBox2 )
        need_update = true;
    } else if( prop->get_enum_value().first == PF::TONE_MAPPING_FILMIC2 ) {
      if( filmic2ControlsBox.get_parent() != &controlsBox2 )
        need_update = true;
    } else {
      if( (gammaControlsBox.get_parent() == &controlsBox2) ||
          (filmicControlsBox.get_parent() == &controlsBox2) ||
          (filmic2ControlsBox.get_parent() == &controlsBox2) )
        need_update = true;
    }

    if( need_update ) {
      if( gammaControlsBox.get_parent() == &controlsBox2 )
        controlsBox2.remove( gammaControlsBox );
      if( filmicControlsBox.get_parent() == &controlsBox2 )
        controlsBox2.remove( filmicControlsBox );
      if( filmic2ControlsBox.get_parent() == &controlsBox2 )
        controlsBox2.remove( filmic2ControlsBox );

      switch( prop->get_enum_value().first ) {
      case PF::TONE_MAPPING_EXP_GAMMA:
        controlsBox2.pack_start( gammaControlsBox, Gtk::PACK_SHRINK );
        gammaControlsBox.show();
        break;
      case PF::TONE_MAPPING_FILMIC:
        controlsBox2.pack_start( filmicControlsBox, Gtk::PACK_SHRINK );
        filmicControlsBox.show();
        break;
      case PF::TONE_MAPPING_FILMIC2:
        controlsBox2.pack_start( filmic2ControlsBox, Gtk::PACK_SHRINK );
        filmic2ControlsBox.show();
        break;
      }
    }
    controlsBox2.show_all_children();
  }

  OperationConfigGUI::do_update();
}

