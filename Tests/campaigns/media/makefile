CD_NAME = media
include ../../campaign.mk

# The TGA, WAV and CubeTile test files are not committed: MEDIA_DATA_SH writes them to cd/data/ before the disc
# image is made. Its pixel, sample and byte formulas must match the expected values in Tests/src/testsTGA.hpp,
# testsWave.hpp and testsCubeTile.hpp.
create_iso : media_data

media_data :
	sh -c "$$MEDIA_DATA_SH" sh cd/data

.PHONY : media_data

# POSIX sh only (no Python, awk or od), so it runs with the msys2 tools of the Windows build: each file is built as a
# string of \ooo escapes and written by one printf.
define MEDIA_DATA_SH
set -e
out=$1
mkdir -p "$out"
s=
W=8
H=4

# b: appends each argument as one byte (its low 8 bits)
b() { for v; do v=$((v & 255)); s="$s\\$((v >> 6))$((v >> 3 & 7))$((v & 7))"; done; }
le16() { b $(($1)) $(($1 >> 8)); }
le32() { b $(($1)) $(($1 >> 8)) $(($1 >> 16)) $(($1 >> 24)); }
be32() { b $(($1 >> 24)) $(($1 >> 16)) $(($1 >> 8)) $(($1)); }
emit() { printf "$s" > "$out/$1"; s=; }

# key: pixel key k of (x, y); plain: x * 3 + y * 5, rle: a run of 4 equal pixels, then 4 different ones, on every row
key() { if [ "$kname" = plain ]; then k=$((x * 3 + y * 5)); else k=$((x * (x >= 4) + y * 9)); fi; }

# pixel: bytes of key k as an escape string in px; a palette index, or the color
# (k * 7, k * 13, 255 - k * 5) as 16-bit ARGB1555, 24-bit BGR or 32-bit BGRA
pixel() {
    keep=$s; s=
    if [ "$plen" -gt 0 ]; then
        b $((k % plen))
    else
        r=$((k * 7 & 255)); g=$((k * 13 & 255)); bl=$((255 - k * 5 & 255))
        case $bpp in
            16) le16 $((0x8000 | (r >> 3) << 10 | (g >> 3) << 5 | (bl >> 3))) ;;
            24) b $bl $g $r ;;
            32) b $bl $g $r 255 ;;
        esac
    fi
    px=$s; s=$keep
}

pget() { eval "pv=\$p$1"; }

# rle_row: RLE packets for p0..p7: runs of 2 or more become RLE packets, the rest raw packets
rle_row() {
    i=0
    while [ $i -lt $W ]; do
        pget $i; first=$pv; run=1
        while [ $((i + run)) -lt $W ] && [ $run -lt 128 ]; do
            pget $((i + run)); [ "$pv" = "$first" ] || break
            run=$((run + 1))
        done
        if [ $run -gt 1 ]; then
            b $((0x80 | (run - 1))); s="$s$first"; i=$((i + run))
        else
            j=$i
            while [ $j -lt $W ] && [ $((j - i)) -lt 128 ]; do
                if [ $((j + 1)) -lt $W ]; then
                    pget $j; cur=$pv; pget $((j + 1)); [ "$pv" != "$cur" ] || break
                fi
                j=$((j + 1))
            done
            b $((j - i - 1))
            while [ $i -lt $j ]; do pget $i; s="$s$pv"; i=$((i + 1)); done
        fi
    done
}

# tga name image_type bpp origin(BL, TL, TR, BR) key(plain, rle) palette_length
tga() {
    name=$1; type=$2; bpp=$3; origin=$4; kname=$5; plen=$6
    right=0; top=0
    case $origin in *R*) right=1 ;; esac
    case $origin in *T*) top=1 ;; esac
    b 0 $((plen > 0)) $type; le16 0; le16 $plen; b $((24 * (plen > 0))); le16 0; le16 0; le16 $W; le16 $H
    b $bpp $((right * 0x10 | top * 0x20 | (bpp == 32) * 8))
    j=0
    while [ $j -lt $plen ]; do b $((0x40 + j)) $((255 - j * 8)) $((j * 8)); j=$((j + 1)); done
    row=0
    while [ $row -lt $H ]; do
        y=$row; [ $top = 1 ] || y=$((H - 1 - row))
        col=0
        while [ $col -lt $W ]; do
            x=$col; [ $right = 0 ] || x=$((W - 1 - col))
            key; pixel; eval "p$col=\$px"
            col=$((col + 1))
        done
        if [ $type = 9 ] || [ $type = 10 ]; then
            rle_row
        else
            col=0
            while [ $col -lt $W ]; do pget $col; s="$s$pv"; col=$((col + 1)); done
        fi
        row=$((row + 1))
    done
    emit $name
}

# wav name channels bits frames extra_chunk(0, 1): 8-bit sample (f * 7 + c * 100) & 255,
# 16-bit sample f * 300 - 5000 (channel 0) or 5000 - f * 200 (channel 1), at 11025 Hz
wav() {
    name=$1; ch=$2; bits=$3; frames=$4; extra=$5
    len=$((frames * ch * bits / 8)); rate=11025
    s="${s}RIFF"; le32 $((4 + 24 + extra * 16 + 8 + len)); s="${s}WAVEfmt "
    le32 16; le16 1; le16 $ch; le32 $rate; le32 $((rate * ch * bits / 8)); le16 $((ch * bits / 8)); le16 $bits
    if [ $extra = 1 ]; then s="${s}LIST"; le32 8; s="${s}INFOtest"; fi
    s="${s}data"; le32 $len
    f=0
    while [ $f -lt $frames ]; do
        c=0
        while [ $c -lt $ch ]; do
            if [ $bits = 8 ]; then b $((f * 7 + c * 100))
            elif [ $c = 0 ]; then le16 $((f * 300 - 5000))
            else le16 $((5000 - f * 200)); fi
            c=$((c + 1))
        done
        f=$((f + 1))
    done
    emit $name
}

# cube name seed color char_size plane_size map_mode map_pages cell_size map_size: 8 big-endian header words,
# then the palette (16 or 256 colors) byte i = i * 3 + seed, the map i * 5 + seed and the cells i * 7 + seed
cube() {
    name=$1; seed=$2; color=$3; cells=$8; map=$9
    be32 0; be32 $cells; be32 $map; be32 $4; be32 $color; be32 $5; be32 $6; be32 $7
    pal=0
    [ $((color)) = 0 ] && pal=32
    [ $((color)) = 16 ] && pal=512
    i=0; while [ $i -lt $pal ]; do b $((i * 3 + seed)); i=$((i + 1)); done
    i=0; while [ $i -lt $map ]; do b $((i * 5 + seed)); i=$((i + 1)); done
    i=0; while [ $i -lt $cells ]; do b $((i * 7 + seed)); i=$((i + 1)); done
    emit $name
}

tga TC16BL.TGA 2 16 BL plain 0
tga TC24TL.TGA 2 24 TL plain 0
tga TC32TR.TGA 2 32 TR plain 0
tga TC24BR.TGA 2 24 BR plain 0
tga TCRLE24.TGA 10 24 BL rle 0
tga TCRLE32.TGA 10 32 TL rle 0
tga PAL256.TGA 1 8 BL plain 256
tga PAL16.TGA 1 8 TL plain 16
tga PAL16BR.TGA 1 8 BR plain 16
tga PALRLE.TGA 9 8 BL rle 256
tga PALRLE16.TGA 9 8 TL rle 16
wav M8.WAV 1 8 64 0
wav M16.WAV 1 16 64 1
wav S8.WAV 2 8 64 0
wav S16.WAV 2 16 64 0
cube TILE16.CUB 1 0x00 1 0 0xC000 1 256 64
cube TILE256.CUB 2 0x10 0 1 0x0000 2 128 32
cube TILERGB.CUB 3 0x30 1 3 0x8000 0x00020003 512 16
endef
# Simply expanded, so make passes the script to the shell as written ($ signs included)
export MEDIA_DATA_SH := $(value MEDIA_DATA_SH)
