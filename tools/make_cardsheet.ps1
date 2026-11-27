# Builds res/cards.bmp from the "PC-style playing cards" sheet by devurandom
# (CC0, opengameart.org/content/pc-style-playing-cards), saved as
# res/95ishdeck.gif: 14 x 4 cells of 71x96 cards with a 1px green gutter,
# rows Spades, Clubs, Diamonds, Hearts; column 13 holds blanks and a back.
#
# Usage:  pwsh tools/make_cardsheet.ps1 [-Source <gif>] [-OutFile <bmp>]
#
# Output layout (each cell 71x96, magenta = transparent):
#   rows    : Clubs, Diamonds, Hearts, Spades
#   columns : A, 2..10, J, Q, K, then column 13 = special:
#             row 0 back, row 1 blank card, row 2 red joker, row 3 black joker
param(
    [string] $Source  = (Join-Path $PSScriptRoot '..\res\95ishdeck.gif'),
    [string] $OutFile = (Join-Path $PSScriptRoot '..\res\cards.bmp')
)

Add-Type -AssemblyName System.Drawing

$w = 71; $h = 96; $pitch = 72
$key    = [System.Drawing.Color]::FromArgb(255, 255, 0, 255)
$green  = [System.Drawing.Color]::FromArgb(255, 0, 128, 0).ToArgb()   # gutter + rounded corners
$black  = [System.Drawing.Color]::FromArgb(255, 0, 0, 0)
$red    = [System.Drawing.Color]::FromArgb(255, 255, 0, 0)
$yellow = [System.Drawing.Color]::FromArgb(255, 255, 255, 0)

$src = New-Object System.Drawing.Bitmap ([System.Drawing.Image]::FromFile([System.IO.Path]::GetFullPath($Source)))
$sheet = New-Object System.Drawing.Bitmap ($w * 14), ($h * 4), ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)

function Copy-Cell([int] $srcCol, [int] $srcRow, [int] $col, [int] $row) {
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $c = $src.GetPixel($srcCol * $pitch + $x, $srcRow * 96 + $y)
            if ($c.ToArgb() -eq $green) { $c = $key }
            $sheet.SetPixel($col * $w + $x, $row * $h + $y, $c)
        }
    }
}

# Source rows are Spades, Clubs, Diamonds, Hearts; ours follow the Suit enum.
$srcRowOf = @(1, 2, 3, 0)
for ($r = 0; $r -lt 4; $r++) {
    for ($c = 0; $c -lt 13; $c++) { Copy-Cell $c $srcRowOf[$r] $c $r }
}
Copy-Cell 13 3 13 0   # back
Copy-Cell 13 0 13 1   # blank card
Copy-Cell 13 0 13 2   # blank cards for the jokers, drawn on below
Copy-Cell 13 1 13 3

# ---- Jokers (our own design) ----------------------------------------------
# Corner index "Jo" over a star, in the deck's 2-pixel style, plus a jester's
# cap and the word JOKER in the middle. Red joker: red ink; black joker: black.

function Put([int] $row, [string[]] $glyph, [int] $ox, [int] $oy, [hashtable] $colors, [switch] $Mirror) {
    for ($gy = 0; $gy -lt $glyph.Count; $gy++) {
        for ($gx = 0; $gx -lt $glyph[$gy].Length; $gx++) {
            $ch = [string]$glyph[$gy][$gx]
            if (-not $colors.ContainsKey($ch)) { continue }
            $x = $ox + $gx; $y = $oy + $gy
            $sheet.SetPixel(13 * $w + $x, $row * $h + $y, $colors[$ch])
            if ($Mirror) { $sheet.SetPixel(13 * $w + ($w - 1 - $x), $row * $h + ($h - 1 - $y), $colors[$ch]) }
        }
    }
}

$jo = @(       # "J" as on the deck's Jacks, plus a matching lowercase "o"
    '.....###.......'
    '.....###.......'
    '......##.......'
    '......##.......'
    '......##.######'
    '......##.##..##'
    '......##.##..##'
    '......##.##..##'
    '.##...##.##..##'
    '.#######.##..##'
    '..#####..##..##'
    '.........######'
)
$star = @(
    '....#....'
    '....#....'
    '...###...'
    '#########'
    '.#######.'
    '..#####..'
    '..##.##..'
    '.##...##.'
    '##.....##'
)
$bell = @(
    '..###..'
    '.#yyy#.'
    '#yyyyy#'
    '#yyyyy#'
    '#yyyyy#'
    '.#yyy#.'
    '..###..'
)
$font = @{
    'J' = @('..###', '...#.', '...#.', '...#.', '#..#.', '#..#.', '.##..')
    'O' = @('.###.', '#...#', '#...#', '#...#', '#...#', '#...#', '.###.')
    'K' = @('#...#', '#..#.', '#.#..', '##...', '#.#..', '#..#.', '#...#')
    'E' = @('#####', '#....', '#....', '####.', '#....', '#....', '#####')
    'R' = @('####.', '#...#', '#...#', '####.', '#.#..', '#..#.', '#...#')
}

foreach ($row in 2, 3) {
    $ink = if ($row -eq 2) { $red } else { $black }
    Put $row $jo 3 5 @{ '#' = $ink } -Mirror
    Put $row $star 3 20 @{ '#' = $ink } -Mirror

    # Jester's cap: three prongs (red, yellow, red) on a band, bells on the tips.
    $g = [System.Drawing.Graphics]::FromImage($sheet)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $g.TranslateTransform(13 * $w, $row * $h)
    $pen = New-Object System.Drawing.Pen $black, 1
    $prong = {
        param($pts, $fill)
        $p = [System.Drawing.Point[]]($pts | ForEach-Object { New-Object System.Drawing.Point $_[0], $_[1] })
        $b = New-Object System.Drawing.SolidBrush $fill
        $g.FillPolygon($b, $p); $g.DrawPolygon($pen, $p); $b.Dispose()
    }
    & $prong @(@(23, 58), @(16, 34), @(35, 58)) $red
    & $prong @(@(35, 58), @(54, 34), @(47, 58)) $red
    & $prong @(@(28, 58), @(35, 25), @(42, 58)) $yellow
    $band = New-Object System.Drawing.SolidBrush $yellow
    $g.FillRectangle($band, 21, 58, 28, 5); $g.DrawRectangle($pen, 21, 58, 28, 5); $band.Dispose()
    $pen.Dispose(); $g.Dispose()
    Put $row $bell 13 28 @{ '#' = $black; 'y' = $yellow }
    Put $row $bell 32 19 @{ '#' = $black; 'y' = $yellow }
    Put $row $bell 51 28 @{ '#' = $black; 'y' = $yellow }

    $x = 21
    foreach ($ch in 'J', 'O', 'K', 'E', 'R') { Put $row $font[$ch] $x 70 @{ '#' = $ink }; $x += 6 }
}

$src.Dispose()
$OutFile = [System.IO.Path]::GetFullPath($OutFile)
New-Item -ItemType Directory -Force (Split-Path $OutFile) | Out-Null
$sheet.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
$sheet.Dispose()
Write-Host "Wrote $OutFile"
