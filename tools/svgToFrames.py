import re
from subprocess import Popen, PIPE
import os
import argparse
from collections import OrderedDict
from math import floor, ceil
import xml.etree.ElementTree as ETree

parser = argparse.ArgumentParser()
parser.add_argument("inp", help="input svg file")
parser.add_argument("out", help="output png and ini file names (without extension)")
parser.add_argument("--config-path", dest="config_path", default=".", help="output folder for the ini file")
parser.add_argument("--image-path", dest="image_path", default=".", help="output folder for the png file")
parser.add_argument("--dpi", type=float, dest="dpi", help="output png dpi")
parser.add_argument("--default-parent", type=str, dest="parent", help="default section parent")
parser.add_argument("--frustum", type=str, dest="frustum", help="an object to be used as export frustum")
args = parser.parse_args()

doc_root = ETree.parse(args.inp).getroot()

if args.dpi is None:
    args.dpi = float(doc_root.attrib.get('dpi', 90))

if args.parent is None:
    args.parent = doc_root.attrib.get('default-parent', None)

if args.frustum is None:
    args.frustum = doc_root.attrib.get('frustum', None)


frames = OrderedDict()
p = Popen(
          'inkscape -S "%s"' % args.inp,
          shell=True,
          stdout=PIPE,
          stderr=PIPE,
         )
all_values = map(lambda s: s.split(","), p.stdout.read().split("\n"))
elem_dict = OrderedDict(map(lambda v: (v[0], map(float, v[1:])), all_values))

if args.frustum is not None:
    _, _, pageW, pageH = map(int, doc_root.attrib['viewBox'].split(" "))
    (x, y, w, h) = elem_dict[args.frustum][0:4]
    (x1, y1) = map(lambda x: int(floor(x)), (x,pageH-h-y))
    (x2, y2) = map(lambda x: int(ceil(x)), (w+x, pageH-y))
    x,y = map(floor, (x,y))
    area_option = "-a %s:%s:%s:%s" % (x1, y1, x2, y2)
else:
    (x, y) = elem_dict['objects'][0:2]
    area_option = ""

export_command = 'inkscape -e "%s/%s.png" -j -i objects %s -d %s "%s"' % \
    (args.image_path, args.out, area_option, args.dpi, args.inp)

print "Export Command: " + export_command

os.system(export_command)


class Frame(object):
    def __init__(self):
        self.props = {}
        self.parent = None

scale = args.dpi/90.0


def transform_to_frame(point, corner):
    transformed = [scale * (point[0]-x), scale * (point[3]+point[1]-y)]
    return [tp-c for (tp, c) in zip(transformed, corner)]


for name, values in elem_dict.items():
    match = re.match("Frame:([^:]*):?(.*)?", name)
    if match:
        frame_name = match.group(1)
        corner = [values[0]-x, values[1]-y]
        corner = map(lambda x: floor(x*scale), corner)
        bottom_right = [values[2]+values[0]-x, values[3]+values[1]-y]
        bottom_right = map(lambda x: ceil(x*scale), bottom_right)
        size = [br-c for (c, br) in zip(corner, bottom_right)]
        frame = Frame()
        frame.props = {"TextureCorner": corner, "TextureSize": size}
        pivot = elem_dict.get("Pivot:"+frame_name, None)
        if pivot:
            frame.props["Pivot"] = transform_to_frame(pivot, corner)
        if match.group(2):
            frame.parent = match.group(2)
        else:
            if "parent" in args:
                frame.parent = args.parent
        frames[frame_name] = frame

for name, values in elem_dict.items():
    match = re.match("Marker:([^:]*):([^:]*):?(.*)?", name)
    if match:
        frame_name = match.group(1)
        marker_name = match.group(2)
        frame = frames[frame_name]
        marker_in_frame = transform_to_frame(values, frame.props["TextureCorner"])

        if "Pivot" in frame.props:
            pivot = frame.props["Pivot"]
        else:
            pivot = map(lambda x: x/2, frame.props["TextureSize"])

        marker_in_frame = [m-p for m, p in zip(marker_in_frame, pivot)]

        if match.group(3):
            marker_in_frame.append(float(match.group(3)))

        frame.props[marker_name] = marker_in_frame

with open("%s/%s.ini" % (args.config_path, args.out), 'w') as f:
    for name, frame in frames.items():
        if frame.parent:
            f.write("[%s@%s]\n" % (name, frame.parent))
        else:
            f.write("[%s]\n" % name)
        f.write('Texture=%s.png\n' % args.out)
        for propname, propval in frame.props.items():
            f.write("%s=(%s, %s, %s)\n" %
                    (propname, propval[0], propval[1], propval[2] if len(propval) > 2 else 0))
        if "Pivot" not in frame.props:
            f.write("Pivot = center\n")
