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
import math
import scipy.optimize

class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def distance(self, p):
        return math.sqrt(math.pow(p.x-self.x, 2) + math.pow(p.y-self.y, 2))

class Bezier:
    def __init__(self, p1, c1, c2, p2):
        self.p1 = p1
        self.c1 = c1
        self.c2 = c2
        self.p2 = p2

    def evaluate(self, t):
        x = math.pow(1.0-t, 3)*self.p1.x + 3.0*math.pow(1.0-t, 2)*t*self.c1.x + 3.0*(1.0-t)*t*t*self.c2.x + math.pow(t, 3)*self.p2.x
        y = math.pow(1.0-t, 3)*self.p1.y + 3.0*math.pow(1.0-t, 2)*t*self.c1.y + 3.0*(1.0-t)*t*t*self.c2.y + math.pow(t, 3)*self.p2.y
        return Point(x, y)

    def distance(self, p):
        i = 0.0
        delta = 0.1
        mindist = 1000000000
        cutoff = (1.0 + delta/2)
        while i<=cutoff:
            curpoint = self.evaluate(i)
            curdist = p.distance(curpoint)
            i += delta
            if curdist < mindist:
                mindist = curdist
                minpoint = curpoint
        return mindist

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

    def draw_cubicbezier(self, b):
        self.cubic_bezier(b.p1.x,
                          b.p1.y,
                          b.c1.x,
                          b.c1.y,
                          b.c2.x,
                          b.c2.y,
                          b.p2.x,
                          b.p2.y)

    def draw_box_marker(self, x, y):
        boxwidth = 0.02
        ET.SubElement(self.canvas, 'rect', {'x': str(x-boxwidth/2),
                                          'y': str(y-boxwidth/2),
                                          'width': str(boxwidth),
                                          'height': str(boxwidth),
                                          'fill': 'transparent',
                                          'stroke': 'black',
                                          'stroke-width': '0.002',})

    def draw_circle_marker(self, x, y):
        radius = 0.01
        ET.SubElement(self.canvas, 'circle', {'cx': str(x),
                                            'cy': str(y),
                                            'r': str(radius),
                                            'fill': 'transparent',
                                            'stroke': 'black',
                                            'stroke-width': '0.002',})

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

def state_to_beziers(x):
    a = x[0]
    p1 = Point(0.0, 0.0)
    p2 = Point(0.5, 0.4)
    p3 = Point(1.0, 0.0)
    c1 = Point(x[1], x[2])
    c2 = Point(p2.x - a, 0.4)
    c3 = Point(p2.x + a, 0.4)
    c4 = Point(x[3], x[4])
    b1 = Bezier(p1, c1, c2, p2)
    b2 = Bezier(p2, c3, c4, p3)
    return (b1, b2)

def target_function(x, state):
    (b1, b2) = state_to_beziers(x)
    t1 = Point(0.1, 0.2)
    t2 = Point(0.9, 0.25)

    return b1.distance(t1) + b2.distance(t2)

def minimize_test():
    stateobj = []
    res = scipy.optimize.minimize(target_function,
                                  [0.1, 0.1, 0.1, 0.9, 0.0],
                                  stateobj,
                                  bounds=[(0.01, None),
                                          (None, None),
                                          (None, None),
                                          (None, None),
                                          (None, None)])
    #print(res.x)
    #print(target_function(res.x, []))
    (b1, b2) = state_to_beziers(res.x)
    draw_result(b1, b2)

def draw_result(b1, b2):
    sw = SvgWriter('fontout.svg')
    sw.setup_canvas()
    sw.draw_cubicbezier(b1)
    sw.draw_cubicbezier(b2)
    # HAXXXXX
    t1 = Point(0.1, 0.2)
    t2 = Point(0.9, 0.25)
    sw.draw_circle_marker(t1.x, t1.y)
    sw.draw_circle_marker(t2.x, t2.y)
    sw.draw_box_marker(b1.p1.x, b1.p1.y)
    sw.draw_box_marker(b1.p2.x, b1.p2.y)
    sw.draw_box_marker(b2.p2.x, b2.p2.y)
    sw.write()

if __name__ == '__main__':
    minimize_test()
    sw = SvgWriter('testfile.svg')
    sw.setup_canvas()
    sw.draw_splines()
    sw.write()
