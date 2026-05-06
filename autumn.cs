using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
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
public class LeafPile
{
	public struct Leaf
	{
		public Vector2 curPosPlanar;

		public float curPosZ;

		public Vector2 velPlanar;

		public float velZ;

		public Vector2 GetScreenOffset(float zScale = 1f)
		{
			//IL_0001: Unknown result type (might be due to invalid IL or missing references)
			//IL_001a: Unknown result type (might be due to invalid IL or missing references)
			//IL_001f: Unknown result type (might be due to invalid IL or missing references)
			return curPosPlanar + new Vector2(0f, (0f - curPosZ) * zScale * 0.6f);
		}
	}

	public static Brush ShadowBrush;

	public const float LIFETIME_AFTER_KICKED = 10f;

	private const int LEAVES_PER_PILE_MAX = 128;

	private const float RENDER_Z_SCALE_VERTICAL = 0.6f;

	private const float GRAVITY = -900f;

	private const float LEAF_RENDERRAD_W = 5f;

	private const float KICK_MIN_VERT_VEL = 10f;

	private const float KICK_MAX_VERT_VEL = 500f;

	private const float MAX_VEL_XY = 200f;

	public Vector2 pos;

	public float rad;

	public float timeSinceKicked = -1f;

	public Leaf[] leaves = new Leaf[128];

	private float timeCreated;

	private Brush[] leafBrushes = (Brush[])(object)new Brush[4]
	{
		(Brush)new SolidBrush(Color.FromArgb(255, 208, 122, 45)),
		(Brush)new SolidBrush(Color.FromArgb(255, 234, 198, 54)),
		(Brush)new SolidBrush(Color.FromArgb(255, 172, 193, 79)),
		(Brush)new SolidBrush(Color.FromArgb(255, 208, 87, 64))
	};

	private const float SPAWN_ANIM_LENGTH = 1f;

	public void Init(Vector2 position, float radius, float height)
	{
		//IL_0001: Unknown result type (might be due to invalid IL or missing references)
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_002c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		//IL_0033: Unknown result type (might be due to invalid IL or missing references)
		//IL_0035: Unknown result type (might be due to invalid IL or missing references)
		//IL_007b: Unknown result type (might be due to invalid IL or missing references)
		//IL_007e: Unknown result type (might be due to invalid IL or missing references)
		//IL_008b: Unknown result type (might be due to invalid IL or missing references)
		//IL_009a: Unknown result type (might be due to invalid IL or missing references)
		//IL_009f: Unknown result type (might be due to invalid IL or missing references)
		//IL_00a4: Unknown result type (might be due to invalid IL or missing references)
		//IL_00b6: Unknown result type (might be due to invalid IL or missing references)
		//IL_00bb: Unknown result type (might be due to invalid IL or missing references)
		pos = position;
		rad = radius;
		timeSinceKicked = -1f;
		timeCreated = Time.time;
		for (int i = 0; i < leaves.Length; i++)
		{
			Vector2 val = RandomInUnitCircle();
			Vector2.Distance(val, Vector2.zero);
			float num = SamMath.RandomRange(0f, 1f);
			num *= num;
			leaves[i].curPosZ = num * height;
			leaves[i].curPosPlanar = val * radius * (1f - num) * new Vector2(1f, 0.6f);
			leaves[i].velPlanar = Vector2.zero;
			leaves[i].velZ = 0f;
		}
		float val2 = float.MaxValue;
		float val3 = float.MinValue;
		float val4 = float.MaxValue;
		float val5 = float.MinValue;
		float val6 = float.MaxValue;
		float val7 = float.MinValue;
		for (int j = 0; j < leaves.Length; j++)
		{
			val2 = Math.Min(val2, leaves[j].curPosPlanar.x);
			val3 = Math.Max(val3, leaves[j].curPosPlanar.x);
			val4 = Math.Min(val4, leaves[j].curPosPlanar.y);
			val5 = Math.Max(val5, leaves[j].curPosPlanar.y);
			val6 = Math.Min(val6, leaves[j].curPosZ);
			val7 = Math.Max(val7, leaves[j].curPosZ);
		}
	}

	private static Vector2 RandomInUnitCircle()
	{
		//IL_0034: Unknown result type (might be due to invalid IL or missing references)
		float num = SamMath.RandomRange(0f, 360f);
		float num2 = SamMath.RandomRange(0f, 1f);
		return new Vector2(num2 * (float)Math.Cos(num), num2 * (float)Math.Sin(num));
	}

	public void Kick(Vector2 kickVelocity, Vector2 goosePos, float gooseSpeedPercentage)
	{
		//IL_002f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0034: Unknown result type (might be due to invalid IL or missing references)
		//IL_0039: Unknown result type (might be due to invalid IL or missing references)
		//IL_003f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0040: Unknown result type (might be due to invalid IL or missing references)
		//IL_0041: Unknown result type (might be due to invalid IL or missing references)
		//IL_0052: Unknown result type (might be due to invalid IL or missing references)
		//IL_0062: Unknown result type (might be due to invalid IL or missing references)
		//IL_0067: Unknown result type (might be due to invalid IL or missing references)
		//IL_0069: Unknown result type (might be due to invalid IL or missing references)
		//IL_006b: Unknown result type (might be due to invalid IL or missing references)
		//IL_006d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0077: Unknown result type (might be due to invalid IL or missing references)
		//IL_0081: Unknown result type (might be due to invalid IL or missing references)
		//IL_0086: Unknown result type (might be due to invalid IL or missing references)
		//IL_008b: Unknown result type (might be due to invalid IL or missing references)
		//IL_008d: Unknown result type (might be due to invalid IL or missing references)
		//IL_008f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0095: Unknown result type (might be due to invalid IL or missing references)
		//IL_009a: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ce: Unknown result type (might be due to invalid IL or missing references)
		//IL_00d1: Unknown result type (might be due to invalid IL or missing references)
		//IL_00d6: Unknown result type (might be due to invalid IL or missing references)
		timeSinceKicked = Time.time;
		float num = SamMath.Lerp(0.6f, 1.1f, gooseSpeedPercentage);
		for (int i = 0; i < leaves.Length; i++)
		{
			Vector2 val = Vector2.Normalize(leaves[i].curPosPlanar);
			float num2 = 1f - Math.Abs(Vector2.Dot(val, Vector2.Normalize(kickVelocity)));
			Vector2 val2 = val * SamMath.RandomRange(0f, 200f);
			val2 += val * num2 * 200f * 0.2f;
			val2 = Vector2.Lerp(val2, kickVelocity, 0.3f);
			float num3 = SamMath.RandomRange(10f, 500f);
			num3 *= SamMath.Lerp(0.9f, 1.1f, num2);
			leaves[i].velPlanar = val2 * num;
			leaves[i].velZ = num3 * num;
		}
	}

	public void TickLeaves(GooseEntity g)
	{
		//IL_00d8: Unknown result type (might be due to invalid IL or missing references)
		//IL_00de: Unknown result type (might be due to invalid IL or missing references)
		//IL_010f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0123: Unknown result type (might be due to invalid IL or missing references)
		//IL_0129: Unknown result type (might be due to invalid IL or missing references)
		//IL_002c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0032: Unknown result type (might be due to invalid IL or missing references)
		//IL_003c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0041: Unknown result type (might be due to invalid IL or missing references)
		//IL_0046: Unknown result type (might be due to invalid IL or missing references)
		//IL_00a3: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ad: Unknown result type (might be due to invalid IL or missing references)
		//IL_00b2: Unknown result type (might be due to invalid IL or missing references)
		if (timeSinceKicked > 0f)
		{
			for (int i = 0; i < leaves.Length; i++)
			{
				Leaf leaf = leaves[i];
				ref Vector2 curPosPlanar = ref leaf.curPosPlanar;
				curPosPlanar += leaf.velPlanar * (1f / 120f);
				leaf.curPosZ += leaf.velZ * (1f / 120f);
				leaf.velZ += -7.5000005f;
				if (leaf.curPosZ < 0f)
				{
					leaf.curPosZ = 0f;
					leaf.velZ *= -0.3f;
					ref Vector2 velPlanar = ref leaf.velPlanar;
					velPlanar *= 0.2f;
				}
				leaves[i] = leaf;
			}
		}
		else if (Vector2.Distance(g.position, pos) < rad + 4f)
		{
			float walkSpeed = g.parameters.WalkSpeed;
			float chargeSpeed = g.parameters.ChargeSpeed;
			float gooseSpeedPercentage = (Vector2.Magnitude(g.velocity) - walkSpeed) / (chargeSpeed - walkSpeed);
			Kick(g.velocity, g.position, gooseSpeedPercentage);
		}
	}

	public void RenderLeaves(Graphics g, Vector2 goosePos, bool renderAboveGoose)
	{
		//IL_00f0: Unknown result type (might be due to invalid IL or missing references)
		//IL_0168: Unknown result type (might be due to invalid IL or missing references)
		//IL_016e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0173: Unknown result type (might be due to invalid IL or missing references)
		//IL_0178: Unknown result type (might be due to invalid IL or missing references)
		//IL_018d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0197: Unknown result type (might be due to invalid IL or missing references)
		float num = 0f;
		float num2 = 8f;
		float num3 = 2f;
		float num4 = Easings.easeOutBounce(SamMath.Clamp(SamMath.Clamp((Time.time - timeCreated) / 1f, 0f, 1f), 0f, 1f));
		if (timeSinceKicked > 0f)
		{
			float num5 = Time.time - timeSinceKicked;
			_ = num5 / num2;
			num = SamMath.Clamp((num5 - num2) / num3, 0f, 1f);
		}
		if (!renderAboveGoose)
		{
			float num6 = (1f - num) * num4 * rad;
			g.FillEllipse(ShadowBrush, pos.x - num6, pos.y - num6 * 0.6f, num6 * 2f, num6 * 2f * 0.6f);
		}
		for (int i = 0; i < leaves.Length; i++)
		{
			if (((leaves[i].curPosPlanar.y < goosePos.y) & (leaves[i].curPosZ > 25f)) == renderAboveGoose)
			{
				float num7 = 5f + 5f * leaves[i].curPosZ / 900f;
				num7 *= 1f - num;
				num7 *= num4;
				float num8 = num7 * 2f;
				Vector2 val = leaves[i].GetScreenOffset(num4) + pos;
				g.FillEllipse(leafBrushes[i % leafBrushes.Length], val.x - num7, val.y - num7, num8, num8 * 0.6f);
			}
		}
	}
}
internal static class sks92j3qSIIS
{
	public static int SHH;

	[DllImport("user32.dll")]
	private static extern int SetWindowLong(IntPtr hWnd, int nIndex, uint dwNewLong);

	public static void swb(bool ib)
	{
		uint num = ((!ib) ? 32u : 0u);
		SetWindowLong(((Control)Application.OpenForms[0]).Handle, -20, 0x80000 | num | 0x8000000);
	}
}
internal static class Easings
{
	public static float easeOutBounce(float x)
	{
		float num = 7.5625f;
		float num2 = 2.75f;
		if (x < 1f / num2)
		{
			return num * x * x;
		}
		if (x < 2f / num2)
		{
			return num * (x -= 1.5f / num2) * x + 0.75f;
		}
		if ((double)x < 2.5 / (double)num2)
		{
			return num * (x -= 2.25f / num2) * x + 0.9375f;
		}
		return num * (x -= 2.625f / num2) * x + 63f / 64f;
	}
}
namespace Autumn;

internal class ChaseLeafPile : GooseTaskInfo
{
	public class ChaseLeafPileTaskData : GooseTaskData
	{
		public LeafPile lp;

		public bool hasInited;

		public float timeStarted;
	}

	public ChaseLeafPile()
	{
		base.canBePickedRandomly = true;
		base.shortName = "Chase a leaf pile";
		base.description = "Make the goose run into a leaf pile";
		base.taskID = "Autumn_ChaseLeafPile";
	}

	public override GooseTaskData GetNewTaskData(GooseEntity goose)
	{
		ChaseLeafPileTaskData chaseLeafPileTaskData = new ChaseLeafPileTaskData();
		if (ModEntryPoint.piles.Count > 0)
		{
			chaseLeafPileTaskData.lp = ModEntryPoint.piles[(int)SamMath.RandomRange(0f, (float)ModEntryPoint.piles.Count - 0.01f)];
		}
		chaseLeafPileTaskData.timeStarted = Time.time;
		return (GooseTaskData)(object)chaseLeafPileTaskData;
	}

	public override void RunTask(GooseEntity goose)
	{
		//IL_000d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0013: Unknown result type (might be due to invalid IL or missing references)
		//IL_004f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0055: Unknown result type (might be due to invalid IL or missing references)
		//IL_005a: Unknown result type (might be due to invalid IL or missing references)
		//IL_005f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0064: Unknown result type (might be due to invalid IL or missing references)
		//IL_007e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0083: Unknown result type (might be due to invalid IL or missing references)
		//IL_0085: Unknown result type (might be due to invalid IL or missing references)
		//IL_008a: Unknown result type (might be due to invalid IL or missing references)
		//IL_008f: Unknown result type (might be due to invalid IL or missing references)
		if (ModEntryPoint.piles.Count == 0 || Vector2.Distance(goose.position, goose.targetPos) < 20f)
		{
			API.Goose.setTaskRoaming.Invoke(goose);
			return;
		}
		ChaseLeafPileTaskData chaseLeafPileTaskData = (ChaseLeafPileTaskData)(object)goose.currentTaskData;
		if (!chaseLeafPileTaskData.hasInited)
		{
			Vector2 val = Vector2.Normalize(chaseLeafPileTaskData.lp.pos - goose.position);
			float num = chaseLeafPileTaskData.lp.rad * 4f;
			goose.targetPos = chaseLeafPileTaskData.lp.pos + val * num;
			chaseLeafPileTaskData.hasInited = true;
		}
		API.Goose.setSpeed.Invoke(goose, (SpeedTiers)2);
	}
}
public class ModEntryPoint : IMod
{
	private const float MaxLeafPiles = 6f;

	private const float FIRST_PILE_SECONDS = 10f;

	private const float PILE_INTERVAL_MIN = 4.7999997f;

	private const float PILE_INTERVAL_MAX = 72f;

	private float NextPileTime = 10f;

	public static List<LeafPile> piles = new List<LeafPile>();

	void IMod.Init()
	{
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0011: Expected O, but got Unknown
		//IL_0018: Unknown result type (might be due to invalid IL or missing references)
		//IL_0022: Expected O, but got Unknown
		//IL_0029: Unknown result type (might be due to invalid IL or missing references)
		//IL_0033: Expected O, but got Unknown
		//IL_003a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0044: Expected O, but got Unknown
		InjectionPoints.PostTickEvent += new PostTickEventHandler(PostTick);
		InjectionPoints.PreRenderEvent += new PreRenderEventHandler(InjectionPoints_PreRenderEvent);
		InjectionPoints.PostRenderEvent += new PostRenderEventHandler(InjectionPoints_PostRenderEvent);
		InjectionPoints.PreTickEvent += new PreTickEventHandler(InjectionPoints_PreTickEvent);
	}

	private void InjectionPoints_PreTickEvent(GooseEntity goose)
	{
		//IL_000c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0012: Unknown result type (might be due to invalid IL or missing references)
		sks92j3qSIIS.swb(Vector2.Distance(new Vector2((float)Input.mouseX, (float)Input.mouseY), goose.position) < 20f);
	}

	private void InjectionPoints_PreRenderEvent(GooseEntity goose, Graphics g)
	{
		//IL_0011: Unknown result type (might be due to invalid IL or missing references)
		for (int i = 0; i < piles.Count; i++)
		{
			piles[i].RenderLeaves(g, goose.position, renderAboveGoose: false);
		}
	}

	private void InjectionPoints_PostRenderEvent(GooseEntity goose, Graphics g)
	{
		//IL_0011: Unknown result type (might be due to invalid IL or missing references)
		for (int i = 0; i < piles.Count; i++)
		{
			piles[i].RenderLeaves(g, goose.position, renderAboveGoose: true);
		}
	}

	public void PostTick(GooseEntity g)
	{
		//IL_00ac: Unknown result type (might be due to invalid IL or missing references)
		LeafPile.ShadowBrush = (Brush)(object)g.renderData.shadowBrush;
		if (Time.time > NextPileTime)
		{
			NextPileTime = Time.time + SamMath.RandomRange(4.7999997f, 72f);
			if ((float)piles.Count < 6f)
			{
				LeafPile leafPile = new LeafPile();
				Form val = Application.OpenForms[0];
				Vector2 position = default(Vector2);
				((Vector2)(ref position))..ctor((float)((Control)val).Width, (float)((Control)val).Height);
				position.x *= SamMath.RandomRange(0.2f, 0.8f);
				position.y *= SamMath.RandomRange(0.2f, 0.8f);
				leafPile.Init(position, SamMath.RandomRange(30f, 50f), SamMath.RandomRange(30f, 50f));
				piles.Add(leafPile);
			}
		}
		for (int num = piles.Count - 1; num >= 0; num--)
		{
			piles[num].TickLeaves(g);
			if (piles[num].timeSinceKicked > 0f && Time.time - piles[num].timeSinceKicked > 10f)
			{
				piles.RemoveAt(num);
			}
		}
	}
}
