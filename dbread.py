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
        state = 'TRACK' #What are we looking for? A TRACK, VALUES, or FRAGMENTS?
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
            if state == 'FRAGMENTS':
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
                    state = 'TRACK'
                else:
                    if line[-1] == ':': 
                        # New group. If we're in a group, this is a group in a group, so a child group.
                        need_indent = True
                        new_group = ['group', line[:-1].strip(), []]
                        stack[-1].append(new_group)
                        stack.append(new_group[2])  # Add the children of the new group to the stack, because that is now the active group.
                    else:
                        name, start = line.rsplit(None, 1)
                        name = name.strip()
                        start = int(start)
                        stack[-1].append(['fragment', name, start])
            if state == 'TRACK':
                if key != 'Track':
                    raise ValueError('New track expected; got: ' + line)
                state = 'VALUES'
            if state in ('VALUES', 'TRACK'):
                if key == 'Track':
                    current = { 'root': root, 'name': value, 'files': [], 'fragments': [], 'tags' : [] }
                    tracks.append(current) #We're appending here, but because it's a dict, and dicts are mutable, we can still write to it and get the changes through. No return needed
                    tags = autotag(value, root)
                    if len(tags)>0:
                        current['tags'].extend(tags)
                elif key == 'Tags':
                    current['tags'].append(value.split(';'))
                elif key == 'File':
                    parts = value.split(';')
                    filename = parts[0].strip()
                    fileargs = [x.split('=') for x in parts[1:]]
                    offsets = [x[1] for x in fileargs if x[0] == 'offset'] 
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
                    state = 'FRAGMENTS'
                    current['tags'].append("has fragments")
def autotag(trackname, root):
    """Function to automagically add some tags based on filenames. Contents are up for discussion"""
    names = {"wals": ["wals","waltz","valse"], "polka":["polka"]}
    tags = set()
    for tag, options in names.items():
        for option in options:
            if option in trackname.lower():
                tags.add(tag)
                break
    return list(tags)


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
            for filename in files:
                if makepath(root, filename) in used:
                    continue
                if os.path.splitext(filename)[1] not in exts:
                    continue
                tracks.append({ 'root': root, 'name': filename, 'files': [(filename, 0)], 'fragments': [] , 'tags' : autotag(filename,root)})

    return load_test_tracks(tracks)

def get_times(fragments):
    """Returns a flattened list of all times in the fragments"""
    times = set()
    for fragment in fragments:
        if fragment[0]!='fragment' and type(fragment) == list:
            times.update( get_times(fragment[2]))
        else:
            if type(fragment[2]) == int:
                times.add(fragment[2])
    return times

def add_end_times(group,times):
    ret = []
    for fragment in group:
        
        if fragment[0] == 'group':
            ret.append([fragment[0],fragment[1],add_end_times(fragment[2],times)])
        else:
            ret.append((fragment[0],fragment[1],fragment[2],times[times.index(fragment[2])+1]))
    return ret

def load_test_tracks(tracks):
    """Opens all media files to insert end markers. Does not just do a single end marker, but also handles all fragments to give start and end times. This duplicates info, but makes it all a lot easier to handle"""
    import media
    for track in tracks:
        durations = []
        for filename,offset in track['files']:
            fullpath = os.path.join(track['root'], filename)
            durations.append( media.Media.get_duration(fullpath) + offset)
        times = []
        end = int(max(durations))
        times = sorted(list(get_times(track['fragments'])))
        times.append(end)
        track['fragments'] = add_end_times(track['fragments'],times)
    return tracks


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

def write_track(track):
    # Every track starts with an empty line and the track name.
    # There are two reasons for the empty line: it separates the tracks,
    # and it lets the BOM be on a line of its own, so grep can find lines
    # that start with 'Track:' without missing the first.
    ret = ''
    ret += '{}Track:{}{}'.format(EOL, track['name'], EOL)
    for file in track['files']:
        if file[1] != 0:
            ret += 'File:{};offset={}{}'.format(file[0], file[1], EOL)
        else:
            ret += 'File:{}{}'.format(file[0], EOL)
    ret += 'Fragments:{}'.format(EOL)
    ret += write_group(track['fragments'], INDENT)
    return ret

def write(tracks):
    '''Write the internal track data to a file that can be read back with db.read()'''

    output = {}
    for track in tracks:
        # Don't define tracks without fragments.
        if len(track['fragments']) == 0:
            continue
        target = makepath(track['root'], FRAGMENTS)
        if target not in output:
            output[target] = ''
        output[target] += write_track(track)

    for target in output:
        # Write as binary and manually encode as utf-8, so line endings are not mangled.
        with open(target, 'wb') as f:
            # To allow Microsoft to write these files as utf-8, start with a BOM.
            f.write(('\ufeff' + output[target]).encode('utf-8'))

# vim: set expandtab tabstop=4 shiftwidth=4 :
