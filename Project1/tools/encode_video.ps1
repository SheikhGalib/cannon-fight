# encode_video.ps1
# ===============
#
# Combines `capture/frame_NNNNN.bmp` into a short presentation video
# using ffmpeg.  Run from inside `Project1/` so the relative paths
# line up:
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools/encode_video.ps1
#
# Requires ffmpeg on PATH (e.g. from winget or choco).

$ErrorActionPreference = "Stop"

$FrameDir = "capture"
$FramePattern = "capture/frame_%05d.bmp"
$Output    = "presentation.mp4"
$Fps       = 30

if (-not (Test-Path $FrameDir)) {
    Write-Error "No '$FrameDir/' directory found.  Run build_mingw\capture.exe first."
}
$frameCount = (Get-ChildItem -Path (Join-Path $FrameDir "frame_*.bmp")).Count
if ($frameCount -eq 0) {
    Write-Error "No frame_*.bmp files in '$FrameDir/'.  Run build_mingw\capture.exe first."
}

Write-Host "Encoding $frameCount frames at $Fps fps into $Output ..."

# ffmpeg -y:    overwrite output
# -framerate:  input frame rate
# -i:          input pattern
# -c:v libx264: h264 video codec (good quality, small files)
# -pix_fmt yuv420p: required for most players
# -vf scale:   ensure dimensions are even (libx264 requirement)
ffmpeg -y -framerate $Fps -i $FramePattern -c:v libx264 -pix_fmt yuv420p -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" $Output

if ($LASTEXITCODE -eq 0) {
    Write-Host "Done.  Video written to $Output"
    # Also copy to images/ for the project layout.
    $imagesDir = "..\images"
    if (Test-Path $imagesDir) {
        $target = Join-Path $imagesDir "phase-9-presentation.mp4"
        Copy-Item $Output $target -Force
        Write-Host "Also copied to $target"
    }
} else {
    Write-Error "ffmpeg failed (exit code $LASTEXITCODE)"
}