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
# along with this program; if not, see <http://www.gnu.org/licenses/>.


import xml.etree.ElementTree as ET#
import math
import scipy.optimize
from strokemodel import Point, Bezier
import strokemodel

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

    def draw_filled_shape(self, b1, b2):
        shape_style = {'stroke': 'none',
                       'fill': 'red'}
        shape = 'M {} {} '.format(b1[0].p1.x, b1[0].p1.y)
        for b in b1:
            shape += 'C {} {} {} {} {} {} '.format(b.c1.x, b.c1.y, b.c2.x, b.c2.y, b.p2.x, b.p2.y)
        shape += 'L {} {}'.format(b2[0].p1.x, b2[0].p1.y)
        for b in b2:
            shape += 'C {} {} {} {} {} {} '.format(b.c1.x, b.c1.y, b.c2.x, b.c2.y, b.p2.x, b.p2.y)
        shape += ' z'
        d = {'d': shape}
        d.update(shape_style)
        ET.SubElement(self.canvas, 'path', **d)

    def draw_splines(self):
        self.cubic_bezier(0.0, 0.0, 0.2, 0.8, 0.9, -0.2, 1.0, 0.0)

    def draw_cubicbezier(self, b, draw_controls=True):
        self.cubic_bezier(b.p1.x,
                          b.p1.y,
                          b.c1.x,
                          b.c1.y,
                          b.c2.x,
                          b.c2.y,
                          b.p2.x,
                          b.p2.y,
                          draw_controls)

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

    def cubic_bezier(self, x1, y1, c1x, c1y, c2x, c2y, x2, y2, draw_controls=True):
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
        if draw_controls:
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

    def draw_line(self, p1, p2):
        line_style = {'stroke-width': '0.001',
                      'stroke': 'black'}
        control = 'M {} {} L {} {}'.format(p1.x, p1.y, p2.x, p2.y)
        d = {'d': control}
        d.update(line_style)
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

def tangentstate_to_beziers(x):
    p1 = Point(0, 0)
    p2 = Point(1, 0)
    c1 = Point(x[0], x[1])
    c2 = Point(x[2], x[3])
    b1 = Bezier(p1, c1, c2, p2)
    c1 = Point(x[4], x[5])
    c2 = Point(x[6], x[7])
    b2 = Bezier(p1, c1, c2, p2)
    return (b1, b2)

def tangent_energy(b, tangential_point):
    # In reality calculate with normal.
    closest_t = b.closest_t(tangential_point)
    closest_point = b.evaluate(closest_t)
    #print('({}, {})'.format(closest_point.x, closest_point.y))
    differential = b.evaluate_d1(closest_t)
    distance_error = tangential_point.distance(closest_point)
    diff_error = math.fabs(differential.y) # FIXME calculate as difference to real direction vector
    total_energy = b.evaluate_energy()
    return distance_error + diff_error + 0.00015*total_energy

def tangent_function(x, state=None):
    skeleton_point = Point(0.6, 0.2)
    tangential_point1 = Point(0.6, 0.3)
    tangential_point2 = Point(0.6, 0.1)
    (b1, b2) = tangentstate_to_beziers(x)
    
    b1_err = tangent_energy(b1, tangential_point1)
    b2_err = tangent_energy(b2, tangential_point2)
    #print(distance_error)
    return b1_err + b2_err 

def draw_tangent(fname, b1, b2):
    targetpoint = Point(0.6, 0.2)
    r = 0.1
    sw = SvgWriter(fname)
    sw.setup_canvas()
    sw.draw_constraint_circle(targetpoint, r)
    sw.draw_circle_marker(targetpoint.x, targetpoint.y)
    sw.draw_cubicbezier(b1)
    sw.draw_cubicbezier(b2)
    sw.write()

def tangent_callback(x):
    global iter_count
    b = tangentstate_to_bezier(x)
    draw_tangent('tang_anim{}.svg'.format(iter_count), b)
    iter_count += 1


def tangent_test():
    callback_fn = tangent_callback
    callback_fn = None
    initial_values = [0.0, 0.2, 1.0, 0.2, 0.1, 0.0, 0.9, 0.0]
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
    (b1, b2) = tangentstate_to_beziers(res.x)
    draw_tangent('tangent.svg', b1, b2)

def draw_model(fname, model, model2=None):
    r = 0.05
    sw = SvgWriter(fname)
    sw.setup_canvas()
    for b in model.beziers():
        sw.draw_cubicbezier(b)
    for p in model.fixed_points():
        sw.draw_constraint_circle(p, r)
    for b in model.beziers():
        for t in [0.0, 0.2, 0.4, 0.6, 0.8]:
            skeleton_point = b.evaluate(t)
            left_step = b.evaluate_left_normal(t)
            stroke_left = skeleton_point + left_step*r
            stroke_right = skeleton_point - left_step*r
            sw.draw_line(skeleton_point, stroke_left)
            sw.draw_line(skeleton_point, stroke_right)
    if model2:
        for b in model2.beziers():
            sw.draw_cubicbezier(b)

    sw.write()

def draw_filled(fname, b1, b2):
    sw = SvgWriter(fname)
    sw.setup_canvas()
    sw.draw_filled_shape(b1, b2)
    sw.write()

tunkki = None
tunkki2 = None
def ess_callback(x):
    global iter_count, tunkki, tunkki2
    draw_model('ess_anim{}.svg'.format(iter_count), tunkki, tunkki2)
    iter_count += 1


def es_test():
    h = 1.0
    w = 0.7
    e1 = 0.2
    e2 = h-e1
    e3 = 0.27
    e4 = h-e3
    r = 0.05

    m = strokemodel.Stroke(6)

    # All points first
    m.add_constraint(strokemodel.FixedConstraint(0, Point(r, e1)))
    m.add_constraint(strokemodel.FixedConstraint(3, Point(w/2, r)))
    m.add_constraint(strokemodel.FixedConstraint(6, Point(w-r, e3)))
    m.add_constraint(strokemodel.FixedConstraint(9, Point(w/2, h/2)))
    m.add_constraint(strokemodel.FixedConstraint(12, Point(r, e4)))
    m.add_constraint(strokemodel.FixedConstraint(15, Point(w/2, h-r)))
    m.add_constraint(strokemodel.FixedConstraint(18, Point(w-r, e2)))

    # Then control points.
    m.add_constraint(strokemodel.DirectionConstraint(3, 2, math.pi))
    m.add_constraint(strokemodel.MirrorConstraint(4, 2, 3))
    m.add_constraint(strokemodel.DirectionConstraint(6, 5, 3.0*math.pi/2.0))
    m.add_constraint(strokemodel.MirrorConstraint(7, 5, 6))
    m.add_constraint(strokemodel.AngleConstraint(8,
                                                 9,
                                                 (360.0-15.0)/360.0*2.0*math.pi,
                                                 (360.0-1.0)/360.0*2.0*math.pi))
    m.add_constraint(strokemodel.MirrorConstraint(10, 8, 9))
    m.add_constraint(strokemodel.DirectionConstraint(12, 11, 3.0*math.pi/2.0))
    m.add_constraint(strokemodel.MirrorConstraint(13, 11, 12))
    m.add_constraint(strokemodel.SameOffsetConstraint(14, 15, 2, 3))
    m.add_constraint(strokemodel.MirrorConstraint(16, 14, 15))
    m.add_constraint(strokemodel.SameOffsetConstraint(17, 18, 0, 1))

    assert(len(m.get_free_variables()) == 5)
    m.points[17] = Point(0.6, 0.9)
    m.fill_free_constraints()
    assert(len(m.get_free_variables()) == 7)

    global tunkki
    tunkki = m
    ess_callback(None)

    class InvokeWrapper:
        def __init__(self, model):
            self.model = model
            
        def __call__(self, x, *args):
            self.model.set_free_variables(x)
            self.model.update_model()
            #return self.model.calculate_energy()
            #return self.model.calculate_length()
            #return self.model.calculate_length() + self.model.calculate_energy()
            return self.model.evaluate_2nd_der_normal2()


    res = scipy.optimize.minimize(InvokeWrapper(m),
                                  m.get_free_variables(),
                                  None,
                                  bounds=m.get_free_variable_limits(),
                                  callback=ess_callback
                                  )
    m.set_free_variables(res.x)
    draw_model('ess_final.svg', m)
    print(res.success)
    message = res.message
    if isinstance(message, bytes):
        message = message.decode('utf-8', errors='replace')
    print(message)
    build_sides(m)

def reverse_bezlist(beziers):
    result = []
    for b in beziers:
        result.append(strokemodel.Bezier(b.p2, b.c2, b.c1, b.p1))
    return result[::-1]

def build_sides(m):
    lm = build_left_side(m)
    rm = build_right_side(m)
    draw_filled('filled.svg', list(lm.beziers()), reverse_bezlist(list(rm.beziers())))

def build_left_side(m):
    r = 0.05
    lm = strokemodel.Stroke(m.num_beziers)
    lm.points = m.points[:]
    zerob = m.bezier(0)
    lm.add_constraint(strokemodel.SmoothConstraint(4, 2, 3))
    lm.add_constraint(strokemodel.SmoothConstraint(7, 5, 6))
    lm.add_constraint(strokemodel.SmoothConstraint(10, 8, 9))
    lm.add_constraint(strokemodel.SmoothConstraint(13, 11, 12))
    lm.add_constraint(strokemodel.SmoothConstraint(16, 14, 15))
    lm.add_constraint(strokemodel.FixedConstraint(0, zerob.evaluate(0) + zerob.evaluate_left_normal(0.0)*r))

    # Edge is parallel to the skeleton.
    lm.add_constraint(strokemodel.DirectionConstraint(0, 1, zerob.evaluate_d1(0.0).angle()))
    for i, b in enumerate(m.beziers()):
        lm.add_constraint(strokemodel.DirectionConstraint(3*(i+1), 3*(i+1)-1, b.evaluate_d1(1.0).angle() + math.pi))
    for i in range(0, lm.num_beziers):
        curbez = m.bezier(i)
        lm.add_constraint(strokemodel.FixedConstraint((i+1)*3, curbez.evaluate(1.0) + curbez.evaluate_left_normal(1.0)*r))
        for o in (0.2, 0.4, 0.6, 0.8):
            lm.add_bezier_target_point(i, curbez.evaluate(o) + curbez.evaluate_left_normal(o)*r)
    lm.fill_free_constraints()

    global tunkki2
    tunkki2 = lm

    class InvokeWrapper:
        def __init__(self, model):
            self.model = model
            
        def __call__(self, x, *args):
            self.model.set_free_variables(x)
            self.model.update_model()
            self.model.update_model()
            #return self.model.calculate_energy()
            #return self.model.calculate_length()
            #return self.model.calculate_length() + self.model.calculate_energy()
            return self.model.calculate_something()

    res = scipy.optimize.minimize(InvokeWrapper(lm),
                                  lm.get_free_variables(),
                                  None,
                                  bounds=lm.get_free_variable_limits(),
                                  callback=ess_callback
                                  )
    lm.set_free_variables(res.x)
    draw_model('ess_sides.svg', m, lm)
    print(res.success)
    message = res.message
    if isinstance(message, bytes):
        message = message.decode('utf-8', errors='replace')
    print(message)
    return lm

# Yes, it's terrible but what do you expect. It's 3 AM currently
def build_right_side(m):
    r = 0.05
    lm = strokemodel.Stroke(m.num_beziers)
    lm.points = m.points[:]
    zerob = m.bezier(0)
    lm.add_constraint(strokemodel.SmoothConstraint(4, 2, 3))
    lm.add_constraint(strokemodel.SmoothConstraint(7, 5, 6))
    lm.add_constraint(strokemodel.SmoothConstraint(10, 8, 9))
    lm.add_constraint(strokemodel.SmoothConstraint(13, 11, 12))
    lm.add_constraint(strokemodel.SmoothConstraint(16, 14, 15))
    lm.add_constraint(strokemodel.FixedConstraint(0, zerob.evaluate(0) - zerob.evaluate_left_normal(0.0)*r))

    # Edge is parallel to the skeleton.
    lm.add_constraint(strokemodel.DirectionConstraint(0, 1, zerob.evaluate_d1(0.0).angle()))
    for i, b in enumerate(m.beziers()):
        lm.add_constraint(strokemodel.DirectionConstraint(3*(i+1), 3*(i+1)-1, b.evaluate_d1(1.0).angle() + math.pi))
    for i in range(0, lm.num_beziers):
        curbez = m.bezier(i)
        lm.add_constraint(strokemodel.FixedConstraint((i+1)*3, curbez.evaluate(1.0) - curbez.evaluate_left_normal(1.0)*r))
        for o in (0.2, 0.4, 0.6, 0.8):
            lm.add_bezier_target_point(i, curbez.evaluate(o) - curbez.evaluate_left_normal(o)*r)
    lm.fill_free_constraints()

    global tunkki2
    tunkki2 = lm

    class InvokeWrapper:
        def __init__(self, model):
            self.model = model
            
        def __call__(self, x, *args):
            self.model.set_free_variables(x)
            self.model.update_model()
            self.model.update_model()
            #return self.model.calculate_energy()
            #return self.model.calculate_length()
            #return self.model.calculate_length() + self.model.calculate_energy()
            return self.model.calculate_something()

    res = scipy.optimize.minimize(InvokeWrapper(lm),
                                  lm.get_free_variables(),
                                  None,
                                  bounds=lm.get_free_variable_limits(),
                                  callback=ess_callback
                                  )
    lm.set_free_variables(res.x)
    draw_model('ess_sides.svg', m, lm)
    print(res.success)
    message = res.message
    if isinstance(message, bytes):
        message = message.decode('utf-8', errors='replace')
    print(message)
    return lm

def print_nums():
    p1 = Point(0, 0)
    c1 = Point(0.25, 0)
    c2 = Point(0.75, 0)
    p2 = Point(1, 0)
    b = Bezier(p1, c2, c2, p2)
    print(b.evaluate_length())
    print(b.evaluate_energy())
    b.c1 = Point(0.25, 0.25)
    b.c2 = Point(0.75, -0.25)
    print(b.evaluate_length())
    print(b.evaluate_energy())

if __name__ == '__main__':
    #simple_draw()
    #basic_minimize_test()
    #tangent_test()
    es_test()
    #print_nums()

