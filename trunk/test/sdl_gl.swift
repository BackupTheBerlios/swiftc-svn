
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define SCREEN_BPP     16

#define TRUE  1
#define FALSE 0

class SDL_Surface
    int dummy_
end

class SDL

    # This is our SDL surface 
    ptr{SDL_Surface} surface

    # function to release/destroy our resources and restoring the old desktop 
    routine quit(int returnCode)
        # clean up the window */
        c_call SDL_Quit()

        # and exit appropriately */
        exit( returnCode )
    end

    # function to reset our viewport after a window resize */
    routine int resizeWindow(int width, int height)
        # Height / width ratio
        real ratio
    
        # Protect against a divide by zero
        if ( height == 0 )
            height = 1

            ratio = ( GLfloat )width / ( GLfloat )height

            # Setup our viewport.
            glViewport( 0, 0, ( GLsizei )width, ( GLsizei )height )

            # change to the projection matrix and set our viewing volume. 
            glMatrixMode( GL_PROJECTION )
            glLoadIdentity( )

            # Set our perspective 
            gluPerspective( 45.0f, ratio, 0.1f, 100.0f )

            # Make sure we're chaning the model view and not the projection 
            glMatrixMode( GL_MODELVIEW )

            # Reset The View 
            glLoadIdentity( )

            return( TRUE )
        end
    end

    # general OpenGL initialization function 
    routine initGL()
        # Enable smooth shading 
        c_call glShadeModel( GL_SMOOTH )

        # Set the background black 
        c_call glClearColor( 0.0, 0.0, 0.0, 0.0 )

        # Depth buffer setup 
        c_call glClearDepth( 1.0 )

        # Enables Depth Testing 
        c_call glEnable( GL_DEPTH_TEST )

        # The Type Of Depth Test To Do 
        c_call glDepthFunc( GL_LEQUAL )

        # Really Nice Perspective Calculations 
        c_call glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST )
    end

    # Here goes our drawing code 
    int drawGLScene()
        # rotational vars for the triangle and quad, respectively 
        real rtri 
        real rquad

        # These are to calculate our fps 
        int t0     = 0
        int frames = 0

        # Clear The Screen And The Depth Buffer 
        c_call glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT )

        # Move Left 1.5 Units And Into The Screen 6.0 
        c_call glLoadIdentity()
        c_call glTranslatef( -1.5, 0.0, -6.0 )

        # Rotate The Triangle On The Y axis ( NEW ) 
        c_call glRotatef(rtri, 0.0, 1.0, 0.0)

        c_call glBegin( GL_TRIANGLES )        # Drawing Using Triangles       
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
        c_call glEnd()                        # Finished Drawing The Triangle 

        # Move Right 3 Units 
        c_call glLoadIdentity( )
        c_call glTranslatef( 1.5, 0.0, -6.0 )

        # Rotate The Quad On The X axis ( NEW ) 
        c_call glRotatef( rquad, 1.0f, 0.0f, 0.0f )

        # Set The Color To Blue One Time Only 
        c_call glColor3f( 0.5f, 0.5f, 1.0f)

        c_call glBegin(GL_QUADS)                # Draw A Quad                      
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
        c_call glEnd()                          # Done Drawing The Quad            

        # Draw it to the screen 
        c_call SDL_GL_SwapBuffers()

        # Gather our frames per second 
        frames = frames + 1

        scope
            int t = c_call int SDL_GetTicks()

            if (t - t0 >= 5000) 
                real seconds = (t - t0) / 1000.0
                real fps = frames / seconds
                vc_call printf("%d frames in %g seconds = %g FPS\n", frames, seconds, fps)
                t0 = t
                frames = 0
            end
        end

        # Increase The Rotation Variable For The Triangle ( NEW ) 
        rtri  += 0.2f
        # Decrease The Rotation Variable For The Quad     ( NEW ) 
        rquad -=0.15f

        return( TRUE )
    end

    routine main(int argc, ptr{ptr{int8}} argv ) -> int result
        result = 0

        # Flags to pass to SDL_SetVideoMode 
        int videoFlags
        # main loop variable 
        int done = FALSE
        # used to collect events 
        SDL_Event event
        # this holds some info about our display 
        const SDL_VideoInfo *videoInfo
        # whether or not the window is active 
        int isActive = TRUE

        # initialize SDL 
        if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
            fprintf( stderr, "Video initialization failed: %s\n",
                 SDL_GetError( ) )
            Quit( 1 )
        end

        # Fetch the video info 
        videoInfo = SDL_GetVideoInfo( )

        if ( !videoInfo )
            fprintf( stderr, "Video query failed: %s\n",
                 SDL_GetError( ) )
            Quit( 1 )
        end

        # the flags to pass to SDL_SetVideoMode 
        videoFlags  = SDL_OPENGL          # Enable OpenGL in SDL 
        videoFlags |= SDL_GL_DOUBLEBUFFER # Enable double buffering 
        videoFlags |= SDL_HWPALETTE       # Store the palette in hardware 
        videoFlags |= SDL_RESIZABLE       # Enable window resizing 

        # This checks to see if surfaces can be stored in memory 
        if ( videoInfo->hw_available )
            videoFlags |= SDL_HWSURFACE
        else
            videoFlags |= SDL_SWSURFACE
        end

        # This checks if hardware blits can be done 
        if ( videoInfo->blit_hw )
            videoFlags |= SDL_HWACCEL
        end

        # Sets up OpenGL double buffering 
        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 )

        # get a SDL surface 
        surface = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,
                    videoFlags )

        # Verify there is a surface 
        if ( !surface )
            fprintf( stderr,  "Video mode set failed: %s\n", SDL_GetError( ) )
            Quit( 1 )
        end

        # initialize OpenGL 
        initGL( )

        # resize the initial window 
        resizeWindow( SCREEN_WIDTH, SCREEN_HEIGHT )

        # wait for events  
        while ( !done )
        {
            # handle the events in the queue 

            while ( SDL_PollEvent( &event ) )
            {
                switch( event.type )
                {
                case SDL_ACTIVEEVENT:
                    # Something's happend with our focus
                    # If we lost focus or we are iconified, we
                    # shouldn't draw the screen
                     
                    if ( event.active.gain == 0 )
                    isActive = FALSE
                    else
                    isActive = TRUE
                    break			    
                case SDL_VIDEORESIZE:
                    # handle resize event 
                    surface = SDL_SetVideoMode( event.resize.w,
                                event.resize.h,
                                16, videoFlags )
                    if ( !surface )
                    {
                        fprintf( stderr, "Could not get a surface after resize: %s\n", SDL_GetError( ) )
                        Quit( 1 )
                    }
                    resizeWindow( event.resize.w, event.resize.h )
                    break
                case SDL_KEYDOWN:
                    # handle key presses 
                    handleKeyPress( &event.key.keysym )
                    break
                case SDL_QUIT:
                    # handle quit requests 
                    done = TRUE
                    break
                default:
                    break
                }
            }

            # draw the scene 
            if ( isActive )
                drawGLScene( )
            end
        }

        # clean ourselves up and exit 
        Quit( 0 )
    end

end
