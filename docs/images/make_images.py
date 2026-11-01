"""Generates the images README.md uses, in a light and a dark version:
banner-{light,dark}.svg and how-it-works-{light,dark}.svg.

Usage: python3 docs/images/make_images.py docs/images
"""
import math, random, sys

COLS, ROWS, CELL, GAP = 16, 10, 22, 3
GX, GY = 72, 64                     # grid origin
W, H = 1280, 400
MARKED = [(2, 7), (5, 3), (7, 8), (9, 5), (12, 2), (13, 7), (15, 4)]

def lerp(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))

def hexc(c):
    return '#%02x%02x%02x' % c

def h2t(h):
    h = h.lstrip('#'); return tuple(int(h[i:i+2], 16) for i in (0, 2, 4))

THEMES = {
    'light': dict(
        bg0='#ffffff', bg1='#f3f0ff', border='#d0d7de',
        fg='#1f2328', muted='#59636e', accent='#8250df',
        sky=['#4dabf7', '#b197fc', '#ffc078'], orb='#ff922b', orbr=1.9, orbc=(12.5, 2.5),
        mount0='#7950f2', mount1='#4c2fa6', water0='#22b8cf', water1='#0b7285',
        chipbg='#f3f0ff', chipfg='#6639ba', stars=False),
    'dark': dict(
        bg0='#0d1117', bg1='#1a1433', border='#30363d',
        fg='#f0f6fc', muted='#9198a1', accent='#d2a8ff',
        sky=['#070b1e', '#1b1745', '#3a2f7a'], orb='#f1f3f5', orbr=1.75, orbc=(12.5, 2.5),
        mount0='#141029', mount1='#0b0918', water0='#1971c2', water1='#0b2a4a',
        chipbg='#2a1f4d', chipfg='#d2a8ff', stars=True),
}

def mountain_top(c):
    # two overlapping ridges, in rows
    a = 6.4 - 3.4 * math.exp(-((c - 3.5) / 2.4) ** 2)
    b = 6.6 - 2.4 * math.exp(-((c - 9.5) / 2.0) ** 2)
    return min(a, b)

def cell_color(t, c, r, rnd):
    if r >= 8:                                   # water
        base = lerp(h2t(t['water0']), h2t(t['water1']), (r - 8) / 1.5)
    elif r >= mountain_top(c):                   # mountains
        base = lerp(h2t(t['mount0']), h2t(t['mount1']), (r - 3) / 5)
    else:                                        # sky, in two bands
        f = r / 6.5
        sky = [h2t(x) for x in t['sky']]
        base = lerp(sky[0], sky[1], f * 2) if f < 0.5 else lerp(sky[1], sky[2], (f - 0.5) * 2)
        ox, oy = t['orbc']
        if math.hypot(c + 0.5 - ox, r + 0.5 - oy) <= t['orbr']:
            base = h2t(t['orb'])
        elif t['stars'] and rnd.random() < 0.10 and r < 4:
            base = lerp(base, (233, 236, 239), 0.55)
    j = rnd.uniform(-9, 9)                       # photo-like noise
    return tuple(max(0, min(255, round(v + j))) for v in base)

def svg(name):
    t = THEMES[name]
    rnd = random.Random(42)
    out = []
    a = out.append
    a(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" role="img" aria-labelledby="title desc">')
    a('<title id="title">steghide</title>')
    a('<desc id="desc">A small pixel-art picture in which a few pixels are outlined, standing for the pixels steghide changes by one shade to hide data, next to the name steghide.</desc>')
    a('<defs>')
    a(f'<linearGradient id="bg" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="{t["bg0"]}"/><stop offset="1" stop-color="{t["bg1"]}"/></linearGradient>')
    a(f'<linearGradient id="titlegrad" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="{t["fg"]}"/><stop offset="1" stop-color="{t["accent"]}"/></linearGradient>')
    a('</defs>')
    a(f'<rect x="1" y="1" width="{W-2}" height="{H-2}" rx="18" fill="url(#bg)" stroke="{t["border"]}" stroke-width="2"/>')
    # the picture
    pw, ph = COLS * (CELL + GAP) - GAP, ROWS * (CELL + GAP) - GAP
    a(f'<rect x="{GX-10}" y="{GY-10}" width="{pw+20}" height="{ph+20}" rx="12" fill="none" stroke="{t["border"]}" stroke-width="2"/>')
    for r in range(ROWS):
        for c in range(COLS):
            col = cell_color(t, c, r, rnd)
            x, y = GX + c * (CELL + GAP), GY + r * (CELL + GAP)
            a(f'<rect x="{x}" y="{y}" width="{CELL}" height="{CELL}" rx="3" fill="{hexc(col)}"/>')
    for c, r in MARKED:
        x, y = GX + c * (CELL + GAP), GY + r * (CELL + GAP)
        a(f'<rect x="{x-2.5}" y="{y-2.5}" width="{CELL+5}" height="{CELL+5}" rx="5" fill="none" stroke="{t["accent"]}" stroke-width="3"/>')
    cy = GY + ph + 42
    a(f'<rect x="{GX}" y="{cy-13}" width="14" height="14" rx="3" fill="none" stroke="{t["accent"]}" stroke-width="3"/>')
    a(f'<text x="{GX+26}" y="{cy}" font-family="ui-sans-serif, -apple-system, \'Segoe UI\', Helvetica, Arial, sans-serif" font-size="19" fill="{t["muted"]}">pixels nudged by one shade to carry the hidden data</text>')
    # the words
    TX = 560
    font = "ui-sans-serif, -apple-system, 'Segoe UI', Helvetica, Arial, sans-serif"
    a(f'<text x="{TX}" y="158" font-family="{font}" font-size="104" font-weight="800" letter-spacing="-3" fill="url(#titlegrad)">steghide</text>')
    a(f'<text x="{TX+4}" y="232" font-family="{font}" font-size="31" font-weight="600" fill="{t["fg"]}">Hide files inside images and audio.</text>')
    a(f'<text x="{TX+4}" y="270" font-family="{font}" font-size="21" fill="{t["muted"]}">A modernized fork that reads and writes steghide 0.5.1 files.</text>')
    x = TX + 4
    for label in ['JPEG · BMP · WAV · AU', 'passphrase + encryption', '0.5.1-compatible']:
        w = 14 + len(label) * 9.7
        a(f'<rect x="{x}" y="298" width="{w:.0f}" height="36" rx="18" fill="{t["chipbg"]}" stroke="{t["accent"]}" stroke-opacity="0.45"/>')
        a(f'<text x="{x + w/2:.0f}" y="322" text-anchor="middle" font-family="{font}" font-size="17" font-weight="600" fill="{t["chipfg"]}">{label}</text>')
        x += w + 12
    a('</svg>')
    return '\n'.join(out) + '\n'

FONT = "ui-sans-serif, -apple-system, 'Segoe UI', Helvetica, Arial, sans-serif"

DIAGRAM_THEMES = {
    'light': dict(bg0='#ffffff', bg1='#f6f8fa', border='#d0d7de', line='#59636e', muted='#59636e',
                  input='#8250df', step='#0969da', key='#bf8700', output='#1a7f37'),
    'dark': dict(bg0='#0d1117', bg1='#161b22', border='#30363d', line='#8b949e', muted='#9198a1',
                 input='#8957e5', step='#1f6feb', key='#9e6a03', output='#238636'),
}

def diagram(name):
    t = DIAGRAM_THEMES[name]
    out = []
    a = out.append
    w, h = 1200, 320
    a(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}" role="img" aria-labelledby="title desc">')
    a('<title id="title">How steghide hides a file</title>')
    a('<desc id="desc">Your file is compressed, encrypted with a key made from the passphrase, and hidden by nudging pixels or samples of the cover file that the passphrase picks. The result is a stego file that looks the same as the cover.</desc>')
    a('<defs>')
    a(f'<linearGradient id="panel" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="{t["bg0"]}"/><stop offset="1" stop-color="{t["bg1"]}"/></linearGradient>')
    a(f'<marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="{t["line"]}"/></marker>')
    a('</defs>')
    a(f'<rect x="1" y="1" width="{w-2}" height="{h-2}" rx="18" fill="url(#panel)" stroke="{t["border"]}" stroke-width="2"/>')

    def box(x, y, bw, bh, fill, label, sub):
        a(f'<rect x="{x}" y="{y}" width="{bw}" height="{bh}" rx="14" fill="{fill}"/>')
        a(f'<text x="{x + bw/2:.0f}" y="{y + bh/2 - 4:.0f}" text-anchor="middle" font-family="{FONT}" font-size="23" font-weight="700" fill="#ffffff">{label}</text>')
        a(f'<text x="{x + bw/2:.0f}" y="{y + bh/2 + 22:.0f}" text-anchor="middle" font-family="{FONT}" font-size="16" fill="#ffffff" fill-opacity="0.85">{sub}</text>')

    def line(points, dashed=False):
        d = ' '.join(f'{"M" if i == 0 else "L"}{x},{y}' for i, (x, y) in enumerate(points))
        dash = ' stroke-dasharray="7 6"' if dashed else ''
        a(f'<path d="{d}" fill="none" stroke="{t["line"]}" stroke-width="2.5"{dash} marker-end="url(#arrow)"/>')

    def note(x, y, text, anchor='middle'):
        a(f'<text x="{x}" y="{y}" text-anchor="{anchor}" font-family="{FONT}" font-size="15" font-style="italic" fill="{t["muted"]}">{text}</text>')

    top, th = 48, 92
    row = [(40, 180, 'input', 'your file', 'note.txt'),
           (275, 180, 'step', 'compress', 'takes less room'),
           (510, 180, 'step', 'encrypt', 'AES by default'),
           (745, 200, 'step', 'hide', 'nudge chosen pixels'),
           (995, 165, 'output', 'stego file', 'looks the same')]
    for x, bw, kind, label, sub in row:
        box(x, top, bw, th, t[kind], label, sub)
    for (x1, w1, *_), (x2, *_ ) in zip(row, row[1:]):
        line([(x1 + w1 + 6, top + th / 2), (x2 - 8, top + th / 2)])

    bot, bh = 210, 74
    box(510, bot, 180, bh, t['key'], 'passphrase', 'only you know it')
    box(840, bot, 200, bh, t['input'], 'cover file', 'photo.jpg')
    line([(600, bot - 6), (600, top + th + 8)], dashed=True)
    note(588, 184, 'makes the key', 'end')
    line([(696, bot + bh / 2), (790, bot + bh / 2), (790, top + th + 8)], dashed=True)
    note(742, bot + bh / 2 + 24, 'picks pixels')
    line([(900, bot - 6), (900, top + th + 8)])
    a('</svg>')
    return '\n'.join(out) + '\n'

outdir = sys.argv[1]
for name in THEMES:
    open(f'{outdir}/banner-{name}.svg', 'w').write(svg(name))
for name in DIAGRAM_THEMES:
    open(f'{outdir}/how-it-works-{name}.svg', 'w').write(diagram(name))
