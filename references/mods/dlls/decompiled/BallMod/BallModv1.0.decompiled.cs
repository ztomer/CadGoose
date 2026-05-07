using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
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
[assembly: TargetFramework(".NETFramework,Version=v4.5.2", FrameworkDisplayName = ".NET Framework 4.5.2")]
[assembly: AssemblyVersion("1.0.0.0")]
namespace Ball;

internal class ChargeTenSecondsTask : GooseTaskInfo
{
	public class ChargeTenSecondsTaskData : GooseTaskData
	{
		public float timeStarted;
	}

	public ChargeTenSecondsTask()
	{
		base.canBePickedRandomly = true;
		base.shortName = "10 Seconds Charge";
		base.description = "This task makes the goose charge for 10 seconds, used with the ball but the target location is set in ModMain";
		base.taskID = "ChargeTenSeconds";
	}

	public override GooseTaskData GetNewTaskData(GooseEntity goose)
	{
		return (GooseTaskData)(object)new ChargeTenSecondsTaskData
		{
			timeStarted = Time.time
		};
	}

	public override void RunTask(GooseEntity goose)
	{
		ChargeTenSecondsTaskData chargeTenSecondsTaskData = (ChargeTenSecondsTaskData)(object)goose.currentTaskData;
		API.Goose.setSpeed.Invoke(goose, (SpeedTiers)2);
		if (Time.time - chargeTenSecondsTaskData.timeStarted > 10f)
		{
			API.Goose.setSpeed.Invoke(goose, (SpeedTiers)0);
			API.Goose.setTaskRoaming.Invoke(goose);
		}
	}
}
public class ModEntryPoint : IMod
{
	private Point position;

	private Vector2 velocity;

	private float speed;

	private float deceleration;

	private float lastKickTime;

	private float lastAnimateTime;

	private float animationGap;

	private float toggleTime;

	private Image[] images;

	private int currentImage;

	private bool on;

	void IMod.Init()
	{
		//IL_0020: Unknown result type (might be due to invalid IL or missing references)
		//IL_0025: Unknown result type (might be due to invalid IL or missing references)
		//IL_00e5: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ef: Expected O, but got Unknown
		//IL_00f6: Unknown result type (might be due to invalid IL or missing references)
		//IL_0100: Expected O, but got Unknown
		position = new Point(300, 300);
		velocity = new Vector2(0f, 0f);
		lastKickTime = Time.time;
		speed = 0f;
		deceleration = 0.25f;
		images = (Image[])(object)new Image[3];
		currentImage = 0;
		lastAnimateTime = Time.time;
		animationGap = 0f;
		on = true;
		toggleTime = 0f;
		string directoryName = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
		images[0] = Image.FromFile(Path.Combine(directoryName, "ball.png"));
		images[1] = Image.FromFile(Path.Combine(directoryName, "ball2.png"));
		images[2] = Image.FromFile(Path.Combine(directoryName, "ball3.png"));
		InjectionPoints.PostTickEvent += new PostTickEventHandler(PostTick);
		InjectionPoints.PreRenderEvent += new PreRenderEventHandler(PreRender);
	}

	private void PreRender(GooseEntity goose, Graphics g)
	{
		if (on)
		{
			g.DrawImage(images[currentImage], position);
		}
	}

	public void PostTick(GooseEntity goose)
	{
		//IL_03e8: Unknown result type (might be due to invalid IL or missing references)
		//IL_03ed: Unknown result type (might be due to invalid IL or missing references)
		//IL_0106: Unknown result type (might be due to invalid IL or missing references)
		//IL_010b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0128: Unknown result type (might be due to invalid IL or missing references)
		//IL_012d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0470: Unknown result type (might be due to invalid IL or missing references)
		//IL_0471: Unknown result type (might be due to invalid IL or missing references)
		//IL_048d: Unknown result type (might be due to invalid IL or missing references)
		//IL_048e: Unknown result type (might be due to invalid IL or missing references)
		//IL_034a: Unknown result type (might be due to invalid IL or missing references)
		//IL_034b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0367: Unknown result type (might be due to invalid IL or missing references)
		//IL_0368: Unknown result type (might be due to invalid IL or missing references)
		if (Input.mouseX <= 0 && Input.mouseY <= 0 && Time.time - toggleTime > 2f)
		{
			toggleTime = Time.time;
			if (on)
			{
				on = false;
			}
			else
			{
				on = true;
				position.X = 300;
				position.Y = 300;
				lastKickTime = Time.time;
				speed = 0f;
				deceleration = 0.25f;
				currentImage = 0;
				lastAnimateTime = Time.time;
				animationGap = 0f;
			}
		}
		if (on)
		{
			if (speed > 0f)
			{
				animationGap = 0.25f / speed;
				if (Time.time - lastAnimateTime > animationGap)
				{
					animate();
					lastAnimateTime = Time.time;
				}
				velocity.x = Vector2.Normalize(velocity).x * speed;
				velocity.y = Vector2.Normalize(velocity).y * speed;
				position.X += (int)velocity.x;
				if (position.X < 0)
				{
					position.X = 0;
					velocity.x *= -1f;
				}
				if (position.X + 40 > Screen.PrimaryScreen.WorkingArea.Width)
				{
					position.X = Screen.PrimaryScreen.WorkingArea.Width - 40;
					velocity.x *= -1f;
				}
				position.Y += (int)velocity.y;
				if (position.Y < 0)
				{
					position.Y = 0;
					velocity.y *= -1f;
				}
				if (position.Y + 40 > Screen.PrimaryScreen.WorkingArea.Height)
				{
					position.Y = Screen.PrimaryScreen.WorkingArea.Height - 40;
					velocity.y *= -1f;
				}
				speed -= deceleration;
				if (speed <= 0f)
				{
					currentImage = 0;
				}
			}
			if (Input.mouseX > position.X && Input.mouseX < position.X + 40 && Input.mouseY > position.Y && Input.mouseY < position.Y + 40)
			{
				speed = 20f;
				Vector2 val = default(Vector2);
				((Vector2)(ref val))..ctor((float)(position.X + 20 - Input.mouseX), (float)(position.Y + 20 - Input.mouseY));
				velocity.x = Vector2.Normalize(val).x * speed;
				velocity.y = Vector2.Normalize(val).y * speed;
				lastKickTime = Time.time;
				API.Goose.setCurrentTaskByID.Invoke(goose, "ChargeTenSeconds", true);
			}
		}
		if (goose.currentTask != API.TaskDatabase.getTaskIndexByID.Invoke("ChargeTenSeconds"))
		{
			return;
		}
		if (on)
		{
			goose.targetPos = new Vector2((float)(position.X + 20), (float)(position.Y + 20));
			if (API.Goose.isGooseAtTarget.Invoke(goose, 40f) && Time.time - lastKickTime > 1f)
			{
				speed = 20f;
				Vector2 val2 = default(Vector2);
				((Vector2)(ref val2))..ctor((float)(position.X + 20) - goose.position.x, (float)(position.Y + 20) - goose.position.y);
				velocity.x = Vector2.Normalize(val2).x * speed;
				velocity.y = Vector2.Normalize(val2).y * speed;
				lastKickTime = Time.time;
				API.Goose.playHonckSound.Invoke();
				API.Goose.setSpeed.Invoke(goose, (SpeedTiers)0);
				API.Goose.setTaskRoaming.Invoke(goose);
			}
		}
		else
		{
			API.Goose.setTaskRoaming.Invoke(goose);
		}
	}

	public void animate()
	{
		if (velocity.x < 0f)
		{
			currentImage++;
		}
		else
		{
			currentImage--;
		}
		if (currentImage > 2)
		{
			currentImage = 0;
		}
		else if (currentImage < 0)
		{
			currentImage = 2;
		}
	}
}
