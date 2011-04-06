__ALL__ = ["group_by", "lazy_load_dyads", "write_dyads"]

def group_by(items, key):
    """Group and yield sorted items by key

    `items`: iterable sorted by key (i.e. all items with the same key appear together)
    `key`: a function that returns a key; should never return None;
    """
    accum = []
    last_key = None
    for item in items:
        new_key = key(item)
        if last_key != new_key:
            if accum:
                yield accum
                accum = []
            last_key = new_key
        accum.append(item)
    if accum: yield accum


def lazy_load_dyads(lines):
    def item_of(line):
        sp = line.strip().split('\t')
        dyad, _, role, code, unit = sp
        return (dyad, role, code, unit)
    def dyad_of(item):
        dyad, _, _, _ = item
        return dyad
    return group_by(map(item_of, lines), dyad_of)


def write_dyads(items, fo):
    for (dyad, role, code, unit) in items:
        fo.write('\t'.join(map(str, (dyad, '_', role, code, unit))) + '\n')
