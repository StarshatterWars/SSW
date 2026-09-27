FORM

// +--------------------------------------------------------------------+
//  Project:   Starshatter 4.5
//  File:      ModDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.
// +--------------------------------------------------------------------+

form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 100,  20, 100, 100, 10),
      x_weights:  ( 0, 0.2, 0.4, 0.2, 0.2,  0),

      y_mins:     (28, 30,  20,  24, 60, 45),
      y_weights:  ( 0,  0,   0,   0,  1,  0)
   },

   // background images:

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame2a,
      cells:         (1,3,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (2,32,40,32)
      hide_partial:  false
   },

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame2b,
      cells:         (3,3,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (0,40,40,32)
      hide_partial:  false
   },

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Options",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,3,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   },

   // tabs:

   ctrl: {
      id:            900
      type:          panel
      transparent:   true
      cells:         (1,3,4,1)
      hide_partial:  false

      layout: {
         x_mins:     (100, 100, 100, 100, 100, 0),
         x_weights:  (0.2, 0.2, 0.2, 0.2, 0.2, 1),

         y_mins:     (24),
         y_weights:  ( 1)
      }
   }

   defctrl: {
      align:            left,
      font:             Limerick12,
      fore_color:       (255, 255, 255),
      standard_image:   BlueTab_0,
      activated_image:  BlueTab_1,
      sticky:           true,
      bevel_width:      6,
      margins:          (8,8,0,0),
      cell_insets:      (0,4,0,0)
   },

   ctrl: {
      id:            901
      pid:           900
      type:          button
      text:          Video
      cells:         (0,0,1,1)
   }

   ctrl: {
      id:            902
      pid:           900
      type:          button
      text:          Audio
      cells:         (1,0,1,1)
   }

   ctrl: {
      id:            903
      pid:           900
      type:          button
      text:          Controls
      cells:         (2,0,1,1)
   }

   ctrl: {
      id:            904
      pid:           900
      type:          button
      text:          Gameplay
      cells:         (3,0,1,1)
   }

   ctrl: {
      id:            905
      pid:           900
      type:          button
      text:          "Mod Config"
      cells:         (4,0,1,1)
   }

   // main panel:

   ctrl: {
      id:               300
      type:             panel
      transparent:      true

      texture:          Panel
      margins:          (12,12,12,0),

      cells:            (1,4,4,2)
      cell_insets:      (10,10,10,54)

      layout: {
         x_mins:     (100, 100, 30, 30,  20, 100, 100, 30, 30)
         x_weights:  (  3,   3,  0,  0,   0,   3,   3,  0,  0)

         y_mins:     (  3,  84,  40,  80,  30,  10)
         y_weights:  (  0,   0,   0,   1,   0,   0)
      }
   }

   defctrl: {
      fore_color:       (255,255,255)
      font:             Verdana,
      standard_image:   ""
      activated_image:  ""
      align:            left
      sticky:           false
      transparent:      true
   },

   ctrl: {
      id:            200
      pid:           300
      type:          text
      cells:         (0,1,9,1)
      cell_insets:   (0,0,0,0)
      transparent:   false
      scroll_bar:    0

      texture:          Panel
      margins:          (12,12,12,0),

      text: "<font Limerick12><color ffffff>INSTRUCTIONS:\n"
            "<font Verdana>\n\t* Download mods from the Internet, "
            "and save them in any subfolder of the Starshatter install folder.\n\t* Mods are initially disabled "
            "and are shown in the DISABLED MODS list below.\n\t* To enable a mod, select it and click the "
            "ENABLE button.\n\t* To disable an enabled mod, select it in the ENABLED MODS list and click the "
            "DISABLE button."
   },

   ctrl: {
      id:            101
      pid:           300
      type:          label
      cells:         (0,2,4,1)
      cell_insets:   (0,0,20,0)
      text:          "Disabled Mods"
      font:          Limerick12
      fore_color:    (255,255,255)
      back_color:    (32,32,32)
      style:         0x40
      transparent:   true
      align:         center
   },

   ctrl: {
      id:            102
      pid:           300
      type:          label
      cells:         (5,2,4,1)
      cell_insets:   (0,0,20,0)
      text:          "Enabled Mods"
      font:          Limerick12
      fore_color:    (255,255,255)
      back_color:    (32,32,32)
      style:         0x40
      transparent:   true
      align:         center
   },

   ctrl: {
      id:            201
      pid:           300
      type:          list,
      cells:         (0,3,4,1)
      cell_insets:   (0,0,0,10)
      transparent:   false

      texture:          Panel
      margins:          (12,12,12,0)

      column:     {
         title:   Disabled,
         width:   248,
         align:   left,
         sort:    0 }
   },

   ctrl: {
      id:            202
      pid:           300
      type:          list,
      cells:         (5,3,4,1)
      cell_insets:   (0,0,0,10)
      transparent:   false

      texture:          Panel
      margins:          (12,12,12,0)

      column:     {
         title:   Enabled,
         width:   248,
         align:   left,
         sort:    0 }
   },

   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,10,0,11)
   }

   ctrl: {
      id:            301
      pid:           300
      type:          button
      cells:         (0,4,1,1)
      text:          "Enable >>"
   },

   ctrl: {
      id:            302
      pid:           300
      type:          button
      cells:         (5,4,1,1)
      text:          "Disable <<"
   },

   defctrl: {
      align:            center
      standard_image:   Button17x17_0
      activated_image:  Button17x17_1
      transition_image: Button17x17_2
      cell_insets:      (10,0,0,0)
      fixed_width:      19
      fixed_height:     19
   }

   ctrl: {
      id:            303
      pid:           300
      type:          button
      cells:         (7,4,1,1)
      text:          "^"
   },

   ctrl: {
      id:            304
      pid:           300
      type:          button
      cells:         (8,4,1,1)
      text:          "v"
   },


   // apply and cancel buttons:

   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,10,0,0)
      fixed_width:      0
   },

   ctrl: {
      id:            1
      type:          button
      text:          Apply
      cells:         (3,5,1,1)
   },

   ctrl: {
      id:            2
      type:          button
      text:          Cancel
      cells:         (4,5,1,1),
   },

}
