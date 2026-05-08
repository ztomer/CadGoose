using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Media;
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
[assembly: TargetFramework(".NETFramework,Version=v4.8", FrameworkDisplayName = ".NET Framework 4.8")]
[assembly: AssemblyVersion("1.0.0.0")]
namespace BreadCrumbs;

public class ModEntryPoint : IMod
{
	private bool feedOut;

	private Vector2 targetVector;

	private Point pointOfCrumbs;

	private SoundPlayer soundplayer;

	private Image img;

	private int tickCount;

	private string keyBind;

	private int imgWidth;

	private int imgHeight;

	[DllImport("user32.dll")]
	public static extern short GetAsyncKeyState(Keys vKey);

	void IMod.Init()
	{
		//IL_002a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0034: Expected O, but got Unknown
		//IL_0070: Unknown result type (might be due to invalid IL or missing references)
		//IL_007a: Expected O, but got Unknown
		//IL_0081: Unknown result type (might be due to invalid IL or missing references)
		//IL_008b: Expected O, but got Unknown
		string directoryName = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
		string text = Path.Combine(directoryName, "crumbs.png");
		string text2 = Path.Combine(directoryName, "nom.wav");
		soundplayer = new SoundPlayer(text2);
		img = Image.FromFile(text);
		imgWidth = img.Width;
		imgHeight = img.Height;
		CheckConfig(directoryName);
		InjectionPoints.PostTickEvent += new PostTickEventHandler(PostTick);
		InjectionPoints.PreRenderEvent += new PreRenderEventHandler(PreRenderEvent);
	}

	private void CheckConfig(string assemFolder)
	{
		//IL_008e: Unknown result type (might be due to invalid IL or missing references)
		string path = Path.Combine(assemFolder, "Config.txt");
		try
		{
			using TextReader textReader = new StreamReader(new FileStream(path, FileMode.Open));
			string text;
			while ((text = textReader.ReadLine()) != null)
			{
				if (text.StartsWith("KeyName"))
				{
					int num = text.IndexOf("=") + 1;
					keyBind = text.Substring(num, text.Length - num).Trim();
				}
			}
		}
		catch
		{
			using StreamWriter streamWriter = (File.Exists(path) ? File.AppendText(path) : File.CreateText(path));
			MessageBox.Show("Config.txt for BreadCrumbs was not found.\nMaking config file please restart", "Mod Error", (MessageBoxButtons)0, (MessageBoxIcon)16);
			streamWriter.WriteLine("KeyName=RShiftKey");
		}
	}

	private void PreRenderEvent(GooseEntity goose, Graphics g)
	{
		if (feedOut)
		{
			g.DrawImage(img, pointOfCrumbs.X, pointOfCrumbs.Y, imgWidth, imgHeight);
		}
	}

	public void PostTick(GooseEntity g)
	{
		//IL_0016: Unknown result type (might be due to invalid IL or missing references)
		//IL_0056: Unknown result type (might be due to invalid IL or missing references)
		//IL_005b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0099: Unknown result type (might be due to invalid IL or missing references)
		//IL_009e: Unknown result type (might be due to invalid IL or missing references)
		//IL_019c: Unknown result type (might be due to invalid IL or missing references)
		//IL_01a1: Unknown result type (might be due to invalid IL or missing references)
		//IL_015a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0169: Unknown result type (might be due to invalid IL or missing references)
		//IL_016e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0173: Unknown result type (might be due to invalid IL or missing references)
		if (GetAsyncKeyState((Keys)Enum.Parse(typeof(Keys), keyBind, ignoreCase: true)) != 0 && !feedOut)
		{
			feedOut = true;
			targetVector = new Vector2((float)(Input.mouseX - imgWidth / 4), (float)(Input.mouseY + imgHeight / 4));
			pointOfCrumbs = new Point(Cursor.Position.X - imgWidth / 2, Cursor.Position.Y - imgHeight / 2);
			g.targetPos = targetVector;
			API.Goose.playHonckSound.Invoke();
			API.Goose.setTaskRoaming.Invoke(g);
			API.Goose.setSpeed.Invoke(g, (SpeedTiers)2);
		}
		if (!feedOut)
		{
			return;
		}
		if (API.Goose.isGooseAtTarget.Invoke(g, 10f))
		{
			tickCount++;
			g.direction = -20f;
			API.Goose.setSpeed.Invoke(g, (SpeedTiers)0);
			API.Goose.setTaskRoaming.Invoke(g);
			if (tickCount == 240)
			{
				soundplayer.Play();
				feedOut = false;
				tickCount = 0;
				g.targetPos = targetVector + new Vector2(20f, -20f);
			}
		}
		else
		{
			API.Goose.setTaskRoaming.Invoke(g);
			API.Goose.setSpeed.Invoke(g, (SpeedTiers)2);
			g.targetPos = targetVector;
		}
	}
}
