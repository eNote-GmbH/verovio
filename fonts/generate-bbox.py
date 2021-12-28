# create svgpathtools Path objects from an SVG file
import json
import os
import sys
import xml.etree.ElementTree as ET
from svgpathtools import svg2paths

svg_ns = 'http://www.w3.org/2000/svg'

######################
#  Helper Functions  #
######################


def get_file_content(filepath):
    """Open file in same directory as script and retrieve its content."""
    location = os.path.realpath(os.path.dirname(__file__))
    file = open(os.path.join(location, filepath), 'r')
    content = file.read()
    file.close()
    return content

def write_file_content(filepath, content):
    """Write content to file with path relative to the script directory."""
    location = os.path.realpath(os.path.dirname(__file__))
    file = open(os.path.join(location, filepath), 'w')
    file.write(content)
    file.close()


def get_elements(xml, tag):
    """Retrieve all elements with given tag name from xml."""
    root = ET.fromstring(bytes(xml, encoding='utf-8'))
    return root.findall('.//' + tag)  # XPath, recursive


def get_svg_elements(root, tag):
    """Retrieve all elements with given tag name from svg."""
    return root.findall('.//svg:' + tag, {'svg': svg_ns})  # XPath, recursive
    
##################
#  Main program  #
##################

if __name__ == '__main__':
    # read paths and attributes for the file
    bounding_box_file = os.sys.argv[1]
    output_file = os.sys.argv[2]
    metadata_file_path = os.sys.argv[3]
    
    #############################

    metadata_file = open(metadata_file_path)
    metadata = json.load(metadata_file)
    
    glyphAnchors = []
    try:
        glyphAnchors = metadata['glyphsWithAnchors']
    except: 
        pass
    
    #############################
    
    try:
        font_svg_content = get_file_content(bounding_box_file)
    except OSError:
        print(f"Error opening font file {font_file_name}!")
        sys.exit(1)
    root = ET.fromstring(bytes(font_svg_content, encoding='utf-8'))
    font_family = root.get("font-family")
    font_faces = get_svg_elements(root, "font-face")
    if len(font_faces) != 1:
        print(f"Error: the file {font_file_name} should have a unique font-face element!")
        print(f"Please check that the svg has correct namespace: {svg_ns}")
        sys.exit(1)
    units_per_em = font_faces[0].get("units-per-em")
    
    #############################
    
    impl = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    impl += "<bounding-boxes ";
    impl += "font-family=\"" + font_family + "\" ";
    impl += "units-per-em=\"" + units_per_em + "\">\n";

    paths, attributes = svg2paths(bounding_box_file)

    for path, attribute in zip(paths, attributes):
        impl += "\t<g c=\"" + attribute['id'] + "\" ";
        
        xmin, xmax, ymin, ymax = path.bbox()
        
        impl += "x=\"{x}\" ".format(x = xmin);
        impl += "y=\"{y}\" ".format(y = ymin);
        impl += "w=\"{width}\" ".format(width = xmax - xmin);
        impl += "h=\"{height}\" ".format(height = ymax - ymin);
        try:
            h_a_x = attribute['horiz-adv-x']
            impl += "h-a-x=\"{horiz_adv_x}\" ".format(horiz_adv_x = h_a_x);
        except: 
            pass
        
        if(glyphAnchors):
            # handle glyphs with glyph anchors here
            impl += "/>\n";
        else:
            impl += "/>\n";
        
    impl += "</bounding-boxes>\n";
    
    #############################
    
    write_file_content(output_file, impl)
