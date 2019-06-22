#!/usr/bin/env python3

# Copyright (C) 2019 Jussi Pakkanen
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

import xml.etree.ElementTree as ET#

class SvgWriter:
    
    def __init__(self, fname):
        self.canvas_w = 1200
        self.canvas_h = 1400
        self.fname = fname
        
        self.w = 0.7
        self.font_size = 0.02
        self.x_height = 0.6
        self.x_overshoot = 0.62
        self.undershoot = -0.02
        self.cap_height = 0.9
        self.ascender_height = 0.92
        self.descender_height = -0.2

    def setup_canvas(self):
        self.root = ET.Element('svg', {'xmlns': 'http://www.w3.org/2000/svg',
                                       'width': str(self.canvas_w) + 'px',
                                       'height': str(self.canvas_h) + 'px'})
        ET.SubElement(self.root, 'rect', {'width': str(self.canvas_w) + 'px',
                                          'height': str(self.canvas_h) + 'px',
                                          'fill': 'white'})
        tmp = ET.SubElement(self.root, 'g', {'transform': 'translate(100, 1100)'})
        tmp = ET.SubElement(tmp, 'g', {'transform': 'matrix(1, 0, 0, -1, 0, 0)'})
        self.canvas = ET.SubElement(tmp, 'g', {'transform': 'scale(1000)'})

        basic_line = {'stroke': 'black',
                      'stroke-width': '0.002'}
        thin_line = {'stroke': 'black',
                     'stroke-width': '0.001'}
        self.canvas.append(self.line(-20, 0, 20, 0, **basic_line))
        self.canvas.append(self.line(0, -20, 0, 20, **basic_line))
        self.canvas.append(self.line(-20, 1, 20, 1, **basic_line))
        self.canvas.append(self.line(1, -20, 1, 20, **basic_line))
        self.canvas.append(self.line(self.w, -20, self.w, 20, **thin_line))
        self.canvas.append(self.text(-0.052, -0.022, '(0, 0)'))
        self.canvas.append(self.text(1.01, 1.01, '(1, 1)'))
        self.canvas.append(self.text(self.w + 0.01, 1.01, '(w, 1)'))

        self.horizontal_guide(self.x_height, 'X-height')
        self.horizontal_guide(self.x_overshoot, 'X-overshoot')
        self.horizontal_guide(self.undershoot, 'Undershoot')
        self.horizontal_guide(self.cap_height, 'Cap height')
        self.horizontal_guide(self.ascender_height, 'Ascender height')
        self.horizontal_guide(self.descender_height, 'Descender depth')

    def draw_splines(self):
        self.cubic_bezier(0.0, 0.0, 0.2, 0.8, 0.9, -0.2, 1.0, 0.0)

    def cubic_bezier(self, x1, y1, c1x, c1y, c2x, c2y, x2, y2):
        spline_style = {'stroke-width': '0.002',
                        'stroke': 'black',
                        'fill': 'none'}
        spline_control_line_style = {'stroke-width': '0.001',
                                     'stroke': 'black',
                                     'stroke-dasharray': '0.01,0.01'}
        spline_cross_style = {'stroke-width': '0.002',
                              'stroke': 'black'}
        
        spline = 'M{} {} C {} {} {} {} {} {}'.format(x1, y1, c1x, c1y, c2x, c2y, x2, y2)
        d = {'d': spline}
        d.update(spline_style)
        ET.SubElement(self.canvas, 'path', **d) 
        control1 = 'M{} {} L {} {}'.format(x1, y1, c1x, c1y)
        d = {'d': control1}
        d.update(spline_control_line_style)
        ET.SubElement(self.canvas, 'path', **d)
        control2 = 'M{} {} L {} {}'.format(x2, y2, c2x, c2y)
        d = {'d': control2}
        d.update(spline_control_line_style)
        ET.SubElement(self.canvas, 'path', **d)
        self.cross(c1x, c1y, 0.01)
        self.cross(c2x, c2y, 0.01)

    def cross(self, x, y, boxsize):
        cross_stroke_style = {'stroke-width': '0.002',
                              'stroke': 'black'}
        control2 = 'M{} {} L {} {} M{} {} L {} {}'.format(x-boxsize,
                                                          y-boxsize,
                                                          x+boxsize,
                                                          y+boxsize,
                                                          x-boxsize,
                                                          y+boxsize,
                                                          x+boxsize,
                                                          y-boxsize)
        d = {'d': control2}
        d.update(cross_stroke_style)
        ET.SubElement(self.canvas, 'path', **d)

    def horizontal_guide(self, h, txt):
        guide_style = {'stroke': 'black',
                       'stroke-width': '0.002',
                       'stroke-dasharray': '0.01,0.01'}
        self.canvas.append(self.line(-20, h, 20, h, **guide_style))
        self.canvas.append(self.text(0.82, h + 0.002, txt))

    def text(self, x, y, txt):
        r = ET.Element('g', {'transform': 'translate({}, {})'.format(x, y)})
        tmp = ET.SubElement(r, 'g', {'transform': 'matrix(1, 0, 0, -1, 0, 0)'})
        n = ET.SubElement(tmp, 'text', {'x': '0',
                                        'y': '0',
                                        'font-size': str(self.font_size),
                                        'fill': 'black'})
        n.text = txt
        return r

    def line(self, x1, y1, x2, y2, **kwargs):
        k = {'x1': str(x1),
             'y1': str(y1),
             'x2': str(x2),
             'y2': str(y2)}
        k.update(kwargs)
        return ET.Element('line', **k)

    def prettyprint_xml(self, ofname):
        import xml.dom.minidom
        # ElementTree can not do prettyprinting so do it manually
        doc = xml.dom.minidom.parse(ofname)
        with open(ofname, 'w', encoding='utf-8') as of:
            of.write(doc.toprettyxml(indent='  '))

    def write(self):
        ET.ElementTree(self.root).write(self.fname)
        self.prettyprint_xml(self.fname)

if __name__ == '__main__':
    sw = SvgWriter('testfile.svg')
    sw.setup_canvas()
    sw.draw_splines()
    sw.write()
