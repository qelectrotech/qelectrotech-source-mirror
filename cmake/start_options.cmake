# Copyright 2006 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.

message(" - start_options")

# Comment the line below to deactivate the --common-elements-dir option
add_definitions(-DQET_ALLOW_OVERRIDE_CED_OPTION)

# Comment the line below to disable the --common-tbt-dir option
add_definitions(-DQET_ALLOW_OVERRIDE_CTBTD_OPTION)

# Comment the line below to deactivate the --config-dir option
add_definitions(-DQET_ALLOW_OVERRIDE_CD_OPTION)

# Comment the line below to deactivate the --data-dir option
add_definitions(-DQET_ALLOW_OVERRIDE_DD_OPTION)

#comment the line below to disable the project database export
add_definitions(-DQET_EXPORT_PROJECT_DB)
