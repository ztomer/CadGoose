using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Threading;
using System.Windows.Forms;
using GooseShared;
using SamEngine;

[assembly: CompilationRelaxations(8)]
[assembly: RuntimeCompatibility(WrapNonExceptionThrows = true)]
[assembly: Debuggable(DebuggableAttribute.DebuggingModes.IgnoreSymbolStoreSequencePoints)]
[assembly: AssemblyTitle("DefaultMod")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("")]
[assembly: AssemblyProduct("DefaultMod")]
[assembly: AssemblyCopyright("Copyright ©  2020")]
[assembly: AssemblyTrademark("")]
[assembly: ComVisible(false)]
[assembly: Guid("b28cf25d-843c-45b6-aef1-0a117f1852f5")]
[assembly: AssemblyFileVersion("1.0.0.0")]
[assembly: TargetFramework(".NETFramework,Version=v4.6.1", FrameworkDisplayName = ".NET Framework 4.6.1")]
[assembly: AssemblyVersion("1.0.0.0")]
namespace portalgoos;

internal class FollowMouseLowAccelerationTak : GooseTaskInfo
{
	public class ChangeColorTaskData : GooseTaskData
	{
		public float timeStarted;

		public float originalAcceleration;
	}

	public FollowMouseLowAccelerationTak()
	{
		base.canBePickedRandomly = true;
		base.shortName = "Portal2";
		base.description = "makes goos go to orange portal";
		base.taskID = "Portal2";
	}

	public override GooseTaskData GetNewTaskData(GooseEntity goose)
	{
		return (GooseTaskData)(object)new ChangeColorTaskData
		{
			timeStarted = Time.time,
			originalAcceleration = goose.currentAcceleration
		};
	}

	public override void RunTask(GooseEntity goose)
	{
		ChangeColorTaskData changeColorTaskData = (ChangeColorTaskData)(object)goose.currentTaskData;
		goose.currentAcceleration = 100f;
		if (Time.time - changeColorTaskData.timeStarted > 5f)
		{
			goose.currentAcceleration = changeColorTaskData.originalAcceleration;
			API.Goose.setTaskRoaming.Invoke(goose);
		}
	}
}
internal class FollowMouseLowAccelerationTask : GooseTaskInfo
{
	public class ChangeColorTaskData : GooseTaskData
	{
		public float timeStarted;

		public float originalAcceleration;
	}

	public FollowMouseLowAccelerationTask()
	{
		base.canBePickedRandomly = true;
		base.shortName = "Portal1";
		base.description = "makes goos go to blue portal";
		base.taskID = "Portal1";
	}

	public override GooseTaskData GetNewTaskData(GooseEntity goose)
	{
		return (GooseTaskData)(object)new ChangeColorTaskData
		{
			timeStarted = Time.time,
			originalAcceleration = goose.currentAcceleration
		};
	}

	public override void RunTask(GooseEntity goose)
	{
		ChangeColorTaskData changeColorTaskData = (ChangeColorTaskData)(object)goose.currentTaskData;
		goose.currentAcceleration = 100f;
		if (Time.time - changeColorTaskData.timeStarted > 5f)
		{
			goose.currentAcceleration = changeColorTaskData.originalAcceleration;
			API.Goose.setTaskRoaming.Invoke(goose);
		}
	}
}
public class ModEntryPoint : IMod
{
	private Vector2 p1pos = new Vector2(300f, 500f);

	private Vector2 p2pos = new Vector2(900f, 500f);

	private Image p1img = Image.FromFile(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "p1.png"));

	private Image p2img = Image.FromFile(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "p2.png"));

	private bool justTeleported;

	private bool portalson = true;

	private bool p0pressed;

	[DllImport("user32.dll")]
	public static extern short GetAsyncKeyState(Keys vKey);

	void IMod.Init()
	{
		//IL_000e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0018: Expected O, but got Unknown
		//IL_001f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0029: Expected O, but got Unknown
		justTeleported = false;
		InjectionPoints.PostTickEvent += new PostTickEventHandler(PostTick);
		InjectionPoints.PreRenderEvent += new PreRenderEventHandler(PreRender);
	}

	private void PreRender(GooseEntity goos, Graphics g)
	{
		if (portalson)
		{
			g.DrawImage(p1img, p1pos.x - (float)(p1img.Size.Width / 2), p1pos.y - (float)(p1img.Size.Height / 2));
			g.DrawImage(p2img, p2pos.x - (float)(p2img.Size.Width / 2), p2pos.y - (float)(p2img.Size.Height / 2));
		}
	}

	public void PostTick(GooseEntity g)
	{
		//IL_0380: Unknown result type (might be due to invalid IL or missing references)
		//IL_03a4: Unknown result type (might be due to invalid IL or missing references)
		//IL_03dc: Unknown result type (might be due to invalid IL or missing references)
		//IL_03bd: Unknown result type (might be due to invalid IL or missing references)
		//IL_03c2: Unknown result type (might be due to invalid IL or missing references)
		//IL_00da: Unknown result type (might be due to invalid IL or missing references)
		//IL_00df: Unknown result type (might be due to invalid IL or missing references)
		//IL_0475: Unknown result type (might be due to invalid IL or missing references)
		//IL_047a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0414: Unknown result type (might be due to invalid IL or missing references)
		//IL_03f5: Unknown result type (might be due to invalid IL or missing references)
		//IL_03fa: Unknown result type (might be due to invalid IL or missing references)
		//IL_01c3: Unknown result type (might be due to invalid IL or missing references)
		//IL_01c8: Unknown result type (might be due to invalid IL or missing references)
		//IL_04ac: Unknown result type (might be due to invalid IL or missing references)
		//IL_04b1: Unknown result type (might be due to invalid IL or missing references)
		if (!justTeleported && portalson)
		{
			if (g.position.x > p1pos.x - (float)(p1img.Size.Width / 2) && g.position.x < p1pos.x + (float)(p1img.Size.Width / 2) && g.position.y > p1pos.y - (float)(p1img.Size.Height / 2) && g.position.y < p1pos.y + (float)(p1img.Size.Height / 2))
			{
				g.position = p2pos;
				API.Goose.playHonckSound.Invoke();
				justTeleported = true;
			}
			else if (g.position.x > p2pos.x - (float)(p2img.Size.Width / 2) && g.position.x < p2pos.x + (float)(p2img.Size.Width / 2) && g.position.y > p2pos.y - (float)(p2img.Size.Height / 2) && g.position.y < p2pos.y + (float)(p2img.Size.Height / 2))
			{
				g.position = p1pos;
				API.Goose.playHonckSound.Invoke();
				justTeleported = true;
			}
		}
		if ((g.position.x < p1pos.x - (float)(p1img.Size.Width / 2) || g.position.x > p1pos.x + (float)(p1img.Size.Width / 2) || g.position.y < p1pos.y - (float)(p1img.Size.Height / 2) || g.position.y > p1pos.y + (float)(p1img.Size.Height / 2)) && (g.position.x < p2pos.x - (float)(p2img.Size.Width / 2) || g.position.x > p2pos.x + (float)(p2img.Size.Width / 2) || g.position.y < p2pos.y - (float)(p2img.Size.Height / 2) || g.position.y > p2pos.y + (float)(p2img.Size.Height / 2)))
		{
			justTeleported = false;
		}
		if (GetAsyncKeyState((Keys)Enum.Parse(typeof(Keys), "p", ignoreCase: true)) != 0)
		{
			if (GetAsyncKeyState((Keys)Enum.Parse(typeof(Keys), "D1", ignoreCase: true)) != 0)
			{
				p1pos = new Vector2((float)Input.mouseX, (float)Input.mouseY);
			}
			if (GetAsyncKeyState((Keys)Enum.Parse(typeof(Keys), "D2", ignoreCase: true)) != 0)
			{
				p2pos = new Vector2((float)Input.mouseX, (float)Input.mouseY);
			}
			if (GetAsyncKeyState((Keys)Enum.Parse(typeof(Keys), "D0", ignoreCase: true)) != 0 && !p0pressed)
			{
				portalson = !portalson;
				p0pressed = true;
				Thread.Sleep(300);
				p0pressed = false;
			}
		}
		if (g.currentTask == API.TaskDatabase.getTaskIndexByID.Invoke("Portal1") && portalson)
		{
			g.targetPos = p1pos;
			g.extendingNeck = true;
		}
		if (g.currentTask == API.TaskDatabase.getTaskIndexByID.Invoke("Portal2") && portalson)
		{
			g.targetPos = p2pos;
			g.extendingNeck = true;
		}
	}
}
