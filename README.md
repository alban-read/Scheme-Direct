# Scheme-Direct

This is a *Direct 2D window* controlled by Chez Scheme.

The idea is to enable simple 2D animations and games for Windows to be written in Scheme.

Functions provided

- Allow shapes to be draw. 

- Images to be loaded and displayed.

- Sounds to be played.


You can see most of the available functions in the base.ss script.

Direct 2D provides hardware acceleration and is much faster than GDI+ allowing for reasonable frame rates.

The small application runs a Scheme script called base.ss when it starts.

The script adds a function to a repeating timer; that function can generate an animation.

**Selfie**

<img src= "https://github.com/alban-read/Scheme-Direct/blob/master/Selfie.PNG">

------


