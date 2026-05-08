using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;
using GooseShared;
using SamEngine;
using DefaultMod;
using System.Drawing.Drawing2D;

namespace OnePunchGoose
    
{
    public class ModEntryPoint : IMod
    {


        [DllImport("user32.dll")]
        public static extern short GetAsyncKeyState(Keys vKey);
        ParticleEngine TEST = new ParticleEngine(new Vector2(400, 240));
        void IMod.Init()
        {
            Config.LoadConfig();
            InjectionPoints.PostTickEvent += PostTick;
            InjectionPoints.PreRenderEvent += preRenderEvent;
        }

        public static double radians(double angle)
        {
            return (Math.PI / 180) * angle;
        }


        public void preRenderEvent(GooseEntity goose, Graphics g)
        {
            if (goose.currentTask == API.TaskDatabase.getTaskIndexByID("CustomMouseNab"))
            {
                g.InterpolationMode = InterpolationMode.NearestNeighbor;
                FollowMouseLowAccelerationTask.ChangeColorTaskData data = (FollowMouseLowAccelerationTask.ChangeColorTaskData)goose.currentTaskData;
                Vector2 v = new Vector2((float)Math.Cos(radians(data.dir)), (float)Math.Sin(radians(data.dir)));
                if (data.ONEPUNCH)
                {
                    goose.direction = data.dir;
                    if (Config.settings.EnableParticle)
                    {
                        TEST.Velocity = v * Config.settings.ParticleVelocity;
                        TEST.VelocityAngle = 1 / data.dir;
                        TEST.Size = Config.settings.ParticlesSize;
                        TEST.EmitterLocation = goose.rig.head2EndPoint;
                        TEST.Update();
                        TEST.Draw(g);
                    }
                }

            }
            else
            {
                if (Config.settings.EnableParticle)
                {
                    TEST.EmitterLocation = goose.rig.head2EndPoint;
                    TEST.Stop();
                    TEST.Draw(g);
                }
            }
        }
        public void LinearSmoothMove(Point newPosition, int steps)
        {
            Point start = Cursor.Position;
            PointF iterPoint = start;


            PointF slope = new PointF(newPosition.X - start.X, newPosition.Y - start.Y);


            slope.X = slope.X / steps;
            slope.Y = slope.Y / steps;


            for (int i = 0; i < steps; i++)
            {
                iterPoint = new PointF(iterPoint.X + slope.X, iterPoint.Y + slope.Y);
                Cursor.Position = (Point.Round(iterPoint));
                Thread.Sleep(10);
            }

            Cursor.Position = (newPosition);
        }
        public void PostTick(GooseEntity g)
        {

            if (g.currentTask == API.TaskDatabase.getTaskIndexByID("NabMouse"))
            {

                API.Goose.setCurrentTaskByID(g,"CustomMouseNab", false);
            }
            if (g.currentTask == API.TaskDatabase.getTaskIndexByID("CustomMouseNab"))
            {

                FollowMouseLowAccelerationTask.ChangeColorTaskData data = (FollowMouseLowAccelerationTask.ChangeColorTaskData)g.currentTaskData;
                if (data.ONEPUNCH)
                {
                    new Thread(() =>
                    {
                        Vector2 cursorVec = new Vector2(Cursor.Position.X, Cursor.Position.Y);
                        Vector2 v = new Vector2((float)Math.Cos(radians(data.dir)), (float)Math.Sin(radians(data.dir)));
                        LinearSmoothMove(new Point((int)(v.x + cursorVec.x), (int)(v.x + cursorVec.y)), 20);
                        Thread.Sleep(1802);
                        API.Goose.setTaskRoaming(g);
                        LinearSmoothMove(new Point((int)(v.x * 2000 + cursorVec.x), (int)(v.y * 2000 + cursorVec.y)), 20);
                    }).Start();
                }
                return;
            }
        }
        }
    }
