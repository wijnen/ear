# Database read and write support.
import fhs
import os

FRAGMENTS = 'fragments.txt'
INDENT = '\t'
EOL = '\r\n'
exts = ('.mp3', '.ogg', '.wav', '.mp4', '.ogv', '.mpg', '.mpeg', '.avi')

def makepath(root, filename):
    '''Write a filename in a form that can be used to detect duplicates'''
    return os.path.realpath(os.path.abspath(os.path.join(root, filename)))

def parse_fragments(root, tracks, used):
    '''Read tracks from a FRAGMENTS file and insert them into tracks parameter.
    Note: this does not add the 'end' marker to the tracks.'''
    with open(os.path.join(root, FRAGMENTS), 'r') as f:
        state = "TRACK" #What are we looking for? A TRACK, VALUES, or FRAGMENTS?
        for line in f:
            # Remove ending whitespace and starting BOM.
            line = line.lstrip('\ufeff').rstrip()
            # Empty lines and lines with # as the first character are ignored.
            # Note that while this allows for comments in the file, they are
            # not written back.
            # Also note that fragment and group names can start with #, because
            # their first character is always indentation.
            if line.strip() == '' or line.startswith('#'):
                continue
            if ':' in line:
                key, value = line.split(':', 1)
                value = value.strip()
            else:
                key, value = None, None
            if state == "FRAGMENTS":
                # Read fragments.
                indent = len(line) - len(line.lstrip())
                while indent < istack[-1]:
                    stack.pop()
                    istack.pop()
                if indent > istack[-1]:
                    if need_indent:
                        istack.append(indent)
                    else:
                        raise NotImplementedError('Annotations are not currently supported')
                elif need_indent:
                    raise ValueError('Expected indent')
                need_indent = False
                if indent == 0:
                    # Fragments are done, read next track.
                    state = "TRACK"
                else:
                    if line[-1] == ':':
                        # New group.
                        need_indent = True
                        stack[-1].append(['group', line[:-1].strip(), []])
                        stack.append(stack[-1][-1][2])
                    else:
                        name, start = line.rsplit(None, 1)
                        name = name.strip()
                        start = int(start)
                        stack[-1].append(['fragment', name, start])
            if state == "TRACK":
                if key != 'Track':
                    raise ValueError('New track expected; got: ' + line)
                state = "VALUES"
            if state in "VALUES","TRACK":
                if key == 'Track':
                    current = { 'root': root, 'name': value, 'files': [], 'fragments': [] }
                    tracks.append(current)
                elif key == 'File':
                    parts = value.split(';')
                    filename = parts[0].strip()
                    fileargs = [x.split('=') for x in parts[1:]]
                    offsets = [x for x in fileargs if x[0] == 'Offset']
                    if len(offsets) == 1:
                        offset = int(offsets[0])
                    else:
                        offset = 0
                    current['files'].append((filename, offset))
                    used.add(makepath(root, filename))
                elif key == 'Fragments':
                    need_indent = True
                    stack = [current['fragments']]
                    istack = [0]
                    state = "FRAGMENTS"

def read():
    '''Read all db files from all fhs data directories (and pwd)
    Return list of tracks.'''
    # db = list of tracks. 
    # track = { 'name': name, 'files': list of (filename, offset), 'fragments': list of fragments and groups, 'end': time }
    # fragment = ( 'fragment', name, start_time )
    # group = ( 'group', name, list of fragments and groups )
    tracks = []
    basedirs = fhs.read_data('db', dir = True, opened = False, multiple = True)
    
    # Parse all fragments files.
    used = set()
    for dirname in basedirs:
        for root, dirs, files in os.walk(dirname):
            if FRAGMENTS in files:
                if makepath(root, FRAGMENTS) in used:
                    continue
                used.add(makepath(root, FRAGMENTS))
                parse_fragments(root, tracks, used)

    # Add all other files.
    for dirname in basedirs:
        for root, dirs, files in os.walk(dirname):
            for f in files:
                if makepath(root, f) in used:
                    continue
                if os.path.splitext(f)[1] not in exts:
                    continue
                tracks.push({ 'root': root, 'name': f, 'files': [(filename, 0)], 'fragments': [] })

    return tracks

def write(tracks):
    '''Write the internal track data to a file that can be read back with db.read()'''

    def write_group(group, indent):
        '''Recursively write a group of fragments'''
        ret = ''
        for fragment in group:
            if fragment[0] == 'group':
                ret += '{}{}:{}'.format(indent, fragment[1], EOL)
                ret += write_group(fragment[2], indent + INDENT)
            elif fragment[0] == 'fragment':
                ret += '{}{}\t{}{}'.format(indent, fragment[1], fragment[2], EOL)
            else:
                assert(fragment[0] == 'end')
                # Ignore.
        return ret

    output = {}
    for track in tracks:
        # Don't define tracks without fragments.
        if len(track['fragments']) == 0:
            continue
        # Every track starts with an empty line and the track name.
        # There are two reasons for the empty line: it separates the tracks,
        # and it lets the BOM be on a line of its own, so grep can find lines
        # that start with 'Track:' without missing the first.
        target = makepath(track['root'], FRAGMENTS)
        if target not in output:
            output[target] = ''
        output[target] += '{}Track:{}{}'.format(EOL, track['name'], EOL)
        output[target] += 'Root:{}{}'.format(track['root'], EOL)
        for file in track['files']:
            if file[1] != 0:
                output[target] += 'File:{};Offset={}{}'.format(file[0], file[1], EOL)
            else:
                output[target] += 'File:{}{}'.format(file[0], EOL)
        output[target] += 'Fragments:{}'.format(EOL)
        output[target] += write_group(track['fragments'], INDENT)

    for target in output:
        # Write as binary and manually encode as utf-8, so line endings are not mangled.
        with open(target, 'wb') as f:
            # To allow Microsoft to write these files as utf-8, start with a BOM.
            f.write(('\ufeff' + output[target]).encode('utf-8'))

# vim: set expandtab tabstop=4 shiftwidth=4 :
