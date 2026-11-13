#!/usr/bin/env python3
"""Port menu names (TEXT_*) from the Eurorack firmware (LX_UMA) into the desktop's menu tables.

Usage: python3 -I tools/port_names.py TEXT_UMA_HOLD TEXT_UMA_GATE ...   (run from the repo root, after a build of
both firmwares, so their compile_commands.json exist)

For each name it takes the Eurorack's valueNames entry (short, category, long), adds the short / category / long
names the desktop lacks (enum in menu.h, string in MenuText.h, both at the end), the TEXT_ enum entry (before
NUM_NAMES) and the valueNames entry (menu.c). Both menu.c files are run through the compiler's preprocessor, so
#if blocks resolve as they build. Existing names are reused (their strings must match).
"""
import json, os, re, shlex, subprocess, sys

EURO = os.path.expanduser('~/src/eurorack/lxr-fm')
DESK = os.path.abspath('.')

def preprocess(repo):
    e = next(e for e in json.load(open(f'{repo}/build/release/compile_commands.json')) if e['file'].endswith('menu/menu.c'))
    args = shlex.split(e['command']); cc = args[0]
    flags = [a for a in args[1:] if a.startswith(('-I', '-D', '-m'))]
    return subprocess.run([cc, *flags, '-E', '-P', e['file']], check=True, cwd=e['directory'], capture_output=True, text=True).stdout

def parse(pp):
    def enum(name):
        body = re.search(r'enum\s+' + name + r'\s*\{(.*?)\};', pp, re.S).group(1)
        vals, cur = {}, -1
        for part in body.split(','):
            part = re.sub(r'//.*', '', part).strip()
            if not part: continue
            if '=' in part:
                k, v = [x.strip() for x in part.split('=', 1)]
                try: cur = int(v, 0)
                except ValueError: cur = vals[v.split('+')[0].strip()] + (int(v.split('+')[1]) if '+' in v else 0)
            else:
                k = part; cur += 1
            vals[k] = cur
        return vals
    def strings(arr):
        m = re.search(r'const char ' + arr + r'\[\]\[\d+\]\s*=\s*\{(.*?)\n\};', pp, re.S)
        return re.findall(r'\{\s*"((?:[^"\\]|\\.)*)"\s*\}', m.group(1))
    vn = re.findall(r'\{\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\}',
                    re.search(r'const Name valueNames\[[^\]]*\]\s*=\s*\{(.*?)\n\};', pp, re.S).group(1))
    return dict(names=enum('NamesEnum'), short=enum('shortNamesEnum'), cat=enum('catNamesEnum'), long=enum('longNamesEnum'),
                shortStr=strings('shortNames'), catStr=strings('catNames'), longStr=strings('longNames'), vn=vn)

def edit_append_before(path, marker_regex, add, crlf=True):
    s = open(path, newline='').read()
    m = re.search(marker_regex, s, re.S)
    assert m, (path, marker_regex)
    if crlf: add = add.replace('\n', '\r\n')
    s = s[:m.start(1)] + add + s[m.start(1):]
    open(path, 'w', newline='').write(s)

def main(wanted):
    eu, de = parse(preprocess(EURO)), parse(preprocess(DESK))
    new = {'short': [], 'cat': [], 'long': []}
    entries = []
    for t in wanted:
        if t in de['names']:
            print('already there:', t); continue
        sh, ca, lo = eu['vn'][eu['names'][t]]
        for kind, n in (('short', sh), ('cat', ca), ('long', lo)):
            s = eu[kind + 'Str'][eu[kind][n]]
            if n in de[kind]:
                if de[kind + 'Str'][de[kind][n]] != s:	# the desktop's own text stays (e.g. CAT_LFO "LFO" / "LFO A")
                    print(f'note: {n} is "{de[kind + "Str"][de[kind][n]]}" here, "{s}" on the Eurorack: kept')
            elif n not in [x for x, _ in new[kind]]:
                new[kind].append((n, s))
        entries.append((t, sh, ca, lo))
    if not entries: return
    mh = 'Core/Inc/menu/menu.h'; mt = 'Core/Inc/menu/MenuText.h'; mc = 'Core/Src/menu/menu.c'
    edit_append_before(mh, r'enum NamesEnum.*?(\r?\n\tNUM_NAMES)', ''.join(f'\n\t{t},' for t, *_ in entries))
    for kind, enumname, arr in (('short', 'shortNamesEnum', 'shortNames'), ('cat', 'catNamesEnum', 'catNames'), ('long', 'longNamesEnum', 'longNames')):
        if not new[kind]: continue
        # enum: after the last entry (it may lack a trailing comma)
        s = open(mh, newline='').read()
        m = re.search(r'enum ' + enumname + r'\s*\{(.*?)\r?\n\};', s, re.S)
        body = m.group(1).rstrip()
        if not body.endswith(','): body += ','
        body += ''.join(f'\r\n\t{n},' for n, _ in new[kind])
        s = s[:m.start(1)] + body + s[m.end(1):]
        open(mh, 'w', newline='').write(s)
        edit_append_before(mt, r'const char ' + arr + r'\[\]\[\d+\]\s*=\s*\{.*?(\r?\n\};)', ''.join(f'\n\t{{"{v}"}},\t// LX_UMA {n}' for n, v in new[kind]))
    edit_append_before(mc, r'const Name valueNames\[NUM_NAMES\] =.*?(\r?\n\};)', ''.join(f'\n\t\t{{{a}, {b}, {c}}},/*{t}*/' for t, a, b, c in entries))
    print('added', [t for t, *_ in entries], {k: [n for n, _ in v] for k, v in new.items()})

if __name__ == '__main__':
    main(sys.argv[1:])
