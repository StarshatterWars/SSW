FORM

// +--------------------------------------------------------------------+
//  Project:   Starshatter 4.5
//  File:      KeyDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.
// +--------------------------------------------------------------------+

form: {
   rect:       (0,0,440,380),
   back_color: (0,0,0),
   fore_color: (255,255,255),
   font:       Limerick12,

   texture:    "Message.pcx",
   margins:    (50,40,48,40),

   layout: {
      x_mins:     (30, 100, 100, 100, 30),
      x_weights:  ( 0,   1,   1,   1,  0),

      y_mins:     (44, 30, 25, 25, 25, 25, 25, 30, 75, 10, 35),
      y_weights:  ( 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0)
   },

   defctrl: {
      align:         center
      base_color:    ( 92,  92,  92)
      back_color:    ( 92,  92,  92)
      fore_color:    (255, 255, 255)
      font:          Limerick12
      bevel_width:   0
      bevel_depth:   128
      border:        true
      border_color:  (0,0,0)
      transparent:   true
   },

   ctrl: {
      id:            100,
      type:          label,
      cells:         (1,1,3,1)
      text:          "Joystick Axis Setup",
   },


   ctrl: {
      id:            501,
      type:          label,
      cells:         (1,2,1,1)
      text:          "Axis",
   },

   ctrl: {
      id:            502,
      type:          label,
      cells:         (2,2,1,1)
      text:          "Select",
   },

   ctrl: {
      id:            503,
      type:          label,
      cells:         (3,2,1,1)
      text:          "Invert",
   },

   defctrl: {
      align:         left
      font:          Verdana
   }

   ctrl: {
      id:            101
      type:          label
      cells:         (1,3,1,1)
      text:          "X Axis:"
   }

   ctrl: {
      id:            102
      type:          label
      cells:         (1,4,1,1)
      text:          "Y Axis:"
   }

   ctrl: {
      id:            103
      type:          label
      cells:         (1,5,1,1)
      text:          "Rudder:"
   }

   ctrl: {
      id:            104
      type:          label
      cells:         (1,6,1,1)
      text:          "Throttle:"
   }

   ctrl: {
      id:            11
      type:          label
      cells:         (1,8,3,1)
      text:          "Click an axis button and then move the controller to bind a new axis.  "
                     "You can also invert any axis by clicking the 'Invert' checkbox to the "
                     "right of the select button.\n\n"
                     "Then click Apply to save the new bindings, or Cancel to return to the "
                     "original bindings."
   }

   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,0,0,6)
      sticky:           true
      transparent:      false
   }

   ctrl: {
      id:               201
      type:             button
      cells:            (2,3,1,1)
      text:             Select
   },

   ctrl: {
      id:               202
      type:             button
      cells:            (2,4,1,1)
      text:             Select
   },

   ctrl: {
      id:               203
      type:             button
      cells:            (2,5,1,1)
      text:             Select
   },

   ctrl: {
      id:               204
      type:             button
      cells:            (2,6,1,1)
      text:             Select
   }

   defctrl: {
      cell_insets:      (55,0,0,0)
      fixed_width:      16
      fixed_height:     16

      standard_image:   Checkbox_0
      activated_image:  Checkbox_2
      transition_image: Checkbox_1
      sticky:           true
   }

   ctrl: {
      id:            301
      type:          button
      cells:         (3,3,1,1)
   }

   ctrl: {
      id:            302
      type:          button
      cells:         (3,4,1,1)
   }

   ctrl: {
      id:            303
      type:          button
      cells:         (3,5,1,1)
   }

   ctrl: {
      id:            304
      type:          button
      cells:         (3,6,1,1)
   }


   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      fixed_width:      0
      fixed_height:     0
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (10,0,0,16)
      transparent:      false
   }

   ctrl: {
      id:            1,
      type:          button,
      text:          "Apply",
      cells:         (2,10,1,1)
   }

   ctrl: {
      id:            2,
      type:          button,
      text:          "Cancel",
      cells:         (3,10,1,1),
   }
}
