using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Windows.Forms;
using GooseShared;

[assembly: CompilationRelaxations(8)]
[assembly: RuntimeCompatibility(WrapNonExceptionThrows = true)]
[assembly: Debuggable(DebuggableAttribute.DebuggingModes.IgnoreSymbolStoreSequencePoints)]
[assembly: AssemblyTitle("Honcker")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("")]
[assembly: AssemblyProduct("Honcker")]
[assembly: AssemblyCopyright("Copyright ©  2020")]
[assembly: AssemblyTrademark("")]
[assembly: ComVisible(false)]
[assembly: Guid("77d7e65e-54ca-41ec-837e-ac07556d49c1")]
[assembly: AssemblyFileVersion("1.0.0.0")]
[assembly: TargetFramework(".NETFramework,Version=v4.7.2", FrameworkDisplayName = ".NET Framework 4.7.2")]
[assembly: AssemblyVersion("1.0.0.0")]
namespace Honcker;

public class ModMain : IMod
{
	[DllImport("user32.dll")]
	public static extern short GetAsyncKeyState(Keys vKey);

	public void Init()
	{
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0011: Expected O, but got Unknown
		InjectionPoints.PreTickEvent += new PreTickEventHandler(PreTick);
	}

	public void PreTick(GooseEntity goose)
	{
		bool flag = false;
		if (GetAsyncKeyState((Keys)70) != 0)
		{
			if (!flag)
			{
				API.Goose.playHonckSound.Invoke();
				flag = true;
			}
		}
		else
		{
			flag = false;
		}
	}
}
