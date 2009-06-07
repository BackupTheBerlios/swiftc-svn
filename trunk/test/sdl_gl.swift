class SDL_Surface
    int dummy
end

class SDL_VideoInfo
    int dummy
end

class App
    # This is our SDL surface
    ptr{SDL_Surface} surface

    # rotational vars for the triangle and quad, respectively 
    real rtri
    real rquad

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
        real64 ratio = 640.0q / 480.0q #= width:to_real64() / height:to_real64() TODO
        c_call gluPerspective(45.0q, ratio, 0.1q, 100.0q)

        # make sure we're chaning the model view and not the projection 
        uint GL_MODELVIEW = 5888u
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
        c_call glClearDepth(1.0q)

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
        c_call glClear(16640u) # TODO GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT )

        # Move Left 1.5 Units And Into The Screen 6.0 
        c_call glLoadIdentity()
        c_call glTranslatef(-1.5, 0.0, -6.0)

        # Rotate The Triangle On The Y axis
        c_call glRotatef(.rtri, 0.0, 1.0, 0.0)

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
        c_call glRotatef(.rquad, 1.0, 0.0, 0.0)

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
        .rtri = .rtri + 0.2
        # Decrease The Rotation Variable For The Quad
        .rquad = .rquad + 0.15
    end

    routine main() -> int result
        ::start()
        result = 0
    end

    routine start() -> int result
        App app

        # main loop variable 
        bool done = false

        # initialize SDL 
        uint SDL_INIT_VIDEO = 32u
        c_call int SDL_Init(SDL_INIT_VIDEO)

        # Fetch the video info 
        ptr{const SDL_VideoInfo} videoInfo = c_call ptr{SDL_VideoInfo} SDL_GetVideoInfo()

        # Sets up OpenGL double buffering 
        int SDL_GL_DOUBLEBUFFER = 5
        int dummmy = c_call int SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)

        # Flags to pass to SDL_SetVideoMode 
        uint videoFlags = 536870935u # SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE | SDL_RESIZABLE | SDL_HWSURFACE | SDL_HWACCEL

        # get a SDL surface
        app.surface = c_call ptr{SDL_Surface} SDL_SetVideoMode(640, 480, 16, videoFlags)

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
