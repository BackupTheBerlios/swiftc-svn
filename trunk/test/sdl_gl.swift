
class SDL_Surface
    int dummy_
end

class SDL_VideoInfo
    uint hw_available #1;	# Flag: Can you create hardware surfaces? 
    uint wm_available #1;	# Flag: Can you talk to a window manager?
    uint UnusedBits1  #6;
    uint UnusedBits2  #1;
    uint blit_hw      #1;	# Flag: Accelerated blits HW --> HW
    uint blit_hw_CC   #1;	# Flag: Accelerated blits with Colorkey
    uint blit_hw_A    #1;	# Flag: Accelerated blits with Alpha
    uint blit_sw      #1;	# Flag: Accelerated blits SW --> HW
    uint blit_sw_CC   #1;	# Flag: Accelerated blits with Colorkey
    uint blit_sw_A    #1;	# Flag: Accelerated blits with Alpha
    uint blit_fill    #1;	# Flag: Accelerated color fill
    uint UnusedBits3  #16;
    uint video_mem       	# The total amount of video memory (in K)
    ptr{int} dummy_         # SDL_PixelFormat *vfmt;	# Value: The format of the video surface 
    int    current_w 	    # Value: The current video mode width
    int    current_h 	    # Value: The current video mode height
end

class App
    # This is our SDL surface
    ptr{SDL_Surface} surface_

    # rotational vars for the triangle and quad, respectively 
    real rtri_
    real rquad_

    # function to release/destroy our resources and restoring the old desktop */
    routine quit(int returnCode)
        # clean up the window
        c_call SDL_Quit()

        # and exit appropriately
        c_call exit(returnCode)
    end

    # function to reset our viewport after a window resize
    routine resizeWindow(int width, int height)
        # Height / width ration 
    
        # protect against a divide by zero 
        if (height == 0)
            height = 1
        end


        # setup our viewport 
        c_call glViewport(0, 0, width, height)

        # change to the projection matrix and set our viewing volume

        uint GL_PROJECTION = 5889u
        c_call glMatrixMode(GL_PROJECTION)
        c_call glLoadIdentity()

        # set our perspective 
        real64 ratio = width:to_real64() / height:to_real64()
        c_call gluPerspective(45.0q, ratio, 0.1q, 100.0q)

        # make sure we're chaning the model view and not the projection 
        uint GL_MODELVIEW = 5890u
        c_call glMatrixMode(GL_MODELVIEW)

        # reset The View 
        c_call glLoadIdentity()
    end

    # general OpenGL initialization function 
    routine initGL()
        # Enable smooth shading 
        uint GL_SMOOTH = 7425u
        c_call glShadeModel(GL_SMOOTH)

        # Set the background black 
        c_call glClearColor(0.0, 0.0, 0.0, 0.0)

        # Depth buffer setup 
        c_call glClearDepth(1.0)

        # Enables Depth Testing 
        uint GL_DEPTH_TEST = 2929u
        c_call glEnable(GL_DEPTH_TEST)

        # The Type Of Depth Test To Do 
        uint GL_LEQUAL = 515u
        c_call glDepthFunc(GL_LEQUAL)

        # Really Nice Perspective Calculations 
        uint GL_PERSPECTIVE_CORRECTION_HINT = 3152u
        uint GL_NICEST = 4354u
        c_call glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST)
    end

    # Here goes our drawing code 
    writer drawGLScene()
        # Clear The Screen And The Depth Buffer 
        c_call glClear() # TODO GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT )

        # Move Left 1.5 Units And Into The Screen 6.0 
        c_call glLoadIdentity()
        c_call glTranslatef(-1.5, 0.0, -6.0)

        # Rotate The Triangle On The Y axis
        c_call glRotatef(.rtri_, 0.0, 1.0, 0.0)

        uint GL_TRIANGLES = 4u
        c_call glBegin(GL_TRIANGLES)        # Drawing Using Triangles      
        c_call glColor3f(  1.0,  0.0,  0.0) # Red                          
        c_call glVertex3f( 0.0,  1.0,  0.0) # Top Of Triangle (Front)      
        c_call glColor3f(  0.0,  1.0,  0.0) # Green                        
        c_call glVertex3f(-1.0, -1.0,  1.0) # Left Of Triangle (Front)     
        c_call glColor3f(  0.0,  0.0,  1.0) # Blue                         
        c_call glVertex3f( 1.0, -1.0,  1.0) # Right Of Triangle (Front)    

        c_call glColor3f(  1.0,  0.0,  0.0) # Red                          
        c_call glVertex3f( 0.0,  1.0,  0.0) # Top Of Triangle (Right)      
        c_call glColor3f(  0.0,  0.0,  1.0) # Blue                         
        c_call glVertex3f( 1.0, -1.0,  1.0) # Left Of Triangle (Right)     
        c_call glColor3f(  0.0,  1.0,  0.0) # Green                        
        c_call glVertex3f( 1.0, -1.0, -1.0) # Right Of Triangle (Right)    

        c_call glColor3f(  1.0,  0.0,  0.0) # Red                          
        c_call glVertex3f( 0.0,  1.0,  0.0) # Top Of Triangle (Back)       
        c_call glColor3f(  0.0,  1.0,  0.0) # Green                        
        c_call glVertex3f( 1.0, -1.0, -1.0) # Left Of Triangle (Back)      
        c_call glColor3f(  0.0,  0.0,  1.0) # Blue                         
        c_call glVertex3f(-1.0, -1.0, -1.0) # Right Of Triangle (Back)     

        c_call glColor3f(  1.0,  0.0,  0.0) # Red                          
        c_call glVertex3f( 0.0,  1.0,  0.0) # Top Of Triangle (Left)       
        c_call glColor3f(  0.0,  0.0,  1.0) # Blue                         
        c_call glVertex3f(-1.0, -1.0, -1.0) # Left Of Triangle (Left)      
        c_call glColor3f(  0.0,  1.0,  0.0) # Green                        
        c_call glVertex3f(-1.0, -1.0,  1.0) # Right Of Triangle (Left)     
        c_call glEnd()                      # Finished Drawing The Triangle

        # Move Right 3 Units 
        c_call glLoadIdentity()
        c_call glTranslatef(1.5, 0.0, -6.0)

        # Rotate The Quad On The X axis
        c_call glRotatef(.rquad_, 1.0, 0.0, 0.0)

        # Set The Color To Blue One Time Only 
        c_call glColor3f(0.5, 0.5, 1.0)

        uint GL_QUADS = 7u
        c_call glBegin(GL_QUADS)            # Draw A Quad                     
        c_call glColor3f(  0.0,  1.0,  0.0) # Set The Color To Green          
        c_call glVertex3f( 1.0,  1.0, -1.0) # Top Right Of The Quad (Top)     
        c_call glVertex3f(-1.0,  1.0, -1.0) # Top Left Of The Quad (Top)      
        c_call glVertex3f(-1.0,  1.0,  1.0) # Bottom Left Of The Quad (Top)   
        c_call glVertex3f( 1.0,  1.0,  1.0) # Bottom Right Of The Quad (Top)  

        c_call glColor3f(  1.0,  0.5,  0.0) # Set The Color To Orange         
        c_call glVertex3f( 1.0, -1.0,  1.0) # Top Right Of The Quad (Botm)    
        c_call glVertex3f(-1.0, -1.0,  1.0) # Top Left Of The Quad (Botm)     
        c_call glVertex3f(-1.0, -1.0, -1.0) # Bottom Left Of The Quad (Botm)  
        c_call glVertex3f( 1.0, -1.0, -1.0) # Bottom Right Of The Quad (Botm) 

        c_call glColor3f(  1.0,  0.0,  0.0) # Set The Color To Red            
        c_call glVertex3f( 1.0,  1.0,  1.0) # Top Right Of The Quad (Front)   
        c_call glVertex3f(-1.0,  1.0,  1.0) # Top Left Of The Quad (Front)    
        c_call glVertex3f(-1.0, -1.0,  1.0) # Bottom Left Of The Quad (Front) 
        c_call glVertex3f( 1.0, -1.0,  1.0) # Bottom Right Of The Quad (Front)

        c_call glColor3f(  1.0,  1.0,  0.0) # Set The Color To Yellow         
        c_call glVertex3f( 1.0, -1.0, -1.0) # Bottom Left Of The Quad (Back)  
        c_call glVertex3f(-1.0, -1.0, -1.0) # Bottom Right Of The Quad (Back) 
        c_call glVertex3f(-1.0,  1.0, -1.0) # Top Right Of The Quad (Back)    
        c_call glVertex3f( 1.0,  1.0, -1.0) # Top Left Of The Quad (Back)     

        c_call glColor3f(  0.0,  0.0,  1.0) # Set The Color To Blue           
        c_call glVertex3f(-1.0,  1.0,  1.0) # Top Right Of The Quad (Left)    
        c_call glVertex3f(-1.0,  1.0, -1.0) # Top Left Of The Quad (Left)     
        c_call glVertex3f(-1.0, -1.0, -1.0) # Bottom Left Of The Quad (Left)  
        c_call glVertex3f(-1.0, -1.0,  1.0) # Bottom Right Of The Quad (Left) 

        c_call glColor3f(  1.0,  0.0,  1.0) # Set The Color To Violet         
        c_call glVertex3f( 1.0,  1.0, -1.0) # Top Right Of The Quad (Right)   
        c_call glVertex3f( 1.0,  1.0,  1.0) # Top Left Of The Quad (Right)    
        c_call glVertex3f( 1.0, -1.0,  1.0) # Bottom Left Of The Quad (Right) 
        c_call glVertex3f( 1.0, -1.0, -1.0) # Bottom Right Of The Quad (Right)
        c_call glEnd( )                     # Done Drawing The Quad           

        # Draw it to the screen 
        c_call SDL_GL_SwapBuffers()

        # Increase The Rotation Variable For The Triangle
        .rtri_ = .rtri_ + 0.2
        # Decrease The Rotation Variable For The Quad
        .rquad_ = .rquad_ + 0.15
    end

    routine main() -> int result
        App app

        # Flags to pass to SDL_SetVideoMode 
        int videoFlags
        # main loop variable 
        bool done = false

        # this holds some info about our display 
        ptr{const SDL_VideoInfo} videoInfo
        # whether or not the window is active 

        # initialize SDL 
        uint SDL_INIT_VIDEO = 20u
        c_call int SDL_Init(SDL_INIT_VIDEO)

        # Fetch the video info 
        videoInfo = c_call ptr{SDL_VideoInfo} SDL_GetVideoInfo()

        # the flags to pass to SDL_SetVideoMode 
        # videoFlags  = SDL_OPENGL          /* Enable OpenGL in SDL */
        # videoFlags |= SDL_GL_DOUBLEBUFFER /* Enable double buffering */
        # videoFlags |= SDL_HWPALETTE       /* Store the palette in hardware */
        # videoFlags |= SDL_RESIZABLE       /* Enable window resizing */

        # This checks to see if surfaces can be stored in memory 
        # if ( videoInfo.hw_available )
        #     videoFlags |= SDL_HWSURFACE
        # else
        #     videoFlags |= SDL_SWSURFACE
        # end

        # This checks if hardware blits can be done 
        # if ( videoInfo.blit_hw )
        #     videoFlags |= SDL_HWACCEL

        # Sets up OpenGL double buffering 
        int SDL_GL_DOUBLEBUFFER = 5
        c_call SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 )

        # get a SDL surface
        .surface_ = c_call ptr{SDL_Surface} SDL_SetVideoMode(640, 480, 16, videoFlags)

        # initialize OpenGL 
        ::initGL()

        # resize the initial window 
        ::resizeWindow(640, 480)

        int counter = 0

        while counter < 10000
            app.drawGLScene()
            counter = counter + 1
        end

        # clean ourselves up and exit 
        ::quit(0)

        # should never get here 
        result = 0
    end
end
