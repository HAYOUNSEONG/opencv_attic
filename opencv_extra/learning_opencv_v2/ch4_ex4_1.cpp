// An example program in which the
// user can draw boxes on the screen.
//
/* License:
   Oct. 3, 2008
   Right to use this code in any way you want without warrenty, support or any guarentee of it working.

   BOOK: It would be nice if you cited it:
   Learning OpenCV: Computer Vision with the OpenCV Library
     by Gary Bradski and Adrian Kaehler
     Published by O'Reilly Media, October 3, 2008
 
   AVAILABLE AT: 
     http://www.amazon.com/Learning-OpenCV-Computer-Vision-Library/dp/0596516134
     Or: http://oreilly.com/catalog/9780596516130/
     ISBN-10: 0596516134 or: ISBN-13: 978-0596516130    

   OTHER OPENCV SITES:
   * The source code is on sourceforge at:
     http://sourceforge.net/projects/opencvlibrary/
   * The OpenCV wiki page (As of Oct 1, 2008 this is down for changing over servers, but should come back):
     http://opencvlibrary.sourceforge.net/
   * An active user group is at:
     http://tech.groups.yahoo.com/group/OpenCV/
   * The minutes of weekly OpenCV development meetings are at:
     http://pr.willowgarage.com/wiki/OpenCV
*/
#include <opencv2/opencv.hpp>

using namespace cv;

// Define our callback which we will install for
// mouse events.
//
void my_mouse_callback(
   int event, int x, int y, int flags, void* param 
);
 
Rect box;
bool drawing_box = false;
 
// A litte subroutine to draw a box onto an image
//
void draw_box( Mat& img, Rect box ) {
  rectangle (
    img, 
    box.tl(),
    box.br(),
    Scalar(0x00,0x00,0xff)    /* red */
  );
}
 
int main( int argc, char* argv[] ) {
  
  box = Rect(-1,-1,0,0);

  Mat image(200, 200, CV_8UC3), temp;
  image = Scalar::all(0);
  
  namedWindow( "Box Example" );
 
  // Here is the crucial moment that we actually install
  // the callback.  Note that we set the value ‘param’ to
  // be the image we are working with so that the callback
  // will have the image to edit.
  //
  setMouseCallback( 
    "Box Example", 
    my_mouse_callback, 
    (void*)&image 
  );
 
  // The main program loop.  Here we copy the working image
  // to the ‘temp’ image, and if the user is drawing, then
  // put the currently contemplated box onto that temp image.
  // display the temp image, and wait 15ms for a keystroke,
  // then repeat…
  //
  for(;;) {
 
	image.copyTo(temp);
    if( drawing_box ) draw_box( temp, box ); 
    imshow( "Box Example", temp );
 
    if( waitKey( 15 )==27 ) break;
  }
	
  return 0;
}
 
// This is our mouse callback.  If the user
// presses the left button, we start a box.
// when the user releases that button, then we
// add the box to the current image.  When the
// mouse is dragged (with the button down) we 
// resize the box.
//
void my_mouse_callback(
   int event, int x, int y, int flags, void* param )
{
 
  Mat& image = *(Mat*) param;

  switch( event ) {
    case CV_EVENT_MOUSEMOVE: {
      if( drawing_box ) {
        box.width  = x-box.x;
        box.height = y-box.y;
      }
    }
    break;
    case CV_EVENT_LBUTTONDOWN: {
      drawing_box = true;
      box = Rect( x, y, 0, 0 );
    }
    break;   
    case CV_EVENT_LBUTTONUP: {
      drawing_box = false; 
      if( box.width<0  ) { 
        box.x+=box.width;  
        box.width *=-1; 
      }
      if( box.height<0 ) { 
        box.y+=box.height; 
        box.height*=-1; 
      }
      draw_box( image, box );
    }
    break;   
  }
}