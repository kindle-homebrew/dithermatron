//====================================================
// dithermatron 1.1a - kindle eink dynamic dither demo
// Copyright (C) 2012 by geekmaster, with MIT license:
// http://www.opensource.org/licenses/mit-license.php
//----------------------------------------------------
//  The speed is limited by the eink device drivers.
//  Newer kindle models are faster, but need delays.
//  This was tested on DX,DXG,K3,K4(Mini),K5(Touch).
//----------------------------------------------------

#include <stdio.h>      // printf
#include <stdlib.h>    // malloc, free
#include <string.h>   // memset, memcpy
#include <unistd.h>  // usleep
#include <fcntl.h>  // open, close, write
#include <time.h>  // time
#include <sys/mman.h>   // mmap, munmap
#include <sys/ioctl.h> // ioctl
#include <linux/fb.h> // screeninfo

#define ED6 570000      // k4 eink delay best quality
#define ED5 500000     // k4 eink delay good quality
#define ED4 230000    // k4 eink delay K3 speed
#define ED3 100000   // k4 eink delay okay
#define ED2 80000   // k4 eink delay fast
#define ED1 0      // k4 eink delay none, bad
#define K4DLY ED2 // k4 eink delay

enum eupd_op { EUPD_OPEN,EUPD_CLOSE,EUPD_UPDATE };
typedef unsigned char u8;
typedef unsigned int u32;

// function prototypes
void inline setpx(int,int,int);
void box(int,int,int,int);
void line(int,int,int,int,int);
void circle(int,int,int);
int eupdate(int);

// global vars
u8 *fb0=NULL;   // framebuffer pointer
int fdFB=0;    // fb0 file descriptor
u32 fs=0;     // fb0 stride
u32 MX=0;    // xres (visible)
u32 MY=0;   // yres (visible)
u8 blk=0;  // black
u8 wht=0; // white
u8 pb=0; // pixel bits

//===============================================
// dithermatron - kindle eink dynamic dither demo
// This works on all kindle eink models.   Enjoy!
//-----------------------------------------------
void dithermatron(void) {
    int x,y;
    struct fb_var_screeninfo screeninfo;
    fdFB=open("/dev/fb0",O_RDWR); // eink framebuffer

// calculate model-specific vars
    ioctl(fdFB,FBIOGET_VSCREENINFO,&screeninfo);
    MX=screeninfo.xres;  // max X+1
    MY=screeninfo.yres; // max Y+1
    pb=screeninfo.bits_per_pixel;     // pixel bits
    fs=screeninfo.xres_virtual*pb/8; // fb0 stride
    blk=pb/8-1; // black
    wht=~blk;  // white
    fb0=(u8 *)mmap(0,MY*fs,PROT_READ|PROT_WRITE,MAP_SHARED,fdFB,0); // map fb0
    eupdate(EUPD_OPEN); // open fb0 update proc

// do dithered gray demo
    int c=0,px1=MX/2,py1=MY/2,vx1=1,vy1=2,px2=px1,py2=py1,vx2=3,vy2=1;
    int dx,dy,cc=31,cu,cl=3;
    for (cu=0;cu<20000;cu++) {
        if (0==cu%3000) { // periodic background display
          for (y=0; y<=MY/2; y++) {
            for (x=0; x<=MX/2; x++) {
                dx=MX/2-x; dy=MY/2-y; c=65-(dx*dx+dy*dy)*65/(MX*220);
                setpx(x,y,c); setpx(MX-x,y,c);
                setpx(x,MY-y,c); setpx(MX-x,MY-y,c);
            }
          }
        }
        box(px1,py1,80,0); box(px1,py1,81,64);
        box(px1,py1,82,64); box(px1,py1,83,cc);
        circle(px2,py2,50); circle(px2,py2,51); circle(px2,py2,52);
        circle(px1+80,py1+80,20); circle(px1+80,py1-80,20);
        circle(px1-80,py1+80,20); circle(px1-80,py1-80,20);
        px1+=vx1; if (px1>MX-110 || px1<110) vx1=-vx1;
        py1+=vy1; if (py1>MY-110 || py1<110) { vy1=-vy1; cl++; }
        px2+=vx2; if (px2>MX-60 || px2<60) vx2=-vx2;
        py2+=vy2; if (py2>MY-60 || py2<60) { vy2=-vy2; cl++; }
        if (0==cu%cl) { eupdate(EUPD_UPDATE); } // update display
        cc=(cc+1)%65; // cycle big box color
    }

// cleanup - close and free resources
    eupdate(EUPD_UPDATE);    // update display
    eupdate(EUPD_CLOSE);    // close fb0 update proc port
    munmap(fb0,fs*(MY+1)); // unmap fb0
    close(fdFB);          // close fb0
}

//===============================
// eupdate - eink update display
// op (open, close, update)
//-------------------------------
int eupdate(int op) {
    static int fdUpdate=-1;
    if (EUPD_OPEN==op) { fdUpdate=open("/proc/eink_fb/update_display",O_WRONLY);
    } else if (EUPD_CLOSE==op) { close(fdUpdate);
    } else if (EUPD_UPDATE==op) {
        if (-1==fdUpdate) { system("eips ''"); usleep(K4DLY);
        } else { write(fdUpdate,"1\n",2); }
    } else { return -1; }
    return fdUpdate;
}

//========================================
// setpx - draw pixel using ordered dither
// x,y: screen coordinates, c: color(0-64).
// (This works on all eink kindle models.)
//----------------------------------------
void inline setpx(int x,int y,int c) {
    static int dt[64] = { 1,33,9,41,3,35,11,43,49,17,57,25,51,19,59,27,13,45,5,
    37,15,47,7,39,61,29,53,21,63,31,55,23,4,36,12,44,2,34,10,42,52,20,60,28,50,
    18,58,26,16,48,8,40,14,46,6,38,64,32,56,24,62,30,54,22 }; // dither table
    fb0[pb*x/8+fs*y]=((128&(c-dt[(7&x)+8*(7&y)]))/128*(blk&(240*(1&~x)|
        15*(1&x)|fb0[pb*x/8+fs*y])))|((128&(dt[(7&x)+8*(7&y)]-c))/128*wht|
        (blk&((240*(1&x)|15*(1&~x))&fb0[pb*x/8+fs*y]))); // geekmaster formula 42
}

//======================
// box - simple box draw
//----------------------
void box(int x,int y,int d,int c) {
    int i;
    for (i=0;i<d;++i) {
        setpx(x+i,y+d,c); setpx(x+i,y-d,c); setpx(x-i,y+d,c); setpx(x-i,y-d,c);
        setpx(x+d,y+i,c); setpx(x+d,y-i,c); setpx(x-d,y+i,c); setpx(x-d,y-i,c);
    }
}
//==================================
// line - Bresenham's line algorithm
//----------------------------------
void line(int x0,int y0,int x1,int y1,int c) {
    int dx,ny,sx,sy,e;
    if (x1>x0) { dx=x1-x0; sx=1; } else { dx=x0-x1; sx=-1; }
    if (y1>y0) { ny=y0-y1; sy=1; } else { ny=y1-y0; sy=-1; }
    e=dx+ny;
    for (;;) { setpx(x0,y0,c);
        if (x0==x1&&y0==y1) { break; }
        if (e+e>ny) { e+=ny; x0+=sx; }
        if (e+e<dx) { e+=dx; y0+=sy; }
    }
}

//==============================================
// circle - optimized midpoint circle algorithm
//----------------------------------------------
void circle(int cx,int cy,int r) {
    int e=-r,x=r,y=0;
    while (x>y) {
        setpx(cx+y,cy-x,64); setpx(cx+x,cy-y,40);
        setpx(cx+x,cy+y,24); setpx(cx+y,cy+x,8);
        setpx(cx-y,cy+x,0); setpx(cx-x,cy+y,16);
        setpx(cx-x,cy-y,32); setpx(cx-y,cy-x,48);
        e+=y; y++; e+=y;
        if (e>0) { e-=x; x-=1; e-=x; }
    }
}

//==================
// main - start here
//------------------
int main(void) {
    dithermatron(); // do the dithermatron demo :D
    return 0;
}
