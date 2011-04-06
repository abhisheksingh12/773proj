#!/usr/bin/env python2.7

import sys
import re

TAGSET = 'OS|OM|IP|IR|IB|SBR|SBE|SF|QP|QR|QB|QS|QM|INN|INP|MU|IDN|IDP|IS|GD|TH|PW|RT|RP|RN|P1|PC|PP|PM|PX|PO|PT|CS|MIN|MIC|MINEG|OT|P!|ESbr|MiC|MINEg|MINeg|MIneg|MiNEG|Min|Mineg|Ib|Pp|SbR|Sbr|Sf|oT|IN N|M INEG|MI N'

def clean_tag(t):
    t = re.sub(' ', '', t.upper())
    if t == 'P!':
        return 'P1'
    return t

def collect_by_dyads(lines):
    accum = []
    last_dyad = None
    # collect by dyads
    for (lineno, line) in lines:
        line = line.strip()
        # dyad number
        m = re.match(r'([0-9]+),[0-9]+,', line)
        if not m:
            print >> sys.stderr, 'm1', lineno, line
            continue
        dyad = m.group(1)
        line = line[len(m.group()):]

        if dyad != last_dyad:
            if accum:
                yield (last_dyad, accum)
            accum = [(lineno, line)]
            last_dyad = dyad
        else:
            accum.append((lineno, line))

    if accum:
        yield (last_dyad, accum)


def collect_by_speaker(lines):
    by_speakers = []
    for (lineno, line) in lines:
        m = re.match(r'(gr|wi)[^,]+,', line, re.I)
        if not m:
            if re.match('Name|Group|Date', line):
                continue
            if not by_speakers:
                print >> sys.stderr, 'm2', lineno, line
                continue
            speaker, _ = by_speakers[-1]
        else:
            # there is mispelling; so we just look at the first letter
            speaker = 'Grocery' if m.group().lower()[0] == 'g' else 'Wine'

        if m:
            line = re.sub(r'\-', '', line[len(m.group()):])
        else:
            line = re.sub(r'\-', '', line)


        sp = re.split('(Grocery,|Wine,)', line)
        if len(line) < 10 or len(sp) == 1 or (len(sp) == 3 and (len(sp[0]) < 10 or len(sp[2]) < 10)):
            if not by_speakers:
                by_speakers.append((speaker, [(lineno, line)]))
            else:
                last_speaker, _ = by_speakers[-1]
                if last_speaker == speaker:
                    by_speakers[-1][1].append((lineno, line))
                else:
                    by_speakers.append((speaker, [(lineno, line)]))
        elif len(sp) == 3:
            line = sp[0]
            if not by_speakers:
                by_speakers.append((speaker, [(lineno, line)]))
            else:
                last_speaker, _ = by_speakers[-1]
                if last_speaker == speaker:
                    by_speakers[-1][1].append((lineno, line))
                else:
                    by_speakers.append((speaker, [(lineno, line)]))

            speaker = sp[1]
            line = sp[2]
            if not by_speakers:
                by_speakers.append((speaker, [(lineno, line)]))
            else:
                last_speaker, _ = by_speakers[-1]
                if last_speaker == speaker:
                    by_speakers[-1][1].append((lineno, line))
                else:
                    by_speakers.append((speaker, [(lineno, line)]))
        else:
            print >> sys.stderr, 'm3', lineno, line
            continue
    return by_speakers


def assign_one_turn(lines):
    units = []
    tags = []
    linenos = []
    for (lineno, line) in lines:
        inline_tags = [clean_tag(t) for t in re.findall(r'\b('+TAGSET+r')\b', line)]
        line = re.sub(r'\b('+TAGSET+r')\b', '', line)
        if 'PM' in inline_tags and len(inline_tags) > 1 and 'QR' not in inline_tags:
            while 'PM' in inline_tags:
                inline_tags.remove('PM')

        inline_units = [i for i in [i.strip() for i in line.split('/')] if re.search('[a-zA-Z0-9?]', i)]

        if len(inline_units) != len(inline_tags):
            merge_units = inline_units[:1]
            for i in inline_units[1:]:
                if merge_units[-1] and re.match('[a-zA-Z0-9]', merge_units[-1][-1]):
                    merge_units[-1] += '/' + i
                else:
                    merge_units.append(i)
            inline_units = merge_units

        units.extend(inline_units)
        tags.extend(inline_tags)
        while len(linenos) < len(units) or len(linenos) < len(tags):
            linenos.append(lineno)

    if len(units) != len(tags):
        while len(units) < len(tags):
            units.append('')
        while len(tags) < len(units):
            tags.append('')
        print >> sys.stderr, 'm4', linenos, zip(units, tags)
        return []

    return zip(linenos, units, tags)


for (dyad, lines) in collect_by_dyads(enumerate(sys.stdin, 1)):
    for (speaker, lines) in collect_by_speaker(lines):
        for (lineno, unit, tag) in assign_one_turn(lines):
            print '\t'.join(map(str, [dyad, lineno, speaker, tag, unit]))
