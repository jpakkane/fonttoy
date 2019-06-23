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
        self.x = float(x)
        self.y = float(y)

    def distance(self, p):
        return math.sqrt(math.pow(p.x-self.x, 2) + math.pow(p.y-self.y, 2))

    def normalize(self, p):
        if math.fabs(p.x) < 0.0001 and math.fabs(p.y) < 0.0001:
            return Point(0.0, 0.0)
        d = self.distance(Point(0, 0))
        return Point(self.x/distance, self.y/distance)

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

    def evaluate_d1(self, t):
        x = 3.0*math.pow(1.0-t, 2)*(self.c1.x-self.p1.x) + 6.0*(1.0-t)*t*(self.c2.x-self.c1.x) + 3.0*t*t*(self.p2.x-self.c2.x)
        y = 3.0*math.pow(1.0-t, 2)*(self.c1.y-self.p1.y) + 6.0*(1.0-t)*t*(self.c2.y-self.c1.y) + 3.0*t*t*(self.p2.y-self.c2.y)
        return Point(x, y)

    def evaluate_d2(self, t):
        x = 6.0*(1.0-t)*(self.c2.x-2.0*self.c1.x + self.p1.x) + 6.0*t*(self.p2.x - 2.0*self.c2.x + self.p1.x)
        y = 6.0*(1.0-t)*(self.c2.y-2.0*self.c1.y + self.p1.y) + 6.0*t*(self.p2.y - 2.0*self.c2.y + self.p1.y)
        return Point(x, y)
    
    def evaluate_curvature(self, t):
        d1 = self.evaluate_d1(t)
        d2 = self.evaluate_d2(t)
        k_nom = math.fabs(d1.x*d2.y - d1.y*d2.x)
        k_denom = math.pow(d1.x*d1.x + d1.y*d1.y, 3.0/2.0)
        return k_nom/k_denom

    def evaluate_energy(self):
        # Note: probably inaccurate.
        i = 0.0
        delta = 0.1
        total_energy = 0.0
        cutoff = (1.0 + delta/2)
        while i<=cutoff:
            total_energy += delta*self.evaluate_curvature(i)
            i += delta
        return total_energy

    def closest_t(self, p):
        t = 0.0
        delta = 0.1
        mindist = 1000000000
        min_t = 0.0
        cutoff = (1.0 + delta/2)
        while t<=cutoff:
            curpoint = self.evaluate(t)
            curdist = p.distance(curpoint)
            if curdist < mindist:
                mindist = curdist
                min_t = t
            t += delta
        return min_t

    def distance(self, p):
        t = self.closest_t(p)
        closest_point = self.evaluate(t)
        closest_distance = closest_point.distance(p)
        return closest_distance

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

    def draw_constraint_circle(self, p, r):
        constraint_circle_style = {'stroke-width': '0.001',
                                   'stroke': 'black',
                                   'fill': 'transparent',
                                   'stroke-dasharray': '0.005,0.005'}
        d = {'cx': str(p.x),
             'cy': str(p.y),
             'r': str(r)}
        d.update(constraint_circle_style)
        ET.SubElement(self.canvas, 'circle', **d)


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

    return b1.distance(t1) + b2.distance(t2) + 0.001*(b1.evaluate_energy() + b2.evaluate_energy())

iter_count = 0

def callback(x):
    global iter_count
    (b1, b2) = state_to_beziers(x)
    draw_result('anim{}.svg'.format(iter_count), b1, b2)
    iter_count += 1

def basic_minimize_test():
    callback_fn = None
    res = scipy.optimize.minimize(target_function,
                                  [0.1, 0.1, 0.1, 0.9, 0.0],
                                  None,
                                  bounds=[(0.01, None),
                                          (None, None),
                                          (None, None),
                                          (None, None),
                                          (None, None)],
                                  callback=callback_fn)
    print(res.success)
    print(res.message.decode('utf-8', errors='replace'))
    (b1, b2) = state_to_beziers(res.x)
    draw_result('fontout.svg', b1, b2)

def draw_result(fname, b1, b2):
    sw = SvgWriter(fname)
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

def simple_draw():
    sw = SvgWriter('testfile.svg')
    sw.setup_canvas()
    sw.draw_splines()
    sw.write()

def tangentstate_to_bezier(x):
    p1 = Point(0, 0)
    p2 = Point(1, 0)
    c1 = Point(x[0], x[1])
    c2 = Point(x[2], x[3])
    b = Bezier(p1, c1, c2, p2)
    return b

def tangent_function(x, state=None):
    skeleton_point = Point(0.6, 0.2)
    b = tangentstate_to_bezier(x)
    
    # In reality calculate with normal.
    tangential_point = Point(0.6, 0.3)
    closest_t = b.closest_t(tangential_point)
    closest_point = b.evaluate(closest_t)
    #print('({}, {})'.format(closest_point.x, closest_point.y))
    differential = b.evaluate_d1(closest_t)
    distance_error = tangential_point.distance(closest_point)
    diff_error = math.fabs(differential.y) # FIXME calculate as difference to real direction vector
    total_energy = b.evaluate_energy()
    #print(distance_error)
    return distance_error + diff_error #+ 0.0002*total_energy

def draw_tangent(fname, b):
    targetpoint = Point(0.6, 0.2)
    r = 0.1
    sw = SvgWriter(fname)
    sw.setup_canvas()
    sw.draw_constraint_circle(targetpoint, r)
    sw.draw_circle_marker(targetpoint.x, targetpoint.y)
    sw.draw_cubicbezier(b)
    sw.write()

def tangent_callback(x):
    global iter_count
    b = tangentstate_to_bezier(x)
    draw_tangent('tang_anim{}.svg'.format(iter_count), b)
    iter_count += 1


def tangent_test():
    callback_fn = tangent_callback
    callback_fn = None
    initial_values = [0.0, 0.2, 1.0, 0.2]
    if callback_fn is not None:
        callback_fn(initial_values)
    res = scipy.optimize.minimize(tangent_function,
                                  initial_values,
                                  None,
                                  callback=callback_fn)
    print(res.success)
    message = res.message
    if isinstance(message, bytes):
        message = message.decode('utf-8', errors='replace')
    print(message)
    b = tangentstate_to_bezier(res.x)
    draw_tangent('tangent.svg', b)

if __name__ == '__main__':
    #simple_draw()
    basic_minimize_test()
    tangent_test()
