import argparse
import re
import os

parser = argparse.ArgumentParser()
parser.add_argument("root", help="The root ini file to conflate")
parser.add_argument("out", help="The conflated output ini file")
parser.add_argument("-I", help="Include path", action='append')
args = parser.parse_args()

include_pattern = re.compile('@([^@]*)@')


def conflateFile(filename, outfile):
    print "Conflating file %s" % filename
    filepath = None

    for path in args.I:
        fullpath = path + '/' + filename
        if os.path.isfile(fullpath):
            filepath = fullpath

    if filepath is None:
        raise IOError('%s was not found in any of the given paths' % filename)
    else:
        with open(filepath, 'r') as infile:
            for line in infile:
                include = include_pattern.match(line)
                if include is not None:
                    conflateFile(include.group(1), outfile)
                else:
                    outfile.write(line)
            outfile.write('\n')

if __name__ == "__main__":
    with open(args.out, 'w') as out:
        conflateFile(args.root, out)
