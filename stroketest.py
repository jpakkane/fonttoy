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

class TestStroke(unittest.TestCase):

    def test_size(self):
        model = strokemodel.Stroke(1)
        self.assertEqual(len(model.points), 4)
        model2 = strokemodel.Stroke(6)
        self.assertEqual(len(model2.points), 19)

    def test_fixed(self):
        model = strokemodel.Stroke(1)
        target_point = strokemodel.Point(3, 3)
        last_point = model.points[-1].clone()
        c1 = strokemodel.FixedConstraint([0], [target_point])
        model.add_constraint(c1)
        self.assertFalse(model.points[0].is_close(target_point))
        model.single_round()
        self.assertTrue(model.points[0].is_close(target_point))
        self.assertTrue(model.points[-1].is_close(last_point))

if __name__ == '__main__':
    unittest.main()
