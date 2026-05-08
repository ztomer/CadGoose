using System;
using SamEngine;
using GooseShared;
using System.Runtime.InteropServices;
using System.Threading;
using System.Drawing;

namespace Clicker
{
    public class ModMain : IMod
    {
        [DllImport("user32.dll")]
        public static extern bool GetCursorPos(out POINT lpPoint);
        [StructLayout(LayoutKind.Sequential)]
        public struct POINT
        {
            public int X;
            public int Y;

            public static implicit operator Point(POINT point)
            {
                return new Point(point.X, point.Y);
            }
        }

        private const int MOUSEEVENTF_LEFTDOWN = 0x02;
        private const int MOUSEEVENTF_LEFTUP = 0x04;
        [DllImport("user32.dll", CharSet = CharSet.Auto, CallingConvention = CallingConvention.StdCall)]
        public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint cButtons, uint dwExtraInfo);
        [DllImport("user32.dll")]
        public static extern int SetCursorPos(int x, int y);
        public void Init()
        {
            InjectionPoints.PostTickEvent += PreTick;
        }

        public void PreTick(GooseEntity goose)
        {
            Random rad = new Random();
            int a = rad.Next(1, 300);
            if (a == 3)
            {
                POINT p;
                GetCursorPos(out p);
                SetCursorPos((int)goose.position.x, (int)goose.position.y);
                mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, (uint)goose.position.x, (uint)goose.position.y, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, (uint)goose.position.x, (uint)goose.position.y, 0, 0);
                SetCursorPos(p.X, p.Y);
                Thread.Sleep(100);
            }            
        }
    }
}
