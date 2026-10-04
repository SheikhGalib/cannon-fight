# Converts every .bmp that RenderDocShots.exe just wrote into a .png and
# removes the .bmp. Run from inside Project1/ (the `shots` Makefile target does).
Add-Type -AssemblyName System.Drawing

$dir = Join-Path $PSScriptRoot "..\..\docs\images\walkthrough"
$dir = (Resolve-Path $dir).Path

$count = 0
Get-ChildItem -Path $dir -Filter *.bmp | ForEach-Object {
    $png = [System.IO.Path]::ChangeExtension($_.FullName, ".png")
    $img = [System.Drawing.Image]::FromFile($_.FullName)
    $img.Save($png, [System.Drawing.Imaging.ImageFormat]::Png)
    $img.Dispose()
    Remove-Item $_.FullName
    $count++
}
Write-Output "converted $count bmp -> png in $dir"
