using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using BulletinboardSystem;
using Cysharp.Threading.Tasks;
using Cysharp.Threading.Tasks.CompilerServices;
using GiantArmy.ECS;
using GiantArmy.Localization;
using GiantArmy.UI.DynamicLayoutSystem;
using GiantArmy.UI.VRDash;
using GiantArmy.Win32;
using Mathematics;
using CosmosLogic.Core;
using CosmosLogic.Events;

public class Sandbox : MonoBehaviour
{
	[Serializable]
	[CompilerGenerated]
	private sealed class <>c
	{
		public static readonly <>c <>9 = new <>c();

		public static Func<bool> <>9__11_0;

		public static Func<bool> <>9__15_0;

		public static Func<bool> <>9__18_0;

		public static Action<Message> <>9__163_0;

		public static CosmosAction <>9__170_0;

		public static CosmosAction <>9__170_1;

		public static Func<bool> <>9__172_0;

		public static Func<bool> <>9__199_0;

		internal bool <ReloadSimulationTask>b__11_0()
		{
			return !FileTools.IsSavingAsync;
		}

		internal bool <LoadSim>b__15_0()
		{
			return WinProgress.IsAwaitingCapture;
		}

		internal bool <LoadSimInternal>b__18_0()
		{
			return !Quicksave.IsBusy;
		}

		internal void <Start>b__163_0(Message msg)
		{
			//IL_001c: Unknown result type (might be due to invalid IL or missing references)
			Debug.Log((object)msg.Data.ToString());
			UniTaskExtensions.Forget(LoadSimAsync(msg.Data.ToString()));
		}

		internal void <CompleteLoading>b__170_0()
		{
			Application.OpenURL("https://universesandbox.com/support/savefileversions");
		}

		internal void <CompleteLoading>b__170_1()
		{
			Application.OpenURL("https://universesandbox.com/support/savefileversions");
		}

		internal bool <OnFirstUpdate>b__172_0()
		{
			return UIPropertyPanel.FinishedInitializingPools;
		}

		internal bool <LoadSimCoroutine>b__199_0()
		{
			return !Quicksave.IsBusy;
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <FinishFrame>d__173 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Sandbox <>4__this;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_004c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0051: Unknown result type (might be due to invalid IL or missing references)
			//IL_0058: Unknown result type (might be due to invalid IL or missing references)
			//IL_0014: Unknown result type (might be due to invalid IL or missing references)
			//IL_0019: Unknown result type (might be due to invalid IL or missing references)
			//IL_001c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0021: Unknown result type (might be due to invalid IL or missing references)
			//IL_0035: Unknown result type (might be due to invalid IL or missing references)
			//IL_0036: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			Sandbox sandbox = <>4__this;
			try
			{
				if (num != 0)
				{
					goto IL_0089;
				}
				Awaiter val = <>u__1;
				<>u__1 = default;
				num = (<>1__state = -1);
				goto IL_0067;
				IL_0067:
				val.GetResult();
				WasSingleStep = false;
				Watchdog.EndOfFrame();
				OnFrameEnded?.Invoke();
				goto IL_0089;
				IL_0089:
				if (!FinalQuit)
				{
					UniTask val2 = UniTask.WaitForEndOfFrame((MonoBehaviour)(object)sandbox);
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <FinishFrame>d__173>(ref val, ref this);
						return;
					}
					goto IL_0067;
				}
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <FromCurrentFileAsync>d__187 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder<Simulation> <>t__builder;

		private Awaiter<Simulation> <>u__1;

		private void MoveNext()
		{
			//IL_0084: Unknown result type (might be due to invalid IL or missing references)
			//IL_0089: Unknown result type (might be due to invalid IL or missing references)
			//IL_0090: Unknown result type (might be due to invalid IL or missing references)
			//IL_0108: Unknown result type (might be due to invalid IL or missing references)
			//IL_010d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0114: Unknown result type (might be due to invalid IL or missing references)
			//IL_00cf: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d4: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d8: Unknown result type (might be due to invalid IL or missing references)
			//IL_00dd: Unknown result type (might be due to invalid IL or missing references)
			//IL_0048: Unknown result type (might be due to invalid IL or missing references)
			//IL_004d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0051: Unknown result type (might be due to invalid IL or missing references)
			//IL_0056: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f1: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f2: Unknown result type (might be due to invalid IL or missing references)
			//IL_006a: Unknown result type (might be due to invalid IL or missing references)
			//IL_006b: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			Simulation result;
			try
			{
				Awaiter<Simulation> val;
				if (num == 0)
				{
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_009f;
				}
				if (num == 1)
				{
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_0123;
				}
				if (CurrentFile.EndsWith("ubox"))
				{
					if (File.Exists(CurrentFile))
					{
						val = FileTools.LoadSimulationAsync(CurrentFile).GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 0);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter<Simulation>, <FromCurrentFileAsync>d__187>(ref val, ref this);
							return;
						}
						goto IL_009f;
					}
					result = null;
				}
				else
				{
					if (Directory.Exists(CurrentFile))
					{
						val = FileTools.LoadSimulationFlatAsync(CurrentFile).GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 1);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter<Simulation>, <FromCurrentFileAsync>d__187>(ref val, ref this);
							return;
						}
						goto IL_0123;
					}
					result = null;
				}
				goto end_IL_0007;
				IL_009f:
				result = val.GetResult();
				goto end_IL_0007;
				IL_0123:
				result = val.GetResult();
				end_IL_0007:;
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult(result);
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <InvokeLoadingAsync>d__20 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		private Awaiter<Simulation> <>u__1;

		private void MoveNext()
		{
			//IL_006f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0074: Unknown result type (might be due to invalid IL or missing references)
			//IL_007b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0034: Unknown result type (might be due to invalid IL or missing references)
			//IL_0039: Unknown result type (might be due to invalid IL or missing references)
			//IL_003c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0041: Unknown result type (might be due to invalid IL or missing references)
			//IL_0055: Unknown result type (might be due to invalid IL or missing references)
			//IL_0056: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				try
				{
					Simulation simulation;
					Awaiter<Simulation> val;
					if (num != 0)
					{
						Thread.CurrentThread.CurrentCulture = CultureInfo.InvariantCulture;
						Body.ResetId();
						Group.ResetId();
						simulation = null;
						if (lastLoadAsync == null)
						{
							goto IL_0092;
						}
						val = lastLoadAsync().GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 0);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter<Simulation>, <InvokeLoadingAsync>d__20>(ref val, ref this);
							return;
						}
					}
					else
					{
						val = <>u__1;
						<>u__1 = default;
						num = (<>1__state = -1);
					}
					simulation = val.GetResult();
					goto IL_0092;
					IL_0092:
					if (simulation == null)
					{
						OnLoadEnd?.Invoke(arg1: false, new Exception("The simulation you are trying to load does not exist anymore."));
					}
					else
					{
						PendingSimulation = simulation;
					}
				}
				catch (Exception ex)
				{
					LoadingSummary.Clear();
					Debug.LogException(ex);
					OnLoadEnd?.Invoke(arg1: false, ex);
				}
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <LoadSim>d__12 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Func<UniTask<Simulation>> loadHandler;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_0052: Unknown result type (might be due to invalid IL or missing references)
			//IL_0057: Unknown result type (might be due to invalid IL or missing references)
			//IL_005e: Unknown result type (might be due to invalid IL or missing references)
			//IL_001a: Unknown result type (might be due to invalid IL or missing references)
			//IL_001f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0022: Unknown result type (might be due to invalid IL or missing references)
			//IL_0027: Unknown result type (might be due to invalid IL or missing references)
			//IL_003b: Unknown result type (might be due to invalid IL or missing references)
			//IL_003c: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				Awaiter val2;
				if (num != 0)
				{
					UniTask val = LoadSim(loadHandler, wasReload, SkipSimLoadConfirmation);
					val2 = val.GetAwaiter();
					if (!val2.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val2;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSim>d__12>(ref val2, ref this);
						return;
					}
				}
				else
				{
					val2 = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
				}
				val2.GetResult();
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <LoadSim>d__15 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public bool reload;

		public bool fromAutomation;

		public Func<UniTask<Simulation>> loadHandler;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_00b2: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b7: Unknown result type (might be due to invalid IL or missing references)
			//IL_00be: Unknown result type (might be due to invalid IL or missing references)
			//IL_013b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0140: Unknown result type (might be due to invalid IL or missing references)
			//IL_0147: Unknown result type (might be due to invalid IL or missing references)
			//IL_019d: Unknown result type (might be due to invalid IL or missing references)
			//IL_01a2: Unknown result type (might be due to invalid IL or missing references)
			//IL_01a9: Unknown result type (might be due to invalid IL or missing references)
			//IL_0165: Unknown result type (might be due to invalid IL or missing references)
			//IL_016a: Unknown result type (might be due to invalid IL or missing references)
			//IL_016d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0172: Unknown result type (might be due to invalid IL or missing references)
			//IL_0100: Unknown result type (might be due to invalid IL or missing references)
			//IL_0105: Unknown result type (might be due to invalid IL or missing references)
			//IL_0108: Unknown result type (might be due to invalid IL or missing references)
			//IL_010d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0186: Unknown result type (might be due to invalid IL or missing references)
			//IL_0187: Unknown result type (might be due to invalid IL or missing references)
			//IL_0121: Unknown result type (might be due to invalid IL or missing references)
			//IL_0122: Unknown result type (might be due to invalid IL or missing references)
			//IL_0077: Unknown result type (might be due to invalid IL or missing references)
			//IL_007c: Unknown result type (might be due to invalid IL or missing references)
			//IL_007f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0084: Unknown result type (might be due to invalid IL or missing references)
			//IL_0098: Unknown result type (might be due to invalid IL or missing references)
			//IL_0099: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				UniTask val2;
				Awaiter val;
				switch (num)
				{
				default:
					if (!IsLoading)
					{
						wasReload = reload;
						if (!wasReload)
						{
							Preset.CurrentPreset = null;
						}
						OnSimLoadBegin?.Invoke();
						val2 = UniTask.WaitWhile((Func<bool>)(() => WinProgress.IsAwaitingCapture), (PlayerLoopTiming)8, default(CancellationToken), false);
						val = val2.GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 0);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSim>d__15>(ref val, ref this);
							return;
						}
						goto IL_00cd;
					}
					goto end_IL_0007;
				case 0:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_00cd;
				case 1:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_0156;
				case 2:
					{
						val = <>u__1;
						<>u__1 = default;
						num = (<>1__state = -1);
						break;
					}
					IL_0156:
					val.GetResult();
					goto end_IL_0007;
					IL_00cd:
					val.GetResult();
					if (wasReload)
					{
						OnSimReloadStarted?.Invoke();
					}
					Terminal.Event(TerminalEvent.SimLoadBegin);
					if (!fromAutomation)
					{
						val2 = LoadSimTask(loadHandler);
						val = val2.GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 1);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSim>d__15>(ref val, ref this);
							return;
						}
						goto IL_0156;
					}
					val2 = LoadSimInternal(loadHandler);
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 2);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSim>d__15>(ref val, ref this);
						return;
					}
					break;
				}
				val.GetResult();
				end_IL_0007:;
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <LoadSimAsync>d__189 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public bool wasReload;

		public string file;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_0064: Unknown result type (might be due to invalid IL or missing references)
			//IL_0069: Unknown result type (might be due to invalid IL or missing references)
			//IL_0070: Unknown result type (might be due to invalid IL or missing references)
			//IL_002c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0031: Unknown result type (might be due to invalid IL or missing references)
			//IL_0034: Unknown result type (might be due to invalid IL or missing references)
			//IL_0039: Unknown result type (might be due to invalid IL or missing references)
			//IL_004d: Unknown result type (might be due to invalid IL or missing references)
			//IL_004e: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				Awaiter val2;
				if (num != 0)
				{
					Sandbox.wasReload = wasReload;
					CurrentFile = file;
					UniTask val = LoadSim((Func<UniTask<Simulation>>)FromCurrentFileAsync);
					val2 = val.GetAwaiter();
					if (!val2.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val2;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimAsync>d__189>(ref val2, ref this);
						return;
					}
				}
				else
				{
					val2 = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
				}
				val2.GetResult();
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <LoadSimAutomated>d__13 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Func<UniTask<Simulation>> loadHandler;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_004a: Unknown result type (might be due to invalid IL or missing references)
			//IL_004f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0056: Unknown result type (might be due to invalid IL or missing references)
			//IL_0012: Unknown result type (might be due to invalid IL or missing references)
			//IL_0017: Unknown result type (might be due to invalid IL or missing references)
			//IL_001a: Unknown result type (might be due to invalid IL or missing references)
			//IL_001f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0033: Unknown result type (might be due to invalid IL or missing references)
			//IL_0034: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				Awaiter val2;
				if (num != 0)
				{
					UniTask val = LoadSim(loadHandler, reload: false, fromAutomation: true);
					val2 = val.GetAwaiter();
					if (!val2.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val2;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimAutomated>d__13>(ref val2, ref this);
						return;
					}
				}
				else
				{
					val2 = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
				}
				val2.GetResult();
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <LoadSimCoroutine>d__199 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Func<Simulation> loadHandler;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_005b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0060: Unknown result type (might be due to invalid IL or missing references)
			//IL_0067: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ce: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d3: Unknown result type (might be due to invalid IL or missing references)
			//IL_00da: Unknown result type (might be due to invalid IL or missing references)
			//IL_0152: Unknown result type (might be due to invalid IL or missing references)
			//IL_0157: Unknown result type (might be due to invalid IL or missing references)
			//IL_015e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0093: Unknown result type (might be due to invalid IL or missing references)
			//IL_0098: Unknown result type (might be due to invalid IL or missing references)
			//IL_009b: Unknown result type (might be due to invalid IL or missing references)
			//IL_00a0: Unknown result type (might be due to invalid IL or missing references)
			//IL_0020: Unknown result type (might be due to invalid IL or missing references)
			//IL_0025: Unknown result type (might be due to invalid IL or missing references)
			//IL_0028: Unknown result type (might be due to invalid IL or missing references)
			//IL_002d: Unknown result type (might be due to invalid IL or missing references)
			//IL_011a: Unknown result type (might be due to invalid IL or missing references)
			//IL_011f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0122: Unknown result type (might be due to invalid IL or missing references)
			//IL_0127: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b4: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b5: Unknown result type (might be due to invalid IL or missing references)
			//IL_0041: Unknown result type (might be due to invalid IL or missing references)
			//IL_0042: Unknown result type (might be due to invalid IL or missing references)
			//IL_013b: Unknown result type (might be due to invalid IL or missing references)
			//IL_013c: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				UniTask val2;
				Awaiter val;
				switch (num)
				{
				default:
					if (GuideManager.IsGuideActive)
					{
						val2 = GuideManager.PromptForClose();
						val = val2.GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 0);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimCoroutine>d__199>(ref val, ref this);
							return;
						}
						goto IL_0076;
					}
					goto IL_008a;
				case 0:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_0076;
				case 1:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_00e9;
				case 2:
					{
						val = <>u__1;
						<>u__1 = default;
						num = (<>1__state = -1);
						break;
					}
					IL_008a:
					val2 = Quicksave.AutosaveAsync(null);
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 1);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimCoroutine>d__199>(ref val, ref this);
						return;
					}
					goto IL_00e9;
					IL_00e9:
					val.GetResult();
					val2 = UniTask.WaitUntil((Func<bool>)(() => !Quicksave.IsBusy), (PlayerLoopTiming)8, default(CancellationToken), false);
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 2);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimCoroutine>d__199>(ref val, ref this);
						return;
					}
					break;
					IL_0076:
					val.GetResult();
					if (GuideManager.CloseResult == UIDialogResult.OK)
					{
						goto IL_008a;
					}
					goto end_IL_0007;
				}
				val.GetResult();
				LoadSimInternal(loadHandler);
				end_IL_0007:;
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <LoadSimInternal>d__17 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Func<UniTask<Simulation>> loadHandler;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_0051: Unknown result type (might be due to invalid IL or missing references)
			//IL_0056: Unknown result type (might be due to invalid IL or missing references)
			//IL_005d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0019: Unknown result type (might be due to invalid IL or missing references)
			//IL_001e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0021: Unknown result type (might be due to invalid IL or missing references)
			//IL_0026: Unknown result type (might be due to invalid IL or missing references)
			//IL_003a: Unknown result type (might be due to invalid IL or missing references)
			//IL_003b: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				Awaiter val2;
				if (num != 0)
				{
					UniTask val = LoadSimInternal(loadHandler, null);
					val2 = val.GetAwaiter();
					if (!val2.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val2;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimInternal>d__17>(ref val2, ref this);
						return;
					}
				}
				else
				{
					val2 = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
				}
				val2.GetResult();
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <LoadSimInternal>d__18 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Func<UniTask<Simulation>> loadHandler;

		public Nullable autosaveTask;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_009e: Unknown result type (might be due to invalid IL or missing references)
			//IL_00a3: Unknown result type (might be due to invalid IL or missing references)
			//IL_00aa: Unknown result type (might be due to invalid IL or missing references)
			//IL_0121: Unknown result type (might be due to invalid IL or missing references)
			//IL_0126: Unknown result type (might be due to invalid IL or missing references)
			//IL_012d: Unknown result type (might be due to invalid IL or missing references)
			//IL_01a8: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ad: Unknown result type (might be due to invalid IL or missing references)
			//IL_01b4: Unknown result type (might be due to invalid IL or missing references)
			//IL_021d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0222: Unknown result type (might be due to invalid IL or missing references)
			//IL_0229: Unknown result type (might be due to invalid IL or missing references)
			//IL_0279: Unknown result type (might be due to invalid IL or missing references)
			//IL_027e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0285: Unknown result type (might be due to invalid IL or missing references)
			//IL_0063: Unknown result type (might be due to invalid IL or missing references)
			//IL_0068: Unknown result type (might be due to invalid IL or missing references)
			//IL_006b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0070: Unknown result type (might be due to invalid IL or missing references)
			//IL_016d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0172: Unknown result type (might be due to invalid IL or missing references)
			//IL_0175: Unknown result type (might be due to invalid IL or missing references)
			//IL_017a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0084: Unknown result type (might be due to invalid IL or missing references)
			//IL_0085: Unknown result type (might be due to invalid IL or missing references)
			//IL_018e: Unknown result type (might be due to invalid IL or missing references)
			//IL_018f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0241: Unknown result type (might be due to invalid IL or missing references)
			//IL_0246: Unknown result type (might be due to invalid IL or missing references)
			//IL_0249: Unknown result type (might be due to invalid IL or missing references)
			//IL_024e: Unknown result type (might be due to invalid IL or missing references)
			//IL_01e2: Unknown result type (might be due to invalid IL or missing references)
			//IL_01e7: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ea: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ef: Unknown result type (might be due to invalid IL or missing references)
			//IL_0262: Unknown result type (might be due to invalid IL or missing references)
			//IL_0263: Unknown result type (might be due to invalid IL or missing references)
			//IL_0203: Unknown result type (might be due to invalid IL or missing references)
			//IL_0204: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e6: Unknown result type (might be due to invalid IL or missing references)
			//IL_00eb: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ee: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f3: Unknown result type (might be due to invalid IL or missing references)
			//IL_0107: Unknown result type (might be due to invalid IL or missing references)
			//IL_0108: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				UniTask val2;
				Awaiter val;
				switch (num)
				{
				default:
					SkipSimLoadConfirmation = false;
					IsLoading = true;
					lastLoadAsync = loadHandler;
					Watchdog.Start(Watchdog.Strings.loadSim, updateOnlyOnStop: true);
					LoadStopwatch.Reset();
					LoadStopwatch.Start();
					if (OnLoadBeginAsync != null)
					{
						val2 = OnLoadBeginAsync.InvokeAllAsync();
						val = val2.GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 0);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimInternal>d__18>(ref val, ref this);
							return;
						}
						goto IL_00b9;
					}
					goto IL_00c0;
				case 0:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_00b9;
				case 1:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_013c;
				case 2:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_01c3;
				case 3:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_0238;
				case 4:
					{
						val = <>u__1;
						<>u__1 = default;
						num = (<>1__state = -1);
						break;
					}
					IL_01c3:
					val.GetResult();
					goto IL_01ca;
					IL_0238:
					val.GetResult();
					goto end_IL_0007;
					IL_00b9:
					val.GetResult();
					goto IL_00c0;
					IL_00c0:
					OnLoadBegin?.Invoke();
					if (autosaveTask.HasValue)
					{
						val2 = autosaveTask.Value;
						val = val2.GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 1);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimInternal>d__18>(ref val, ref this);
							return;
						}
						goto IL_013c;
					}
					goto IL_01ca;
					IL_01ca:
					if (LoadSimHandlerAsync != null)
					{
						val2 = LoadSimHandlerAsync(InvokeLoadingAsync);
						val = val2.GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 3);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimInternal>d__18>(ref val, ref this);
							return;
						}
						goto IL_0238;
					}
					val2 = InvokeLoadingAsync();
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 4);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimInternal>d__18>(ref val, ref this);
						return;
					}
					break;
					IL_013c:
					val.GetResult();
					val2 = UniTask.WaitUntil((Func<bool>)(() => !Quicksave.IsBusy), (PlayerLoopTiming)8, default(CancellationToken), false);
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 2);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimInternal>d__18>(ref val, ref this);
						return;
					}
					goto IL_01c3;
				}
				val.GetResult();
				end_IL_0007:;
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <LoadSimTask>d__16 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Func<UniTask<Simulation>> loadHandler;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_0053: Unknown result type (might be due to invalid IL or missing references)
			//IL_0058: Unknown result type (might be due to invalid IL or missing references)
			//IL_005f: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d5: Unknown result type (might be due to invalid IL or missing references)
			//IL_00da: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e1: Unknown result type (might be due to invalid IL or missing references)
			//IL_008b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0090: Unknown result type (might be due to invalid IL or missing references)
			//IL_0097: Unknown result type (might be due to invalid IL or missing references)
			//IL_009d: Unknown result type (might be due to invalid IL or missing references)
			//IL_00a2: Unknown result type (might be due to invalid IL or missing references)
			//IL_00a5: Unknown result type (might be due to invalid IL or missing references)
			//IL_00aa: Unknown result type (might be due to invalid IL or missing references)
			//IL_0018: Unknown result type (might be due to invalid IL or missing references)
			//IL_001d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0020: Unknown result type (might be due to invalid IL or missing references)
			//IL_0025: Unknown result type (might be due to invalid IL or missing references)
			//IL_00be: Unknown result type (might be due to invalid IL or missing references)
			//IL_00bf: Unknown result type (might be due to invalid IL or missing references)
			//IL_0039: Unknown result type (might be due to invalid IL or missing references)
			//IL_003a: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				Awaiter val;
				UniTask val2;
				if (num != 0)
				{
					if (num == 1)
					{
						val = <>u__1;
						<>u__1 = default;
						num = (<>1__state = -1);
						goto IL_00f0;
					}
					if (!GuideManager.IsGuideActive)
					{
						goto IL_0082;
					}
					val2 = GuideManager.PromptForClose();
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimTask>d__16>(ref val, ref this);
						return;
					}
				}
				else
				{
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
				}
				val.GetResult();
				if (GuideManager.CloseResult == UIDialogResult.OK)
				{
					goto IL_0082;
				}
				goto end_IL_0007;
				IL_00f0:
				val.GetResult();
				goto end_IL_0007;
				IL_0082:
				UniTask value = Quicksave.AutosaveAsync(null);
				val2 = LoadSimInternal(loadHandler, value);
				val = val2.GetAwaiter();
				if (!val.IsCompleted)
				{
					num = (<>1__state = 1);
					<>u__1 = val;
					<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <LoadSimTask>d__16>(ref val, ref this);
					return;
				}
				goto IL_00f0;
				end_IL_0007:;
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <MergeSim>d__19 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Func<UniTask<Simulation>> loadHandler;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_004e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0053: Unknown result type (might be due to invalid IL or missing references)
			//IL_005a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0016: Unknown result type (might be due to invalid IL or missing references)
			//IL_001b: Unknown result type (might be due to invalid IL or missing references)
			//IL_001e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0023: Unknown result type (might be due to invalid IL or missing references)
			//IL_0037: Unknown result type (might be due to invalid IL or missing references)
			//IL_0038: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				Awaiter val2;
				if (num != 0)
				{
					IsMerge = true;
					UniTask val = LoadSim(loadHandler);
					val2 = val.GetAwaiter();
					if (!val2.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val2;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <MergeSim>d__19>(ref val2, ref this);
						return;
					}
				}
				else
				{
					val2 = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
				}
				val2.GetResult();
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <OnFirstUpdate>d__172 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Sandbox <>4__this;

		private bool <showWhatsNew>5__2;

		private WinWhatsNew.ContentType <whatsNewContent>5__3;

		private bool <doRunIntro>5__4;

		private Awaiter <>u__1;

		private Awaiter <>u__2;

		private IPreloadOnStart[] <>7__wrap4;

		private int <>7__wrap5;

		private void MoveNext()
		{
			//IL_0067: Unknown result type (might be due to invalid IL or missing references)
			//IL_006c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0074: Unknown result type (might be due to invalid IL or missing references)
			//IL_0315: Unknown result type (might be due to invalid IL or missing references)
			//IL_031a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0322: Unknown result type (might be due to invalid IL or missing references)
			//IL_03b1: Unknown result type (might be due to invalid IL or missing references)
			//IL_03b6: Unknown result type (might be due to invalid IL or missing references)
			//IL_03be: Unknown result type (might be due to invalid IL or missing references)
			//IL_04a2: Unknown result type (might be due to invalid IL or missing references)
			//IL_04a7: Unknown result type (might be due to invalid IL or missing references)
			//IL_04af: Unknown result type (might be due to invalid IL or missing references)
			//IL_051a: Unknown result type (might be due to invalid IL or missing references)
			//IL_051f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0527: Unknown result type (might be due to invalid IL or missing references)
			//IL_0029: Unknown result type (might be due to invalid IL or missing references)
			//IL_002e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0032: Unknown result type (might be due to invalid IL or missing references)
			//IL_0037: Unknown result type (might be due to invalid IL or missing references)
			//IL_004c: Unknown result type (might be due to invalid IL or missing references)
			//IL_004e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0373: Unknown result type (might be due to invalid IL or missing references)
			//IL_0378: Unknown result type (might be due to invalid IL or missing references)
			//IL_037c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0381: Unknown result type (might be due to invalid IL or missing references)
			//IL_0407: Unknown result type (might be due to invalid IL or missing references)
			//IL_04df: Unknown result type (might be due to invalid IL or missing references)
			//IL_04e4: Unknown result type (might be due to invalid IL or missing references)
			//IL_04e8: Unknown result type (might be due to invalid IL or missing references)
			//IL_04ed: Unknown result type (might be due to invalid IL or missing references)
			//IL_00de: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d6: Unknown result type (might be due to invalid IL or missing references)
			//IL_00dc: Invalid comparison between Unknown and I4
			//IL_0396: Unknown result type (might be due to invalid IL or missing references)
			//IL_0398: Unknown result type (might be due to invalid IL or missing references)
			//IL_0502: Unknown result type (might be due to invalid IL or missing references)
			//IL_0504: Unknown result type (might be due to invalid IL or missing references)
			//IL_0464: Unknown result type (might be due to invalid IL or missing references)
			//IL_0469: Unknown result type (might be due to invalid IL or missing references)
			//IL_046d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0472: Unknown result type (might be due to invalid IL or missing references)
			//IL_0487: Unknown result type (might be due to invalid IL or missing references)
			//IL_0489: Unknown result type (might be due to invalid IL or missing references)
			//IL_029d: Unknown result type (might be due to invalid IL or missing references)
			//IL_01fc: Unknown result type (might be due to invalid IL or missing references)
			//IL_02d7: Unknown result type (might be due to invalid IL or missing references)
			//IL_02dc: Unknown result type (might be due to invalid IL or missing references)
			//IL_02e0: Unknown result type (might be due to invalid IL or missing references)
			//IL_02e5: Unknown result type (might be due to invalid IL or missing references)
			//IL_02fa: Unknown result type (might be due to invalid IL or missing references)
			//IL_02fc: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			Sandbox sandbox = <>4__this;
			try
			{
				YieldAwaitable val3;
				Awaiter val;
				Awaiter val2;
				bool flag;
				string version;
				int buildRevision;
				int optionsVersion;
				string version2;
				bool hasRunIntro;
				UniTask val4;
				IPreloadOnStart[] componentsInChildren;
				switch (num)
				{
				default:
					val3 = UniTask.Yield((PlayerLoopTiming)5);
					val = val3.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <OnFirstUpdate>d__172>(ref val, ref this);
						return;
					}
					goto IL_0083;
				case 0:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_0083;
				case 1:
					val2 = <>u__2;
					<>u__2 = default;
					num = (<>1__state = -1);
					goto IL_0331;
				case 2:
					val2 = <>u__2;
					<>u__2 = default;
					num = (<>1__state = -1);
					goto IL_03cd;
				case 3:
					val2 = <>u__2;
					<>u__2 = default;
					num = (<>1__state = -1);
					goto IL_04be;
				case 4:
					{
						val = <>u__1;
						<>u__1 = default;
						num = (<>1__state = -1);
						goto IL_0536;
					}
					IL_0536:
					val.GetResult();
					goto IL_053d;
					IL_0083:
					val.GetResult();
					if (ShowExitBecauseNoDRM && BuildInfo.BUILDTYPE == BuildType.Steam && OnSteamWarning != null)
					{
						OnSteamWarning();
					}
					flag = DeviceInfo.systemMemorySize >= 3686;
					if (!DeviceInfo.IsMobile)
					{
						flag &= DeviceInfo.graphicsMemorySize >= 3686;
					}
					if ((flag && (int)Application.platform == 1) || (int)Application.platform == 0)
					{
						flag &= DeviceInfo.IsMacSilicon();
					}
					if (!flag && OnSpecs != null)
					{
						OnSpecs();
					}
					version = Preferences.Instance.Version;
					buildRevision = Preferences.Instance.BuildRevision;
					optionsVersion = Preferences.Instance.OptionsVersion;
					version2 = Readable.VersionVariableNameAndDateAndCommitCount();
					hasRunIntro = Preferences.Instance.HasRunIntro;
					<showWhatsNew>5__2 = false;
					<whatsNewContent>5__3 = WinWhatsNew.ContentType.Major;
					if (buildRevision < BuildInfo.VERSION_COMMIT_COUNT)
					{
						<showWhatsNew>5__2 = !Readable.IsVersionNamedMatch(version);
						if (buildRevision < BuildInfo.SHOW_WHATSNEW_IF_LESS_THAN_COMMIT_COUNT)
						{
							<whatsNewContent>5__3 = WinWhatsNew.ContentType.Major;
						}
						else
						{
							<whatsNewContent>5__3 = WinWhatsNew.ContentType.Minor;
						}
					}
					if (optionsVersion != BuildInfo.VERSION_OPTIONS)
					{
						Debug.LogWarning((object)"Incompatible Options Version found - Resetting to default values");
						Preferences.ResetPlayerPrefs();
						Preferences.Instance.Reset();
						Preferences.Instance.HasRunIntro = hasRunIntro;
						if (DeviceInfo.IsVR)
						{
							Preferences.Instance.RunCountVR++;
						}
						else
						{
							Preferences.Instance.RunCount++;
						}
						Settings.Global.ResetToDefaults();
						Settings.Global.AutoSetGraphicsQuality();
						Settings.Global.AutoSetSimQuality();
						UniTaskExtensions.Forget(Settings.Global.SaveTask());
					}
					<doRunIntro>5__4 = false;
					if (!hasRunIntro)
					{
						<showWhatsNew>5__2 = false;
						hasRunIntro = true;
						<doRunIntro>5__4 = true;
					}
					if (!DateTime.TryParse(Preferences.Instance.BuildDate, out StartupBuildDate))
					{
						StartupBuildDate = DateTime.Now;
					}
					Preferences.Instance.Version = version2;
					Preferences.Instance.OptionsVersion = BuildInfo.VERSION_OPTIONS;
					Preferences.Instance.BuildRevision = BuildInfo.VERSION_COMMIT_COUNT;
					Preferences.Instance.BuildDate = BuildInfo.VERSION_DATE.ToString("yyyy-MM-dd", CultureInfo.InvariantCulture);
					Preferences.Instance.BuildHash = BuildInfo.VERSION_COMMIT_HASH_FULL;
					UniTaskExtensions.Forget(Preferences.SaveTask());
					BodySearchUtils.Initialize();
					val4 = UniTask.WaitUntil((Func<bool>)(() => UIPropertyPanel.FinishedInitializingPools), (PlayerLoopTiming)8, default(CancellationToken), false);
					val2 = val4.GetAwaiter();
					if (!val2.IsCompleted)
					{
						num = (<>1__state = 1);
						<>u__2 = val2;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <OnFirstUpdate>d__172>(ref val2, ref this);
						return;
					}
					goto IL_0331;
					IL_053d:
					if (IsLoading || UIManager.Instance.ProgressWindow.Active)
					{
						val3 = UniTask.Yield();
						val = val3.GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 4);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <OnFirstUpdate>d__172>(ref val, ref this);
							return;
						}
						goto IL_0536;
					}
					ShowIntro();
					break;
					IL_03e2:
					if (<>7__wrap5 < <>7__wrap4.Length)
					{
						val4 = <>7__wrap4[<>7__wrap5].Preload();
						val2 = val4.GetAwaiter();
						if (!val2.IsCompleted)
						{
							num = (<>1__state = 2);
							<>u__2 = val2;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <OnFirstUpdate>d__172>(ref val2, ref this);
							return;
						}
						goto IL_03cd;
					}
					<>7__wrap4 = null;
					UIDynamicLayout.Instance.EnableTweens();
					UniTaskExtensions.Forget(sandbox.FinishFrame());
					if ((<showWhatsNew>5__2 && !BuildInfo.SHOW_ONLY_TO_DEVS && BuildInfo.USE_WHATSNEW) || (BuildInfo.TEST_STARTUP_BUILD && BuildInfo.SHOW_ONLY_TO_DEVS) || BuildInfo.FORCE_WHATSNEW)
					{
						OnWhatsNew?.Invoke(<whatsNewContent>5__3);
					}
					DebugUtil.StopTimerItemized("Sandbox", "FIRST UPDATE");
					val4 = UniTask.WaitForEndOfFrame((MonoBehaviour)(object)sandbox);
					val2 = val4.GetAwaiter();
					if (!val2.IsCompleted)
					{
						num = (<>1__state = 3);
						<>u__2 = val2;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <OnFirstUpdate>d__172>(ref val2, ref this);
						return;
					}
					goto IL_04be;
					IL_03cd:
					val2.GetResult();
					<>7__wrap5++;
					goto IL_03e2;
					IL_0331:
					val2.GetResult();
					DebugUtil.StartTimer();
					UIDynamicLayout.Instance.DisableTweens();
					componentsInChildren = ((Component)UIManager.Instance).GetComponentsInChildren<IPreloadOnStart>(true);
					<>7__wrap4 = componentsInChildren;
					<>7__wrap5 = 0;
					goto IL_03e2;
					IL_04be:
					val2.GetResult();
					OnFirstFrameEnded?.Invoke();
					if (<doRunIntro>5__4)
					{
						goto IL_053d;
					}
					break;
				}
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <QuitAsync>d__186 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		private bool <needsAutosave>5__2;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_0090: Unknown result type (might be due to invalid IL or missing references)
			//IL_0095: Unknown result type (might be due to invalid IL or missing references)
			//IL_009c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0100: Unknown result type (might be due to invalid IL or missing references)
			//IL_0105: Unknown result type (might be due to invalid IL or missing references)
			//IL_010c: Unknown result type (might be due to invalid IL or missing references)
			//IL_017e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0183: Unknown result type (might be due to invalid IL or missing references)
			//IL_018a: Unknown result type (might be due to invalid IL or missing references)
			//IL_01e8: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ed: Unknown result type (might be due to invalid IL or missing references)
			//IL_01f4: Unknown result type (might be due to invalid IL or missing references)
			//IL_01b0: Unknown result type (might be due to invalid IL or missing references)
			//IL_01b5: Unknown result type (might be due to invalid IL or missing references)
			//IL_01b8: Unknown result type (might be due to invalid IL or missing references)
			//IL_01bd: Unknown result type (might be due to invalid IL or missing references)
			//IL_0143: Unknown result type (might be due to invalid IL or missing references)
			//IL_0148: Unknown result type (might be due to invalid IL or missing references)
			//IL_014b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0150: Unknown result type (might be due to invalid IL or missing references)
			//IL_0055: Unknown result type (might be due to invalid IL or missing references)
			//IL_005a: Unknown result type (might be due to invalid IL or missing references)
			//IL_005d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0062: Unknown result type (might be due to invalid IL or missing references)
			//IL_00c5: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ca: Unknown result type (might be due to invalid IL or missing references)
			//IL_00cd: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d2: Unknown result type (might be due to invalid IL or missing references)
			//IL_01d1: Unknown result type (might be due to invalid IL or missing references)
			//IL_01d2: Unknown result type (might be due to invalid IL or missing references)
			//IL_0164: Unknown result type (might be due to invalid IL or missing references)
			//IL_0165: Unknown result type (might be due to invalid IL or missing references)
			//IL_0076: Unknown result type (might be due to invalid IL or missing references)
			//IL_0077: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e6: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e7: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				UniTask val2;
				Awaiter val;
				switch (num)
				{
				default:
					if (DeviceInfo.IsVR)
					{
						VRPlayer.FadeOutHeadset();
					}
					<needsAutosave>5__2 = Quicksave.NeedsAutosave();
					if (!<needsAutosave>5__2)
					{
						val2 = EnumeratorAsyncExtensions.ToUniTask(UIManager.Instance.FadeOutScreen(0.5f), (PlayerLoopTiming)8, default(CancellationToken));
						val = val2.GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 0);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <QuitAsync>d__186>(ref val, ref this);
							return;
						}
						goto IL_00ab;
					}
					goto IL_00b2;
				case 0:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_00ab;
				case 1:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_011b;
				case 2:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_0199;
				case 3:
					{
						val = <>u__1;
						<>u__1 = default;
						num = (<>1__state = -1);
						goto IL_0203;
					}
					IL_0199:
					val.GetResult();
					goto IL_01a0;
					IL_0203:
					val.GetResult();
					FileTools.Destroy();
					break;
					IL_011b:
					val.GetResult();
					if (<needsAutosave>5__2)
					{
						val2 = EnumeratorAsyncExtensions.ToUniTask(UIManager.Instance.FadeOutScreen(0.5f), (PlayerLoopTiming)8, default(CancellationToken));
						val = val2.GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 2);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <QuitAsync>d__186>(ref val, ref this);
							return;
						}
						goto IL_0199;
					}
					goto IL_01a0;
					IL_01a0:
					Preferences.Instance.LastRunEndedCleanly = true;
					val2 = Settings.Global.SaveTask();
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 3);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <QuitAsync>d__186>(ref val, ref this);
						return;
					}
					goto IL_0203;
					IL_00ab:
					val.GetResult();
					goto IL_00b2;
					IL_00b2:
					if (SilentAndDontSaveQuit)
					{
						break;
					}
					val2 = Quicksave.AutosaveAsync(null, waitForCompletion: true);
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 1);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <QuitAsync>d__186>(ref val, ref this);
						return;
					}
					goto IL_011b;
				}
				Application.Quit();
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <ReloadSimulationAsync>d__192 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_0058: Unknown result type (might be due to invalid IL or missing references)
			//IL_005d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0064: Unknown result type (might be due to invalid IL or missing references)
			//IL_0020: Unknown result type (might be due to invalid IL or missing references)
			//IL_0025: Unknown result type (might be due to invalid IL or missing references)
			//IL_0028: Unknown result type (might be due to invalid IL or missing references)
			//IL_002d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0041: Unknown result type (might be due to invalid IL or missing references)
			//IL_0042: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				Awaiter val2;
				if (num != 0)
				{
					if (FileTools.IsSavingAsync)
					{
						UIManager.Instance.ProgressWindow.Open();
					}
					UniTask val = ReloadSimulationTask();
					val2 = val.GetAwaiter();
					if (!val2.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val2;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <ReloadSimulationAsync>d__192>(ref val2, ref this);
						return;
					}
				}
				else
				{
					val2 = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
				}
				val2.GetResult();
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <ReloadSimulationTask>d__11 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_0076: Unknown result type (might be due to invalid IL or missing references)
			//IL_007b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0082: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e2: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e7: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ee: Unknown result type (might be due to invalid IL or missing references)
			//IL_00aa: Unknown result type (might be due to invalid IL or missing references)
			//IL_00af: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b2: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b7: Unknown result type (might be due to invalid IL or missing references)
			//IL_003b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0040: Unknown result type (might be due to invalid IL or missing references)
			//IL_0043: Unknown result type (might be due to invalid IL or missing references)
			//IL_0048: Unknown result type (might be due to invalid IL or missing references)
			//IL_00cb: Unknown result type (might be due to invalid IL or missing references)
			//IL_00cc: Unknown result type (might be due to invalid IL or missing references)
			//IL_005c: Unknown result type (might be due to invalid IL or missing references)
			//IL_005d: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				Awaiter val;
				UniTask val2;
				if (num != 0)
				{
					if (num == 1)
					{
						val = <>u__1;
						<>u__1 = default;
						num = (<>1__state = -1);
						goto IL_00fd;
					}
					val2 = UniTask.WaitUntil((Func<bool>)(() => !FileTools.IsSavingAsync), (PlayerLoopTiming)8, default(CancellationToken), false);
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <ReloadSimulationTask>d__11>(ref val, ref this);
						return;
					}
				}
				else
				{
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
				}
				val.GetResult();
				if (reloadHandlerAsync != null)
				{
					val2 = LoadSim(reloadHandlerAsync, reload: true, SkipSimLoadConfirmation);
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 1);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <ReloadSimulationTask>d__11>(ref val, ref this);
						return;
					}
					goto IL_00fd;
				}
				reloadHandler?.Invoke();
				goto end_IL_0007;
				IL_00fd:
				val.GetResult();
				end_IL_0007:;
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <SlowCurrentSim>d__14 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		private float <fadeTime>5__2;

		private float <elapsed>5__3;

		private double <start>5__4;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			//IL_012c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0131: Unknown result type (might be due to invalid IL or missing references)
			//IL_0138: Unknown result type (might be due to invalid IL or missing references)
			//IL_01e4: Unknown result type (might be due to invalid IL or missing references)
			//IL_01e9: Unknown result type (might be due to invalid IL or missing references)
			//IL_01f0: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ac: Unknown result type (might be due to invalid IL or missing references)
			//IL_01b1: Unknown result type (might be due to invalid IL or missing references)
			//IL_01b4: Unknown result type (might be due to invalid IL or missing references)
			//IL_01b9: Unknown result type (might be due to invalid IL or missing references)
			//IL_01cd: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ce: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f1: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f6: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f9: Unknown result type (might be due to invalid IL or missing references)
			//IL_00fe: Unknown result type (might be due to invalid IL or missing references)
			//IL_0112: Unknown result type (might be due to invalid IL or missing references)
			//IL_0113: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			try
			{
				Awaiter val;
				if (num == 0)
				{
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_0147;
				}
				if (num == 1)
				{
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_01ff;
				}
				YieldAwaitable val2;
				if (ActiveSimulation != null && LoadingRoutine.status == LoadingRoutine.Status.EndLoading && !Paused && !SystemPause && ActiveSimulation.TargetTimeStepPerRealSec > 0.0)
				{
					<fadeTime>5__2 = Tweakables.Instance.SimulationSlowdownSeconds * Settings.Global.UITransitionMultiplier;
					bool flag = UIDynamicLayout.Instance.IsViewportAnimating();
					<fadeTime>5__2 = (flag ? Settings.Global.DLSViewportEaseTime : Settings.Global.DLSTransitionTime) * Settings.Global.UITransitionMultiplier;
					<elapsed>5__3 = 0f;
					<start>5__4 = ActiveSimulation.TargetTimeStepPerRealSec;
					ActiveSimulation.SuspendTimestepAveraging = true;
					if (<fadeTime>5__2 <= 0f)
					{
						ActiveSimulation.TargetTimeStepPerRealSec = 0.0;
						val2 = UniTask.Yield();
						val = val2.GetAwaiter();
						if (!val.IsCompleted)
						{
							num = (<>1__state = 0);
							<>u__1 = val;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <SlowCurrentSim>d__14>(ref val, ref this);
							return;
						}
						goto IL_0147;
					}
					goto IL_0206;
				}
				goto end_IL_0007;
				IL_0147:
				val.GetResult();
				goto IL_0217;
				IL_0206:
				if (<elapsed>5__3 <= <fadeTime>5__2)
				{
					<elapsed>5__3 = Mathf.Min(<elapsed>5__3 + Time.unscaledDeltaTime, <fadeTime>5__2 + 0.1f);
					float transitionValue = MaterialEasing.GetTransitionValue(MaterialEasing.TransitionType.AccelerateEasing, <elapsed>5__3, <fadeTime>5__2);
					ActiveSimulation.TargetTimeStepPerRealSec = MathHelper.Lerp(<start>5__4, 0.0, transitionValue);
					val2 = UniTask.Yield();
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 1);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <SlowCurrentSim>d__14>(ref val, ref this);
						return;
					}
					goto IL_01ff;
				}
				goto IL_0217;
				IL_0217:
				ActiveSimulation.TargetTimeStepPerRealSec = 0.0;
				PanelBottomBar.Instance.ForceUpdate();
				goto end_IL_0007;
				IL_01ff:
				val.GetResult();
				goto IL_0206;
				end_IL_0007:;
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	public static Func<Func<UniTask>, UniTask> LoadSimHandlerAsync;

	public static Func<UniTask> OnLoadBeginAsync;

	public static Func<bool, UniTask> OnLoadEndAsync;

	public static Func<UniTask<Simulation>> reloadHandlerAsync;

	private static Func<UniTask<Simulation>> _lastLoadAsync;

	internal static bool StartedByLoadingScene;

	internal static SandboxState _mode;

	private static Simulation _activeSimulation;

	private static BackgroundWorker worker;

	private static Simulation PendingSimulation;

	private static bool wasReload;

	private static bool _paused;

	private static bool _singleStep;

	public static bool FinalQuit;

	public static bool AboutToQuit;

	public static Action<Action> LoadSimHandler;

	public static bool FirstSingleStepCycle;

	public static bool IsLoading;

	public static bool IsMerge;

	private static bool SkipSimLoadConfirmation;

	public static string CurrentFile;

	private static Action reloadHandler;

	private static Func<Simulation> _lastLoad;

	private static string _lastSaveFile;

	public static bool AsyncLoading;

	public static bool ChangeAmbienceOnLoad;

	public static bool ShowExitBecauseNoDRM;

	public static DateTime StartupBuildDate;

	public static CommandSequence CommandSequence;

	public static Stopwatch LoadStopwatch;

	public static List<string> LoadingSummary;

	public static bool LoadingSummaryShowRestore;

	public static YieldInstruction WaitForEndOfFrame;

	public static Vector3D VRSceneCenter;

	public static bool SilentAndDontSaveQuit;

	public bool QuitStarted;

	public TextAsset Script;

	public bool RunScript;

	private static Func<UniTask<Simulation>> lastLoadAsync
	{
		get
		{
			return _lastLoadAsync;
		}
		set
		{
			_lastLoad = null;
			reloadHandler = null;
			_lastLoadAsync = value;
			reloadHandlerAsync = value;
		}
	}

	public static Func<UniTask<Simulation>> ReloadHandlerAsync
	{
		get
		{
			return reloadHandlerAsync;
		}
		set
		{
			reloadHandlerAsync = value;
			reloadHandler = null;
		}
	}

	private static Func<Simulation> lastLoad
	{
		get
		{
			return _lastLoad;
		}
		set
		{
			_lastLoadAsync = null;
			reloadHandlerAsync = null;
			_lastLoad = value;
			reloadHandler = () =>
			{
				LoadSim(value, reload: true, SkipSimLoadConfirmation);
			};
		}
	}

	public static Sandbox Instance { get; private set; }

	public static Protocol Protocol { get; private set; }

	public static bool IsQuitting
	{
		get
		{
			if (!AboutToQuit)
			{
				return FinalQuit;
			}
			return true;
		}
	}

	public static Simulation ActiveSimulation
	{
		get
		{
			return _activeSimulation;
		}
		set
		{
			if (_activeSimulation != null)
			{
				_activeSimulation.Dispose();
			}
			_activeSimulation = value;
			if (_activeSimulation != null)
			{
				_activeSimulation.IsActive = true;
			}
		}
	}

	public static Clipboard Clipboard { get; private set; }

	public static WorkerProgress Progress { get; private set; }

	public static bool IsDemoBuild { get; private set; }

	public static bool IsDemoMode => CommandSequence != null;

	public static bool Paused
	{
		get
		{
			if (_paused)
			{
				return !_singleStep;
			}
			return false;
		}
		set
		{
			_paused = value;
			Terminal.Event(_paused ? TerminalEvent.SimPaused : TerminalEvent.SimStarted);
		}
	}

	public static bool SystemPause { get; set; }

	public static bool WasSingleStep { get; set; }

	public static bool SingleStep
	{
		get
		{
			return _singleStep;
		}
		set
		{
			FirstSingleStepCycle = value;
			_singleStep = value;
			if (_singleStep)
			{
				WasSingleStep = true;
			}
		}
	}

	public static SandboxState Mode
	{
		get
		{
			return _mode;
		}
		set
		{
			if (_mode != value)
			{
				_mode = value;
			}
		}
	}

	public static event Action OnFirstFrameEnded;

	public static event Action OnFrameEnded;

	public static event Action OnStartScript;

	public static event Action OnSteamWarning;

	public static event Action OnEpicWarning;

	public static event Action OnSpecs;

	public static event Action<WinWhatsNew.ContentType> OnWhatsNew;

	public static event Action OnIntro;

	public static event Func<bool> OnNeedsSurvey;

	public static event Action OnSurvey;

	public static event Action OnExit;

	public static event Action OnLoadBegin;

	public static event Action OnOptionsLoaded;

	public static event Action OnSimReloadStarted;

	public static event Action OnSimLoadBegin;

	public static event Action<bool> OnSimLoading;

	public static event Action<bool> OnSimLoaded;

	public static event Action<bool, Exception> OnLoadEnd;

	public static event Action OnBeforeCameraSetup;

	[AsyncStateMachine(typeof(<ReloadSimulationTask>d__11))]
	private static UniTask ReloadSimulationTask()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0029: Unknown result type (might be due to invalid IL or missing references)
		<ReloadSimulationTask>d__11 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<ReloadSimulationTask>d__11>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<LoadSim>d__12))]
	public static UniTask LoadSim(Func<UniTask<Simulation>> loadHandler)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		<LoadSim>d__12 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.loadHandler = loadHandler;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<LoadSim>d__12>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<LoadSimAutomated>d__13))]
	public static UniTask LoadSimAutomated(Func<UniTask<Simulation>> loadHandler)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		<LoadSimAutomated>d__13 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.loadHandler = loadHandler;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<LoadSimAutomated>d__13>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<SlowCurrentSim>d__14))]
	public static UniTask SlowCurrentSim()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0029: Unknown result type (might be due to invalid IL or missing references)
		<SlowCurrentSim>d__14 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<SlowCurrentSim>d__14>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<LoadSim>d__15))]
	private static UniTask LoadSim(Func<UniTask<Simulation>> loadHandler, bool reload, bool fromAutomation)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0041: Unknown result type (might be due to invalid IL or missing references)
		<LoadSim>d__15 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.loadHandler = loadHandler;
		obj.reload = reload;
		obj.fromAutomation = fromAutomation;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<LoadSim>d__15>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<LoadSimTask>d__16))]
	private static UniTask LoadSimTask(Func<UniTask<Simulation>> loadHandler)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		<LoadSimTask>d__16 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.loadHandler = loadHandler;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<LoadSimTask>d__16>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<LoadSimInternal>d__17))]
	private static UniTask LoadSimInternal(Func<UniTask<Simulation>> loadHandler)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		<LoadSimInternal>d__17 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.loadHandler = loadHandler;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<LoadSimInternal>d__17>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<LoadSimInternal>d__18))]
	private static UniTask LoadSimInternal(Func<UniTask<Simulation>> loadHandler, Nullable autosaveTask)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0039: Unknown result type (might be due to invalid IL or missing references)
		<LoadSimInternal>d__18 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.loadHandler = loadHandler;
		obj.autosaveTask = autosaveTask;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<LoadSimInternal>d__18>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<MergeSim>d__19))]
	public static UniTask MergeSim(Func<UniTask<Simulation>> loadHandler)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		<MergeSim>d__19 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.loadHandler = loadHandler;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<MergeSim>d__19>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<InvokeLoadingAsync>d__20))]
	private static UniTask InvokeLoadingAsync()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0029: Unknown result type (might be due to invalid IL or missing references)
		<InvokeLoadingAsync>d__20 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<InvokeLoadingAsync>d__20>(ref obj);
		return obj.<>t__builder.Task;
	}

	[RuntimeInitializeOnLoadMethod(/*Could not decode attribute arguments.*/)]
	private static void OnRuntimeInitializeLoadSubsys()
	{
		//IL_014a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0154: Expected Obj, but got Unknown
		OnFirstFrameEnded = null;
		OnFrameEnded = null;
		OnStartScript = null;
		OnSteamWarning = null;
		OnEpicWarning = null;
		OnSpecs = null;
		OnWhatsNew = null;
		OnIntro = null;
		OnNeedsSurvey = null;
		OnSurvey = null;
		OnExit = null;
		OnLoadBegin = null;
		OnOptionsLoaded = null;
		OnSimReloadStarted = null;
		OnSimLoadBegin = null;
		OnSimLoading = null;
		OnSimLoaded = null;
		OnLoadEnd = null;
		Instance = null;
		reloadHandler = null;
		_lastLoad = null;
		Progress = null;
		CommandSequence = null;
		LoadSimHandler = null;
		_activeSimulation = null;
		worker = null;
		PendingSimulation = null;
		StartedByLoadingScene = false;
		wasReload = false;
		_paused = false;
		_singleStep = false;
		FinalQuit = false;
		AboutToQuit = false;
		FirstSingleStepCycle = false;
		IsLoading = false;
		IsMerge = false;
		SkipSimLoadConfirmation = false;
		AsyncLoading = true;
		ChangeAmbienceOnLoad = true;
		ShowExitBecauseNoDRM = false;
		LoadingSummaryShowRestore = true;
		SilentAndDontSaveQuit = false;
		IsDemoBuild = false;
		SystemPause = false;
		WasSingleStep = false;
		CurrentFile = string.Empty;
		_lastSaveFile = string.Empty;
		Clipboard = new Clipboard();
		Protocol = new Protocol();
		LoadStopwatch = new Stopwatch();
		LoadingSummary = new List<string>();
		WaitForEndOfFrame = (YieldInstruction)new WaitForEndOfFrame();
		VRSceneCenter = Vector3D.Zero;
		_mode = SandboxState.Simulate;
	}

	static Sandbox()
	{
		//IL_002c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0036: Expected Obj, but got Unknown
		AsyncLoading = true;
		ChangeAmbienceOnLoad = true;
		ShowExitBecauseNoDRM = false;
		LoadStopwatch = new Stopwatch();
		LoadingSummary = new List<string>();
		LoadingSummaryShowRestore = true;
		WaitForEndOfFrame = (YieldInstruction)new WaitForEndOfFrame();
		Clipboard = new Clipboard();
		Protocol = new Protocol();
	}

	public static void NoOp()
	{
	}

	[RuntimeInitializeOnLoadMethod]
	private static void StartupSandbox()
	{
		Application.wantsToQuit += WantsToQuit;
	}

	private void OnSkipSimLoadConfirmation(Message msg)
	{
		SkipSimLoadConfirmation = (bool)msg.Data;
	}

	private void Start()
	{
		//IL_00f3: Unknown result type (might be due to invalid IL or missing references)
		Instance = this;
		if (!StartedByLoadingScene)
		{
			Debug.LogError((object)"FATAL: Entering play mode from LoadingScene is now required");
			Debug.Break();
		}
		DebugUtil.StartTimer();
		MessageDispatcher.AddListener("LoadSimulationFile", (Action<Message>)((Message msg) =>
		{
			//IL_001c: Unknown result type (might be due to invalid IL or missing references)
			Debug.Log((object)msg.Data.ToString());
			UniTaskExtensions.Forget(LoadSimAsync(msg.Data.ToString()));
		}));
		MessageDispatcher.AddListener("SkipSimLoadConfirmation", (Action<Message>)OnSkipSimLoadConfirmation);
		BulletinBoard.TakeOwnership();
		FastRandom.Seed((int)DateTime.Now.Ticks);
		DebugUtil.StopTimerItemized("Sandbox.Start", "MISC");
		DateTime now = DateTime.Now;
		ActiveSimulation = new Simulation();
		DebugUtil.LogTime("SIMULATION CTOR", now);
		OnLoadBegin += Sandbox_OnLoadBegin;
		if (RunScript && (Object)(object)Script != (Object)null)
		{
			IsDemoBuild = true;
			CommandSequence = new CommandSequence(Script.text);
		}
		UniTaskExtensions.Forget(OnFirstUpdate());
	}

	private void Sandbox_OnLoadBegin()
	{
		Settings.Ambience.AmbientLight = AmbientLightMode.Automatic;
	}

	public static void FireOnOptionsLoaded()
	{
		if (OnOptionsLoaded != null)
		{
			OnOptionsLoaded();
		}
	}

	public void SetupVRCamera()
	{
		((MonoBehaviour)this).StartCoroutine(VRCameraSetup());
	}

	private IEnumerator VRCameraSetup()
	{
		MessageDispatcher.Send("AttachVRCameraToBody", (object[])null);
		Vector3D vector3D = new Vector3D(double.MaxValue, double.MaxValue, double.MaxValue);
		Vector3D vector3D2 = new Vector3D(double.MinValue, double.MinValue, double.MinValue);
		bool flag = false;
		foreach (Body body in ActiveSimulation.Bodies)
		{
			if (!body.Suspended && !body.IsDead)
			{
				flag = true;
			}
			vector3D = Vector3D.Min(vector3D, body.Position - Vector3D.One * body.Radius);
			vector3D2 = Vector3D.Max(vector3D2, body.Position + Vector3D.One * body.Radius);
		}
		Vector3D vector3D3 = (vector3D + vector3D2) * 0.5;
		double num = Vector3D.Distance(vector3D, vector3D2);
		Body target = ActiveSimulation.Settings.CameraTarget;
		Vector3D position = ActiveSimulation.Settings.CameraPosition;
		VRSceneCenter = target?.Position ?? vector3D3;
		if (position != Vector3D.Zero)
		{
			if (!flag)
			{
				VirtualSpace.ScaleFactor = 1.56961230576048E-08;
			}
			else if (ActiveSimulation.Settings.CameraTargetDistance != 0.0)
			{
				VirtualSpace.ScaleFactor = Math.Min(1.0, 1.0 / ActiveSimulation.Settings.CameraTargetDistance);
			}
			else
			{
				VirtualSpace.ScaleFactor = Math.Min(1.0, 1.0 / (num / 2.0));
			}
			yield return (object)new WaitForEndOfFrame();
			if ((Object)(object)DashManager.instance != (Object)null)
			{
				Vector3D vector3D4 = ((Component)DashManager.instance.dock).transform.localPosition.ToVector3D() / VirtualSpace.ScaleFactor;
				VRLocomotion.Instance.SetPosition(position - vector3D4);
			}
			else
			{
				Vector3D vector3D5 = Camera6DOF.Instance.roomTranslation.ToVector3D() / VirtualSpace.ScaleFactor;
				VRLocomotion.Instance.SetPosition(position - vector3D5);
			}
		}
		else
		{
			if (target != null && target.CameraDistance != 1.0)
			{
				num = target.CameraDistance * 20.0;
			}
			if (!flag)
			{
				VirtualSpace.ScaleFactor = 1.56961230576048E-08;
			}
			else
			{
				VirtualSpace.ScaleFactor = Math.Min(1.0, 1.0 / (num / 2.0));
			}
			yield return (object)new WaitForEndOfFrame();
			Vector3D backward = Vector3D.Backward;
			backward.y -= 0.15000000596046448;
			backward *= ((ActiveSimulation.Settings.CameraTargetDistance != 0.0) ? ActiveSimulation.Settings.CameraTargetDistance : (1.0 / VirtualSpace.ScaleFactor));
			if ((Object)(object)DashManager.instance != (Object)null)
			{
				Camera6DOF.RotateRoot(DashManager.instance.dock.rotation.Inverse(), force: true);
				Vector3D vector3D6 = (Camera6DOF.rootRotation * ((Component)DashManager.instance.dock).transform.localPosition).ToVector3D() / VirtualSpace.ScaleFactor;
				VRLocomotion.Instance.SetPosition(VRSceneCenter - vector3D6 - backward);
			}
			else
			{
				backward.y += Math.Max(1.0, ((Component)Camera6DOF.Camera).transform.localPosition.y) * 1.0 / VirtualSpace.ScaleFactor;
				VRLocomotion.Instance.SetPosition(VRSceneCenter - backward);
			}
		}
		if (target != null)
		{
			MessageDispatcher.Send("AttachVRCameraToBody", (object)target);
		}
		yield return null;
	}

	private static void AttachCameraFromSettings(SimulationSettings settings)
	{
		//IL_0011: Unknown result type (might be due to invalid IL or missing references)
		//IL_0016: Unknown result type (might be due to invalid IL or missing references)
		//IL_001b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0021: Unknown result type (might be due to invalid IL or missing references)
		//IL_002c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		//IL_0036: Unknown result type (might be due to invalid IL or missing references)
		//IL_0037: Unknown result type (might be due to invalid IL or missing references)
		//IL_003c: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ab: Unknown result type (might be due to invalid IL or missing references)
		//IL_00b0: Unknown result type (might be due to invalid IL or missing references)
		//IL_00b5: Unknown result type (might be due to invalid IL or missing references)
		CameraState.OnEvent(CameraEvent.Attach);
		Quaternion val = Quaternion.Euler(settings.CameraAngle.ToVector3());
		CameraState.Rotation.Quat = val;
		CameraState.Orientation = CameraState.Target.EngineOrientation.Inverse() * val;
		CameraState.OnEvent(CameraEvent.TransitionDone);
		CameraState.AttachPoint = ActiveSimulation.Settings.CameraAttachPoint;
		CameraState.Position.Vector = ActiveSimulation.Settings.CameraPosition;
		CameraState.DistanceBeforeAttached = 2.0 * CameraState.Target.Radius;
		CameraState.RotationBeforeAttached = Quaternion.LookRotation((CameraState.Target.Position - CameraState.Position.Vector).ToVector3());
		if (Camera6DOF.Instance.CameraState is Attached attached)
		{
			attached.SetHeight(ActiveSimulation.Settings.CameraAttachHeight);
		}
	}

	public static void DesktopCameraSetup()
	{
		//IL_0067: Unknown result type (might be due to invalid IL or missing references)
		//IL_006c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0071: Unknown result type (might be due to invalid IL or missing references)
		//IL_0081: Unknown result type (might be due to invalid IL or missing references)
		//IL_0086: Unknown result type (might be due to invalid IL or missing references)
		//IL_01f6: Unknown result type (might be due to invalid IL or missing references)
		//IL_0146: Unknown result type (might be due to invalid IL or missing references)
		//IL_014b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0150: Unknown result type (might be due to invalid IL or missing references)
		//IL_0151: Unknown result type (might be due to invalid IL or missing references)
		//IL_0156: Unknown result type (might be due to invalid IL or missing references)
		//IL_013b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0265: Unknown result type (might be due to invalid IL or missing references)
		if (IsDemoMode || (ActiveSimulation.Settings.IgnoreCamera && ActiveSimulation.Settings.CameraPath == null))
		{
			return;
		}
		CameraBase.Move(0.0, 0.0, 0.0, 0.0);
		Camera6DOF.ResetRotation();
		Quaternion val = Quaternion.Euler(ActiveSimulation.Settings.CameraAngle.ToVector3());
		Camera6DOF.RotateRoot(Quaternion.Euler(ActiveSimulation.Settings.CameraRootRotation.ToVector3()), force: true);
		if (ActiveSimulation.Settings.CameraPath != null)
		{
			ActiveSimulation.Settings.CameraPath.Play(runIfPaused: false);
			CameraBase.SetPath(ActiveSimulation.Settings.CameraPath);
		}
		else if (ActiveSimulation.Settings.CameraTarget != null)
		{
			CameraBase.Target = ActiveSimulation.Settings.CameraTarget;
			CameraBase.MoveInstantlyToTarget();
			if (ActiveSimulation.Settings.IsCameraAttached)
			{
				AttachCameraFromSettings(ActiveSimulation.Settings);
			}
			else
			{
				if (ActiveSimulation.Settings.CameraAngle != Vector3D.Zero)
				{
					CameraState.Rotation.Quat = val;
				}
				CameraState.Orientation = CameraBase.Target.EngineOrientation.Inverse() * val;
				try
				{
					CameraState.OnEvent(CameraEvent.TransitionDone);
				}
				catch (Exception)
				{
				}
			}
			if (ActiveSimulation.Settings.CameraTargetDistance != 0.0)
			{
				CameraBase.Zoom.Value = ActiveSimulation.Settings.CameraTargetDistance;
			}
		}
		else if (ActiveSimulation.Settings.CameraPosition != Vector3D.Zero)
		{
			CameraBase.Position.Vector = ActiveSimulation.Settings.CameraPosition;
			if (ActiveSimulation.Settings.CameraAngle != Vector3D.Zero)
			{
				CameraBase.Rotation.Quat = val;
			}
		}
		else
		{
			CameraBase.Target = ActiveSimulation.GetLargestBody();
			CameraBase.MoveInstantlyToTarget();
			if (ActiveSimulation.Settings.CameraTargetDistance != 0.0)
			{
				CameraBase.Zoom.Value = ActiveSimulation.Settings.CameraTargetDistance;
			}
			if (ActiveSimulation.Settings.CameraAngle != Vector3D.Zero)
			{
				CameraBase.Rotation.Quat = val;
			}
		}
	}

	private static void CompleteLoading()
	{
		//IL_0484: Unknown result type (might be due to invalid IL or missing references)
		//IL_03b0: Unknown result type (might be due to invalid IL or missing references)
		//IL_03b5: Unknown result type (might be due to invalid IL or missing references)
		//IL_03bb: Expected Obj, but got Unknown
		//IL_040d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0412: Unknown result type (might be due to invalid IL or missing references)
		//IL_0418: Expected Obj, but got Unknown
		if (PendingSimulation == null)
		{
			return;
		}
		DateTime now = DateTime.Now;
		if (IsMerge)
		{
			ActiveSimulation.Merge(PendingSimulation);
			PendingSimulation.Dispose();
			IsMerge = false;
		}
		else
		{
			ActiveSimulation.Clear();
			ActiveSimulation = null;
			Reset();
			ActiveSimulation = PendingSimulation;
			if (UGCMain.CurrentUGCItem != null)
			{
				string currentSimName = UGCMain.GetCurrentSimName();
				if (string.IsNullOrEmpty(ActiveSimulation.Name))
				{
					ActiveSimulation.Name = currentSimName;
				}
				string currentSimDescription = UGCMain.GetCurrentSimDescription();
				if (!string.IsNullOrEmpty(currentSimDescription))
				{
					ActiveSimulation.Description = currentSimDescription;
				}
			}
		}
		USNumberFormatting.Initialize();
		EntityQuery.FlushRemoved();
		OnSimLoading?.Invoke(wasReload);
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)("Initializing Simulation '" + ActiveSimulation.Name + "' (" + Readable.Time(LoadStopwatch.Elapsed.TotalSeconds) + ")"));
		}
		bool wasDeserialized = ActiveSimulation.WasDeserialized;
		ActiveSimulation.Initialize();
		PendingSimulation = null;
		foreach (Entity entity in ActiveSimulation.Entities)
		{
			EntityRegister.Add(entity);
		}
		SystemPause = false;
		Paused = (Settings.Global.PauseOnLoad || ActiveSimulation.Settings.StartPaused) && !GuideManager.IsGuideActive && !Settings.Global.BenchmarkMode;
		Settings.Simulation = ActiveSimulation.Settings;
		if (ActiveSimulation.View == null)
		{
			if (Settings.View == null)
			{
				ActiveSimulation.View = ViewSettings.Instance;
			}
			else
			{
				ActiveSimulation.View = Settings.View;
			}
		}
		else if (Settings.View != null)
		{
			ActiveSimulation.View = Settings.View;
		}
		ActiveSimulation.View.SetActive(syncInterpolators: true);
		if (ChangeAmbienceOnLoad)
		{
			Settings.Ambience = ActiveSimulation.Ambience;
		}
		CameraBase.Target = null;
		OnBeforeCameraSetup?.Invoke();
		if (DeviceInfo.IsVR)
		{
			Instance.SetupVRCamera();
		}
		else
		{
			DesktopCameraSetup();
		}
		if (!ActiveSimulation.UserCreated && ActiveSimulation.Settings.RegenerateTrails)
		{
			Utilities.AddHistoricalTrails();
		}
		BodySearchUtils.Initialize();
		Watchdog.Stop(Watchdog.Strings.loadSim);
		LoadStopwatch.Stop();
		IsLoading = false;
		double totalSeconds = LoadStopwatch.Elapsed.TotalSeconds;
		if (BuildInfo.DEBUG_SIMULATIONSTART || Application.isEditor)
		{
			Debug.Log((object)("Simulation loaded: " + Language.Get(ActiveSimulation.Name) + " (" + Readable.Time(totalSeconds) + ")"));
		}
		Terminal.Event(TerminalEvent.SimLoaded);
		if (wasReload)
		{
			Terminal.Event(TerminalEvent.SimReloaded);
		}
		Budget.ResetCounters();
		EntityQuery.FlushAdded();
		SurfaceLockState.OnSimLoaded();
		if (OnSimLoaded != null)
		{
			OnSimLoaded(wasReload);
		}
		wasReload = false;
		OnLoadEnd?.Invoke(arg1: true, null);
		if (ActiveSimulation.BuildCommit > 0 && !UIUtils.HasAlreadyVersionPrompted(ActiveSimulation))
		{
			if (ActiveSimulation.BuildCommit <= 28657)
			{
				UIUtils.AddPrompted(ActiveSimulation);
				string text = Constants.UI.WARNING_OLD_SIM_LOAD;
				string buttonLabel = Constants.UI.WARNING_LEARNMORE;
				CosmosAction val = <>c.<>9__170_0;
				if (val == null)
				{
					CosmosAction val2 = () =>
					{
						Application.OpenURL("https://universesandbox.com/support/savefileversions");
					};
					<>c.<>9__170_0 = val2;
					val = val2;
				}
				Snackbar.ShowPrompt("", text, buttonLabel, val);
			}
			else if (ActiveSimulation.BuildCommit > BuildInfo.VERSION_COMMIT_COUNT)
			{
				UIUtils.AddPrompted(ActiveSimulation);
				string text2 = Constants.UI.WARNING_NEWER_SIM_LOAD;
				string buttonLabel2 = Constants.UI.WARNING_LEARNMORE;
				CosmosAction val3 = <>c.<>9__170_1;
				if (val3 == null)
				{
					CosmosAction val4 = () =>
					{
						Application.OpenURL("https://universesandbox.com/support/savefileversions");
					};
					<>c.<>9__170_1 = val4;
					val3 = val4;
				}
				Snackbar.ShowPrompt("", text2, buttonLabel2, val3);
			}
		}
		ActiveSimulation.Changed = false;
		if (wasDeserialized && ActiveSimulation.Settings.Constellations)
		{
			MessageDispatcher.Send("BATCH_CONSTELLATION_ADD", (object)new List<Body>(ActiveSimulation.Bodies));
			MessageDispatcher.Send("CONSTELLATION_SET_READY", (object)ActiveSimulation);
		}
		DebugUtil.LogTime("Sandbox.CompleteLoading", now);
		if (OnLoadEndAsync != null)
		{
			UniTaskExtensions.Forget(OnLoadEndAsync.InvokeAllAsync(wasReload));
		}
	}

	private void FixedUpdate()
	{
		Watchdog.Stop(Watchdog.Strings.entireFrame);
		Watchdog.Start(Watchdog.Strings.entireFrame, updateOnlyOnStop: false, resetOnStop: true);
		Settings.Global.Apply();
		Settings.Simulation.Apply();
		VROptions.Instance?.Apply();
		CompleteLoading();
	}

	[AsyncStateMachine(typeof(<OnFirstUpdate>d__172))]
	private UniTask OnFirstUpdate()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		<OnFirstUpdate>d__172 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.<>4__this = this;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<OnFirstUpdate>d__172>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<FinishFrame>d__173))]
	private UniTask FinishFrame()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		<FinishFrame>d__173 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.<>4__this = this;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<FinishFrame>d__173>(ref obj);
		return obj.<>t__builder.Task;
	}

	public static void ShowIntro()
	{
		OnIntro?.Invoke();
	}

	public static void StopScript()
	{
		CameraBase.SetPath(null);
		if (CommandSequence.OnFinishAction != null)
		{
			CommandSequence.OnFinishAction.Execute();
		}
		Cursor.visible = true;
		CommandSequence = null;
	}

	public static void StartScript(string text)
	{
		StartScript(text, allowSkip: true);
	}

	public static void StartScript(string text, bool allowSkip)
	{
		StartScript(text, allowSkip, null, null);
	}

	public static void StartScript(string text, bool allowSkip, CommandAction onStart, CommandAction onFinished)
	{
		OnStartScript?.Invoke();
		CommandSequence = new CommandSequence(text);
		CommandSequence.AllowSkip = allowSkip;
		Cursor.visible = false;
		if (onStart != null)
		{
			onStart.Sequencer = CommandSequence;
			CommandSequence.Actions.Insert(0, onStart);
		}
		if (onFinished != null)
		{
			onFinished.Sequencer = CommandSequence;
			CommandSequence.OnFinishAction = onFinished;
		}
	}

	public static void Reset()
	{
		ToolManager.Instance.ClearAllTools();
		Mode = SandboxState.Simulate;
		Settings.Simulation.RayContext = GuiSetting.Selected;
		Protocol.Clear();
		ShurikenParticleRenderer[] array = Object.FindObjectsByType<ShurikenParticleRenderer>();
		for (int i = 0; i < array.Length; i++)
		{
			((Component)array[i]).SendMessage("Clear", (SendMessageOptions)1);
		}
		SimpleParticleRenderer[] array2 = Object.FindObjectsByType<SimpleParticleRenderer>();
		for (int i = 0; i < array2.Length; i++)
		{
			((Component)array2[i]).SendMessage("Clear", (SendMessageOptions)1);
		}
		NovaRenderer[] array3 = Object.FindObjectsByType<NovaRenderer>();
		for (int i = 0; i < array3.Length; i++)
		{
			Object.Destroy((Object)(object)((Component)array3[i]).gameObject);
		}
		TetherRenderer.Clear();
		FPSCounter.Reset();
		Selector.Clear();
		SelectionStack.Clear();
		BulletinBoard.Reset();
		DebugTools.ClearLines();
		ChartUtil.Reset();
		ObjectPool.Reset();
	}

	public static void Destroy()
	{
		if (ActiveSimulation != null)
		{
			ActiveSimulation.Clear();
			ActiveSimulation.Dispose();
			ActiveSimulation = null;
			CameraBase.SetPath(null);
			Resources.UnloadUnusedAssets();
		}
	}

	private void OnDestroy()
	{
		Destroy();
	}

	public void CancelExit()
	{
		Audio.Mute = false;
		AboutToQuit = false;
		QuitStarted = false;
	}

	private static bool WantsToQuit()
	{
		if (!AboutToQuit && !FinalQuit)
		{
			Instance?.TryToQuit();
		}
		if (!AboutToQuit)
		{
			return FinalQuit;
		}
		return true;
	}

	public void TryToQuit()
	{
		//IL_0056: Unknown result type (might be due to invalid IL or missing references)
		if ((QuitStarted && !FinalQuit) || AboutToQuit)
		{
			return;
		}
		if (GIFCapture.Running)
		{
			GIFCapture.EndCapture();
		}
		if (IsDemoBuild || DeviceInfo.IsVR)
		{
			FinalQuit = true;
			if (!Application.isEditor)
			{
				Process.GetCurrentProcess().Kill();
			}
			return;
		}
		if (Application.isEditor)
		{
			UniTaskExtensions.Forget(Settings.Global.SaveTask());
		}
		if (!FinalQuit)
		{
			QuitStarted = true;
			Audio.Mute = true;
			if (SilentAndDontSaveQuit)
			{
				DragAndDropHook.UninstallHook();
				Quit();
				AboutToQuit = false;
				Audio.Mute = false;
			}
			else if (!QuitShortcut.WasKeyboardQuit() && OnNeedsSurvey != null && OnNeedsSurvey())
			{
				OnSurvey?.Invoke();
			}
			else
			{
				OnExit?.Invoke();
			}
		}
	}

	public void Quit()
	{
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		FinalQuit = true;
		UniTaskExtensions.Forget(QuitAsync());
	}

	[AsyncStateMachine(typeof(<QuitAsync>d__186))]
	private UniTask QuitAsync()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0029: Unknown result type (might be due to invalid IL or missing references)
		<QuitAsync>d__186 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<QuitAsync>d__186>(ref obj);
		return obj.<>t__builder.Task;
	}

	[AsyncStateMachine(typeof(<FromCurrentFileAsync>d__187))]
	public static UniTask<Simulation> FromCurrentFileAsync()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0029: Unknown result type (might be due to invalid IL or missing references)
		<FromCurrentFileAsync>d__187 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder<Simulation>.Create();
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<FromCurrentFileAsync>d__187>(ref obj);
		return obj.<>t__builder.Task;
	}

	[Obsolete]
	public static Simulation FromCurrentFile()
	{
		if (CurrentFile.EndsWith("ubox"))
		{
			if (!File.Exists(CurrentFile))
			{
				return null;
			}
			return FileTools.LoadSimulation(CurrentFile);
		}
		if (!Directory.Exists(CurrentFile))
		{
			return null;
		}
		return FileTools.LoadSimulationFlat(CurrentFile);
	}

	[AsyncStateMachine(typeof(<LoadSimAsync>d__189))]
	public static UniTask LoadSimAsync(string file, bool wasReload = false)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0039: Unknown result type (might be due to invalid IL or missing references)
		<LoadSimAsync>d__189 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.file = file;
		obj.wasReload = wasReload;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<LoadSimAsync>d__189>(ref obj);
		return obj.<>t__builder.Task;
	}

	[Obsolete]
	public static void LoadSim(string file, bool wasReload = false)
	{
		Sandbox.wasReload = wasReload;
		CurrentFile = file;
		LoadSim((Func<Simulation>)FromCurrentFile);
	}

	[Obsolete]
	public static void LoadSimImmediate(Func<Simulation> loadHandler)
	{
		IsLoading = true;
		wasReload = false;
		lastLoad = loadHandler;
		Watchdog.Start(Watchdog.Strings.loadSim, updateOnlyOnStop: true);
		LoadStopwatch.Reset();
		LoadStopwatch.Start();
		OnLoadBegin?.Invoke();
		try
		{
			Body.ResetId();
			Group.ResetId();
			Terminal.Event("Simulation loading...");
			PendingSimulation = lastLoad();
			CompleteLoading();
		}
		catch (Exception ex)
		{
			Watchdog.Stop(Watchdog.Strings.loadSim);
			LoadingSummary.Clear();
			Debug.LogException(ex);
			OnLoadEnd?.Invoke(arg1: false, ex);
		}
	}

	[AsyncStateMachine(typeof(<ReloadSimulationAsync>d__192))]
	public static UniTask ReloadSimulationAsync()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0029: Unknown result type (might be due to invalid IL or missing references)
		<ReloadSimulationAsync>d__192 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<ReloadSimulationAsync>d__192>(ref obj);
		return obj.<>t__builder.Task;
	}

	public static void ReloadSimulation()
	{
		//IL_0016: Unknown result type (might be due to invalid IL or missing references)
		if (FileTools.IsSavingAsync)
		{
			UIManager.Instance.ProgressWindow.Open();
		}
		UniTaskExtensions.Forget(ReloadSimulationTask());
	}

	public static void ForceDRMWarning()
	{
		switch (BuildInfo.BUILDTYPE)
		{
		case BuildType.Steam:
			OnSteamWarning?.Invoke();
			break;
		case BuildType.Epic:
			OnEpicWarning?.Invoke();
			break;
		}
	}

	private static IEnumerator ReloadSimulationCoroutine()
	{
		while (FileTools.IsSavingAsync)
		{
			yield return WaitForEndOfFrame;
		}
		reloadHandler?.Invoke();
	}

	public static void LoadSim(Func<Simulation> loadHandler)
	{
		LoadSim(loadHandler, wasReload, SkipSimLoadConfirmation);
	}

	public static void LoadSimAutomated(Func<Simulation> loadHandler)
	{
		LoadSim(loadHandler, reload: false, fromAutomation: true);
	}

	private static void LoadSim(Func<Simulation> loadHandler, bool reload, bool fromAutomation)
	{
		//IL_004d: Unknown result type (might be due to invalid IL or missing references)
		if (!IsLoading)
		{
			wasReload = reload;
			if (!wasReload)
			{
				Preset.CurrentPreset = null;
			}
			OnSimLoadBegin?.Invoke();
			if (wasReload)
			{
				OnSimReloadStarted?.Invoke();
			}
			Terminal.Event(TerminalEvent.SimLoadBegin);
			if (!fromAutomation)
			{
				UniTaskExtensions.Forget(LoadSimCoroutine(loadHandler));
			}
			else
			{
				LoadSimInternal(loadHandler);
			}
		}
	}

	[AsyncStateMachine(typeof(<LoadSimCoroutine>d__199))]
	private static UniTask LoadSimCoroutine(Func<Simulation> loadHandler)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		<LoadSimCoroutine>d__199 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.loadHandler = loadHandler;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<LoadSimCoroutine>d__199>(ref obj);
		return obj.<>t__builder.Task;
	}

	private static void LoadSimInternal(Func<Simulation> loadHandler)
	{
		SkipSimLoadConfirmation = false;
		IsLoading = true;
		lastLoad = loadHandler;
		Watchdog.Start(Watchdog.Strings.loadSim, updateOnlyOnStop: true);
		LoadStopwatch.Reset();
		LoadStopwatch.Start();
		OnLoadBegin?.Invoke();
		if (LoadSimHandler != null)
		{
			LoadSimHandler(InvokeLoading);
		}
		else
		{
			InvokeLoading();
		}
	}

	[Obsolete]
	public static void MergeSim(Func<Simulation> loadHandler)
	{
		IsMerge = true;
		LoadSim(loadHandler);
	}

	private static void InvokeLoading()
	{
		Terminal.Event("Simulation loading...");
		if (AsyncLoading)
		{
			if (worker == null)
			{
				worker = new BackgroundWorker();
				worker.WorkerReportsProgress = true;
				worker.WorkerSupportsCancellation = true;
				Progress = new WorkerProgress(worker);
			}
			if (!worker.IsBusy)
			{
				worker.DoWork += worker_LoadSim;
				worker.RunWorkerCompleted += worker_LoadSimCompleted;
				worker.RunWorkerAsync();
			}
			return;
		}
		try
		{
			Body.ResetId();
			Group.ResetId();
			Simulation simulation = lastLoad();
			if (simulation == null)
			{
				OnLoadEnd?.Invoke(arg1: false, new Exception("The simulation you are trying to load does not exist anymore."));
			}
			else
			{
				PendingSimulation = simulation;
			}
		}
		catch (Exception ex)
		{
			LoadingSummary.Clear();
			Debug.LogException(ex);
			OnLoadEnd?.Invoke(arg1: false, ex);
		}
	}

	private static void worker_LoadSimCompleted(object sender, RunWorkerCompletedEventArgs e)
	{
		worker.DoWork -= worker_LoadSim;
		worker.RunWorkerCompleted -= worker_LoadSimCompleted;
		if (e.Cancelled)
		{
			IsLoading = false;
			Watchdog.Stop(Watchdog.Strings.loadSim);
			return;
		}
		if (e.Result == null)
		{
			IsLoading = false;
			OnLoadEnd?.Invoke(arg1: false, new Exception("The simulation you are trying to load does not exist anymore."));
		}
		PendingSimulation = e.Result as Simulation;
	}

	private static void worker_LoadSim(object sender, DoWorkEventArgs e)
	{
		try
		{
			Thread.CurrentThread.CurrentCulture = CultureInfo.InvariantCulture;
			Thread.CurrentThread.Name = "#0 USX Simloader";
			Body.ResetId();
			Group.ResetId();
			e.Result = lastLoad();
		}
		catch (Exception ex)
		{
			LoadingSummary.Clear();
			Thread.Sleep(1000);
			e.Cancel = true;
			Debug.LogException(ex);
			OnLoadEnd?.Invoke(arg1: false, ex);
		}
	}
}
