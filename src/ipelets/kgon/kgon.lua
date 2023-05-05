----------------------------------------------------------------------
-- kgon ipelet description
----------------------------------------------------------------------
--[[

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2023 Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

--]]

label = "Regular k-gon"

about = [[
Constructs a regular k-gon from a circle.

This ipelet is part of Ipe.
]]

-- this variable will store the C++ ipelet when it has been loaded
ipelet = false

-- parameters for the C++ code
parameters = { n = "7" }

function run(model)
  if not ipelet then ipelet = assert(ipe.Ipelet(dllname)) end
  model:runIpelet(label, ipelet, 1, parameters)
end

-- define a shortcut for this function
shortcuts.ipelet_1_kgon = "Alt+Ctrl+K"

----------------------------------------------------------------------
