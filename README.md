# Scheme-Direct

**This is a simple to use *Direct 2D window* controlled by Chez Scheme.**

The idea is to play back 2D animations and games for Windows that are written in Scheme.

Only a few functions are provided:-

- Allow shapes to be drawn. 
- Allows Text to be drawn.
- Images to be loaded, moved and displayed (in quite rapid succession.)

- Sounds to be played.
- The keyboard state to be read.

You can see most of the useful functions defined in the base.ss script.

I intend to add more examples and documentation later.

Direct 2D provides hardware acceleration; typically much faster than GDI+ allowing for reasonable frame rates.

The small application runs a Scheme script called base.ss when it starts.

The base script adds a function to a repeating timer; that function can generate an animation.

The drawing commands all draw onto a bitmap; the bitmap is drawn into the screen by a timer; the whole arrangement is double buffered for smoothness.

Media support

- Images supported are PNG and JPEG.
- MP3 sound clips play well.

I think it should be feasible to create many two dimensional vector or sprite based games using this.

 

**Selfie**

<img src= "https://github.com/alban-read/Scheme-Direct/blob/master/Selfie.PNG">

------


