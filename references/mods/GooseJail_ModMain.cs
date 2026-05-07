using System;
using System.Windows.Forms;
using System.Drawing;
using System.Threading;
using System.Runtime.InteropServices;
using System.IO;
using System.Reflection;
// 1. Added the "GooseModdingAPI" project as a reference.
// 2. Compile this.
// 3. Create a folder with this DLL in the root, and *no GooseModdingAPI DLL*
using GooseShared;
using SamEngine;


namespace GooseOfChaos
{

    public class ModEntryPoint : IMod
    {
        //These two lines allow for key capture
        [DllImport("user32.dll")]
        public static extern short GetAsyncKeyState(Keys vKey);
        
        //Setting the position the jail will be when its not active
        Vector2 jailpos = new Vector2(-250, -250);
        //Loads the image for the jail
        Image jailimg = Image.FromFile(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "jail.png"));
        //Exactly what you think it is
        bool jailtoggle = false;
        //The default position of the jail when you haven't changed it
        Vector2 jailposforgoose = new Vector2(300, 400);

        void IMod.Init()
        {

            // Subscribe to whatever events we want
            InjectionPoints.PostTickEvent += PostTick;
            InjectionPoints.PostRenderEvent += PostRender;
        }

        //This function allows me to render the jailcell on top of the goose because it fires and runs
        //after the goose is drawn onto the screen
        private void PostRender(GooseEntity goos, Graphics gfx)
        {
            //This listens for when the key "o" is pressed and then changes the position where
            //the goose will be caged
            if (GetAsyncKeyState((Keys)Enum.Parse(typeof(Keys), "O", true)) != 0)
            {
                jailposforgoose = new Vector2(Input.mouseX, Input.mouseY);
            }
            if (GetAsyncKeyState((Keys)Enum.Parse(typeof(Keys), "P", true)) != 0)
            {
                //Toggles the jail with some extra QoL
                jailtoggle = !jailtoggle;
                API.Goose.setTaskRoaming(goos);
                API.Goose.playHonckSound();
                Thread.Sleep(200);
                
            }
            if (jailtoggle)
            {
                //Makes the goose look at your mouse
                goos.targetPos = new Vector2(Input.mouseX, Input.mouseY);
                //makes the jailcell lock to the goose
                jailpos = new Vector2(goos.position.x, goos.position.y);
                //Draws the jailcell
                
                /* 
                 do NOT EVER EVER have half transparent pixels,
                 for me they showed up as orange lines and I spent way
                 too long trying to figure it out.
                */

                gfx.DrawImageUnscaled(jailimg, ((int)jailpos.x-(jailimg.Width/2)), ((int)jailpos.y-(jailimg.Height/2)));
                //makes the goose go to the position where you set him to be jailed.
                goos.position = new Vector2(jailposforgoose.x, jailposforgoose.y);
            }

        }
        



        public void PostTick(GooseEntity g)
        {
            //literally there is nothing I can add here
        }
    }
}
