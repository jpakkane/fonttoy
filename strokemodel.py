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

from typing import List, Any
import math

class Point:
    def __init__(self, x, y):
        self.x = float(x)
        self.y = float(y)

    def distance(self, p) -> float:
        return math.sqrt(math.pow(p.x-self.x, 2) + math.pow(p.y-self.y, 2))

    def normalized(self, p):
        if math.fabs(p.x) < 0.0001 and math.fabs(p.y) < 0.0001:
            return Point(0.0, 0.0)
        d = self.distance(Point(0, 0))
        return Point(self.x/distance, self.y/distance)

    def is_close(self, p) -> bool:
        return self.distance(p) < 0.0001

    def clone(self):
        return Point(self.x, self.y)

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

class Constraint:
    pass

class FixedConstraint(Constraint):
    def __init__(self, pointindexes: List[int], values: List[Point]):
        super().__init__()
        assert(len(pointindexes) == len(values))
        self.pointindexes = pointindexes
        self.values = values

    def get_free_variables(self) -> List[float]:
        return []

#    def get_free_variable_default_values(self) -> List[float]:
#        return []

    def calculate_error(self, points: List[Point]) -> float:
        total_error = 0.0
        for i in range(len(self.pointindexes)):
            total_error += self.values[i].distance(points[self.pointindexes[i]])
        return total_error

    def get_limits(self):
        return []

    def set_free_variables(self, new_values: List[float], offset: int) -> int:
        assert(isinstance(new_values, list))
        return 0

    def update_model(self, points: List[Point]):
        for i, pi in enumerate(self.pointindexes):
            points[pi] = self.values[i]

    def determines_points(self) -> List[int]:
        return self.pointindexes

    def depends_on_constraints(self) -> List[int]:
        return []

class Stroke:

    def __init__(self, num_beziers: int):
        self.points = []
        num_points = (num_beziers)*3 + 1
        for i in range(num_points):
            self.points.append(Point(i/num_points, i/num_points))
        #self.beziers = []
        self.constraints = []

    def add_constraint(self, c: Constraint) -> None:
        self.constraints.append(c)

    def calculate_constraint_error(self) -> float:
        total_error = 0.0
        for c in self.constraints:
            total_error = c.calculate_error(self.points)
        return total_error

    def optimize(self, optimizer: Any):
        values = []
        limits = []
        for c in self.constraints:
            values += c.get_free_variables()
            limits += c.get_limits()
        optimizer.optimize(self)
        assert(len(values) == len(limits))
        new_values = values # SUPER LEET!
        offset = 0
        for c in self.constraints:
            offset += c.set_free_variables(new_values, offset)
        assert(offset == len(values))
        # Topological sorting here.
        for c in self.constraints:
            c.update_model(self.points)

    def num_unconstrained_points(self) -> int:
        is_constrained = [False] * len(self.points)
        for c in self.constraints:
            for pi in c.determines_points():
                assert(not is_constrained[pi])
                is_constrained[pi] = True
        return len([x for x in is_constrained if not x])
