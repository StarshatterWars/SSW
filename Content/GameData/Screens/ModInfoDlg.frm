FORM

// +--------------------------------------------------------------------+
//  Project:   Starshatter 4.5
//  File:      ModInfoDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.
// +--------------------------------------------------------------------+

form: {
   rect:       (0,0,440,340)
   back_color: (0,0,0)
   fore_color: (255,255,255)
   font:       Limerick12

   texture:    "Message.pcx"
   margins:    (50,40,48,40)

   layout: {
      x_mins:     (20, 100, 15, 128, 20)
      x_weights:  ( 0,   1,  0,   0,  0)

      y_mins:     (44, 25,  5, 30, 128, 10, 45, 35)
      y_weights:  ( 0,  0,  0,  0,   0,  0,  1,  0)
   }

   defctrl: {
      base_color: (160, 160, 150)
      back_color: ( 92,  92,  92)
      fore_color: (255, 255, 255)
      font:       Verdana
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
      transparent: true
   },

   ctrl: {
      id:            100,
      type:          label,
      cells:         (1,1,3,1)
      text:          "Mod Description"
      font:          Limerick18
      align:         center
   },

   ctrl: {
      id:            101,
      type:          label,
      cells:         (1,3,3,1)
      text:          "Mod Name Here"
      font:          Limerick12
      align:         left
   },

   ctrl: {
      id:            102,
      type:          label,
      cells:         (1,4,1,1)
      text:          "Description goes here",
      font:          Verdana,
      transparent:   true
   },

   ctrl: {
      id:            200,
      type:          image,
      cells:         (3,4,1,1)
      back_color:    (12,12,12)
      border:        true
      style:         0x02,
      picture:       "mod_logo.pcx"
      fixed_width:   128
      fixed_height:  128
      transparent:   false
   },

   ctrl: {
      id:            103,
      type:          label,
      cells:         (1,6,3,1)
      text:          "Copyright Notice",
      font:          Verdana,
      transparent:   true
   },



   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (10,0,0,16)
      transparent:      false
   }

   ctrl: {
      id:               1
      type:             button
      cells:            (3,7,1,1)
      text:             Close
   }
}

