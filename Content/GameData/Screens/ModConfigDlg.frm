FORM

// +--------------------------------------------------------------------+
//  Project:   STARSHATTER
//  File:      ModConfigDlg.frm
//
//  John DiCamillo
//  Copyright © 1997-2002. All Rights Reserved.
// +--------------------------------------------------------------------+

form: {
   rect:       (0, 0, 640, 480),
   screen_width:  640,
   screen_height: 480,

   base_color: (  0,   0,   0),
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),
   font:       GUI,

   texture:    "Demo_640.pcx",

   defctrl: {
      base_color: (  0,   0,   0),
      back_color: ( 62, 106, 151),
      fore_color: (255, 255, 255),
      font:       GUI,
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
   },

   ctrl: {
      id:            100,
      type:          label,
      rect:          (0, 120, 640, 20),
      text:          "MOD Configuration",
      transparent:   true,
      align:         center
   }

   ctrl: {
      id:            101,
      type:          label,
      rect:          (70, 140, 240, 20),
      text:          "Disabled Mods",
      transparent:   true,
      align:         center
   }

   ctrl: {
      id:            102,
      type:          label,
      rect:          (330, 140, 240, 20),
      text:          "Enabled Mods",
      transparent:   true,
      align:         center
   }

   ctrl: {
      id:            201,
      type:          list,
      rect:          (70, 170, 240, 180),
      back_color:    (21, 21, 21),
      font:          GUIsmall,
      style:         0x02,
      scroll_bar:    2,

      column:     {
         title:   Disabled,
         width:   218,
         align:   left,
         sort:    0 }
   },

   ctrl: {
      id:            202,
      type:          list,
      rect:          (330, 170, 240, 180),
      back_color:    (21, 21, 21),
      font:          GUIsmall,
      style:         0x02,
      scroll_bar:    2,

      column:     {
         title:   Enabled,
         width:   218,
         align:   left,
         sort:    0 }
   },


   ctrl: {
      id:            301,
      type:          button,
      rect:          (70, 360, 130, 25),
      text:          "Enable"
   },

   ctrl: {
      id:            302,
      type:          button,
      rect:          (330, 360, 130, 25),
      text:          "Disable"
   },

   ctrl: {
      id:            303,
      type:          button,
      rect:          (515, 360, 26, 25),
      text:          "^",
      font:          GUIsmall
   },

   ctrl: {
      id:            304,
      type:          button,
      rect:          (545, 360, 26, 25),
      text:          "v",
      font:          GUIsmall
   },

   ctrl: {
      id:            1,
      type:          button,
      rect:          (300, 420, 130, 25),
      text:          "Accept"
   },

   ctrl: {
      id:            2,
      type:          button,
      rect:          (440, 420, 130, 25),
      text:          "Cancel"
   }
}

