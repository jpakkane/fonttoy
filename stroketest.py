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


import unittest
import strokemodel

class DoNothingOptimizer:

    def optimize(self, model):
        pass

class TestStroke(unittest.TestCase):

    def test_size(self):
        model = strokemodel.Stroke(1)
        self.assertEqual(len(model.points), 4)
        model2 = strokemodel.Stroke(6)
        self.assertEqual(len(model2.points), 19)

    def test_fixed(self):
        opt = DoNothingOptimizer()
        model = strokemodel.Stroke(1)
        target_point = strokemodel.Point(3, 3)
        last_point = model.points[-1].clone()
        c1 = strokemodel.FixedConstraint([0], [target_point])
        self.assertEqual(model.calculate_constraint_error(), 0.0)
        self.assertEqual(model.num_unconstrained_points(), 4)
        model.add_constraint(c1)
        self.assertFalse(model.points[0].is_close(target_point))
        self.assertTrue(model.calculate_constraint_error() > 0.0)
        self.assertEqual(model.num_unconstrained_points(), 3)
        model.optimize(opt)
        self.assertTrue(model.points[0].is_close(target_point))
        self.assertTrue(model.points[-1].is_close(last_point))
        self.assertEqual(model.calculate_constraint_error(), 0.0)
        self.assertEqual(model.num_unconstrained_points(), 3)

if __name__ == '__main__':
    unittest.main()
