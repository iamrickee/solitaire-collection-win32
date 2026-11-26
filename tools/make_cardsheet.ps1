# Builds res/cards.bmp from the Kenney "Playing Cards Pack" (CC0).
#
# Usage:  pwsh tools/make_cardsheet.ps1 -PackDir <path to extracted pack>
#
# Output layout (each cell 42x60, magenta = transparent):
#   rows    : Clubs, Diamonds, Hearts, Spades
#   columns : A, 2..10, J, Q, K, then column 13 = special (row 0 back, row 1 empty slot)
param(
    [Parameter(Mandatory)] [string] $PackDir,
    [string] $OutFile = (Join-Path $PSScriptRoot '..\res\cards.bmp')
)

Add-Type -AssemblyName System.Drawing

$src    = Join-Path $PackDir 'PNG\Cards (large)'
$cropX  = 11; $cropY = 2; $w = 42; $h = 60
$suits  = 'clubs', 'diamonds', 'hearts', 'spades'
$ranks  = 'A', '02', '03', '04', '05', '06', '07', '08', '09', '10', 'J', 'Q', 'K'
$key    = [System.Drawing.Color]::FromArgb(255, 255, 0, 255)

$sheet = New-Object System.Drawing.Bitmap ($w * 14), ($h * 4), ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
$g = [System.Drawing.Graphics]::FromImage($sheet)
$g.Clear($key)
$g.Dispose()

function Copy-Cell([string] $name, [int] $col, [int] $row) {
    $img = [System.Drawing.Bitmap]::FromFile((Join-Path $src "$name.png"))
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $c = $img.GetPixel($cropX + $x, $cropY + $y)
            if ($c.A -ge 128) {
                $sheet.SetPixel($col * $w + $x, $row * $h + $y, [System.Drawing.Color]::FromArgb(255, $c.R, $c.G, $c.B))
            }
        }
    }
    $img.Dispose()
}

for ($r = 0; $r -lt 4; $r++) {
    for ($c = 0; $c -lt 13; $c++) {
        Copy-Cell "card_$($suits[$r])_$($ranks[$c])" $c $r
    }
}
Copy-Cell 'card_back'  13 0
Copy-Cell 'card_empty' 13 1

$OutFile = [System.IO.Path]::GetFullPath($OutFile)
New-Item -ItemType Directory -Force (Split-Path $OutFile) | Out-Null
$sheet.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
$sheet.Dispose()
Write-Host "Wrote $OutFile"
