# Play-test helper: drives the running game window with real mouse/keyboard input and captures screenshots.
# Usage: play.ps1 shot <file.png> | click <x> <y> | key <VK> [holdMs] | move <dx> <dy> | wait <ms>
# x/y are client-area pixels (the game runs -windowed -ResX=1600 -ResY=900).
param([string]$Action, [string]$A1, [string]$A2, [string]$A3)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public struct PT { public int X, Y; }
public struct RC { public int L, T, R, B; }
public static class W {
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RC r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref PT p);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern bool GetCursorPos(out PT p);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, int dx, int dy, uint d, IntPtr e);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte sc, uint f, IntPtr e);
}
"@
[W]::SetProcessDPIAware() | Out-Null
$p = Get-Process | Where-Object { $_.ProcessName -eq "UnrealEditor" -and $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { Write-Output "no game window"; exit 1 }
$h = $p.MainWindowHandle
[W]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 120
$o = New-Object PT; [W]::ClientToScreen($h, [ref]$o) | Out-Null
$r = New-Object RC; [W]::GetClientRect($h, [ref]$r) | Out-Null
switch ($Action) {
  "shot" {
    $bmp = New-Object System.Drawing.Bitmap $r.R, $r.B
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($o.X, $o.Y, 0, 0, $bmp.Size)
    $bmp.Save($A1, [System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()
    Write-Output "shot $A1 ($($r.R)x$($r.B))"
  }
  "click" {
    [W]::SetCursorPos($o.X + [int]$A1, $o.Y + [int]$A2) | Out-Null; Start-Sleep -Milliseconds 80
    [W]::mouse_event(0x0002, 0, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 60; [W]::mouse_event(0x0004, 0, 0, 0, [IntPtr]::Zero)
    Write-Output "click $A1,$A2"
  }
  "down" { [W]::SetCursorPos($o.X + [int]$A1, $o.Y + [int]$A2) | Out-Null; Start-Sleep -Milliseconds 80; [W]::mouse_event(0x0002, 0, 0, 0, [IntPtr]::Zero); Write-Output "down $A1,$A2" }
  "up" { [W]::mouse_event(0x0004, 0, 0, 0, [IntPtr]::Zero); Write-Output "up" }
  "glide" {
    # move the cursor in small steps to x,y (so the game sees a real drag)
    $c = New-Object PT; [W]::GetCursorPos([ref]$c) | Out-Null
    for ($i = 1; $i -le 20; $i++) { [W]::SetCursorPos([int]($c.X + ($o.X + [int]$A1 - $c.X) * $i / 20), [int]($c.Y + ($o.Y + [int]$A2 - $c.Y) * $i / 20)) | Out-Null; Start-Sleep -Milliseconds 25 }
    Write-Output "glide $A1,$A2"
  }
  "move" { [W]::mouse_event(0x0001, [int]$A1, [int]$A2, 0, [IntPtr]::Zero); Write-Output "move $A1,$A2" }
  "key" {
    $vk = [byte][int]$A1; $hold = if ($A2) { [int]$A2 } else { 60 }
    [W]::keybd_event($vk, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds $hold; [W]::keybd_event($vk, 0, 2, [IntPtr]::Zero)
    Write-Output "key $A1 ${hold}ms"
  }
  "wait" { Start-Sleep -Milliseconds ([int]$A1) }
}
