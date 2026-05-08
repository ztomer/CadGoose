using System;
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Threading;
using GooseShared;

[assembly: CompilationRelaxations(8)]
[assembly: RuntimeCompatibility(WrapNonExceptionThrows = true)]
[assembly: Debuggable(DebuggableAttribute.DebuggingModes.Default | DebuggableAttribute.DebuggingModes.DisableOptimizations | DebuggableAttribute.DebuggingModes.IgnoreSymbolStoreSequencePoints | DebuggableAttribute.DebuggingModes.EnableEditAndContinue)]
[assembly: TargetFramework(".NETStandard,Version=v2.0", FrameworkDisplayName = "")]
[assembly: AssemblyCompany("Clicker")]
[assembly: AssemblyConfiguration("Debug")]
[assembly: AssemblyFileVersion("1.0.0.0")]
[assembly: AssemblyInformationalVersion("1.0.0")]
[assembly: AssemblyProduct("Clicker")]
[assembly: AssemblyTitle("Clicker")]
[assembly: AssemblyVersion("1.0.0.0")]
namespace Clicker;

public class ModMain : IMod
{
	public struct POINT
	{
		public int X;

		public int Y;

		public static implicit operator Point(POINT point)
		{
			return new Point(point.X, point.Y);
		}
	}

	private const int MOUSEEVENTF_LEFTDOWN = 2;

	private const int MOUSEEVENTF_LEFTUP = 4;

	[DllImport("user32.dll")]
	public static extern bool GetCursorPos(out POINT lpPoint);

	[DllImport("user32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Auto)]
	public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint cButtons, uint dwExtraInfo);

	[DllImport("user32.dll")]
	public static extern int SetCursorPos(int x, int y);

	public void Init()
	{
		//IL_0008: Unknown result type (might be due to invalid IL or missing references)
		//IL_0012: Expected O, but got Unknown
		InjectionPoints.PostTickEvent += new PostTickEventHandler(PreTick);
	}

	public void PreTick(GooseEntity goose)
	{
		Random random = new Random();
		int num = random.Next(1, 300);
		if (num == 3)
		{
			GetCursorPos(out var lpPoint);
			SetCursorPos((int)goose.position.x, (int)goose.position.y);
			mouse_event(6u, (uint)goose.position.x, (uint)goose.position.y, 0u, 0u);
			mouse_event(6u, (uint)goose.position.x, (uint)goose.position.y, 0u, 0u);
			SetCursorPos(lpPoint.X, lpPoint.Y);
			Thread.Sleep(100);
		}
	}
}
