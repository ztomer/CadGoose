using System;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using GooseShared;
using SamEngine;

[assembly: CompilationRelaxations(8)]
[assembly: RuntimeCompatibility(WrapNonExceptionThrows = true)]
[assembly: Debuggable(DebuggableAttribute.DebuggingModes.IgnoreSymbolStoreSequencePoints)]
[assembly: AssemblyTitle("DragGoose")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("")]
[assembly: AssemblyProduct("DragGoose")]
[assembly: AssemblyCopyright("Copyright ©  2020")]
[assembly: AssemblyTrademark("")]
[assembly: ComVisible(false)]
[assembly: Guid("b28cf25d-843c-45b6-aef1-0a117f1852f5")]
[assembly: AssemblyFileVersion("1.0.0.0")]
[assembly: TargetFramework(".NETFramework,Version=v4.5.2", FrameworkDisplayName = ".NET Framework 4.5.2")]
[assembly: AssemblyVersion("1.0.0.0")]
namespace DragGoose;

public class ModEntryPoint : IMod
{
	private Random rnd = new Random();

	void IMod.Init()
	{
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0011: Expected O, but got Unknown
		InjectionPoints.PostTickEvent += new PostTickEventHandler(PostTick);
	}

	public void PostTick(GooseEntity g)
	{
		//IL_002d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0032: Unknown result type (might be due to invalid IL or missing references)
		if (IsMouseOnGoose(g) && Input.leftMouseButton.Held && g.currentTask != 4)
		{
			g.position = new Vector2((float)(Input.mouseX - 5), (float)Input.mouseY);
			g.direction += (float)rnd.Next(-10, 10);
		}
	}

	public bool IsMouseOnGoose(GooseEntity g)
	{
		float num = g.position.x - (float)Input.mouseX;
		float num2 = g.position.y - (float)Input.mouseY;
		if (-45f < num && num < 45f && -45f < num2 && num2 < 45f)
		{
			return true;
		}
		return false;
	}
}
