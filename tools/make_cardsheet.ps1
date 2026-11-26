# Builds res/cards.bmp from the Kenney "Playing Cards Pack" (CC0).
#
# Usage:  pwsh tools/make_cardsheet.ps1 -PackDir <path to extracted pack>
#
# Output layout (each cell 42x60, magenta = transparent):
#   rows    : Clubs, Diamonds, Hearts, Spades
#   columns : A, 2..10, J, Q, K, then column 13 = special:
#             row 0 back, row 1 empty slot, row 2 red joker, row 3 black joker
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
Copy-Cell 'card_joker_red'   13 2
Copy-Cell 'card_joker_black' 13 3

# Turn the jokers' corner "J" into "Jo" (a lowercase o beside the J, in the
# same 2-pixel stroke), in both corners.
$o = @(
    '.###.'
    '##.##'
    '##.##'
    '##.##'
    '.###.'
)
function Add-JokerO([int] $row, [System.Drawing.Color] $ink) {
    $ox = 13 * $w; $oy = $row * $h
    foreach ($rot in $false, $true) {
        for ($gy = 0; $gy -lt $o.Count; $gy++) {
            for ($gx = 0; $gx -lt $o[$gy].Length; $gx++) {
                if ($o[$gy][$gx] -ne '#') { continue }
                $x = 11 + $gx; $y = 8 + $gy
                $px = if ($rot) { $w - 1 - $x } else { $x }
                $py = if ($rot) { $h - 1 - $y } else { $y }
                $sheet.SetPixel($ox + $px, $oy + $py, $ink)
            }
        }
    }
}
Add-JokerO 2 ([System.Drawing.Color]::FromArgb(255, 0xF5, 0x2C, 0x4E))
Add-JokerO 3 ([System.Drawing.Color]::FromArgb(255, 0x4D, 0x57, 0x66))

# The pack's jokers show a "J" in the corners, which reads as a Jack when the
# card is fanned. Invert them so they stand out: the card face turns gold and
# the ink turns white. The outer outline is left alone.
function Set-JokerInverted([int] $row, [System.Drawing.Color] $ink) {
    $ox = 13 * $w; $oy = $row * $h
    $white = [System.Drawing.Color]::FromArgb(255, 255, 255, 255)
    $shade = [System.Drawing.Color]::FromArgb(255, [int]($ink.R * 0.7), [int]($ink.G * 0.7), [int]($ink.B * 0.7))
    $map = @{
        'FFFFFFFF' = $ink     # paper  -> joker colour
        'FFC7D7EC' = $shade   # light shading -> darker joker colour
        'FF4D5766' = $white   # dark ink -> white
        'FFF52C4E' = $white   # red ink -> white
    }
    for ($y = 1; $y -lt $h - 1; $y++) {
        for ($x = 1; $x -lt $w - 1; $x++) {
            $key = $sheet.GetPixel($ox + $x, $oy + $y).ToArgb().ToString('X8')
            if ($map.ContainsKey($key)) { $sheet.SetPixel($ox + $x, $oy + $y, $map[$key]) }
        }
    }
}
$gold = [System.Drawing.Color]::FromArgb(255, 0xE8, 0xB0, 0x20)
Set-JokerInverted 2 $gold
Set-JokerInverted 3 $gold

$OutFile = [System.IO.Path]::GetFullPath($OutFile)
New-Item -ItemType Directory -Force (Split-Path $OutFile) | Out-Null
$sheet.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
$sheet.Dispose()
Write-Host "Wrote $OutFile"
